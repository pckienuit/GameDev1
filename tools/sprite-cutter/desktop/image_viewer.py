"""
ImageViewer — Zoomable, pannable sprite-sheet canvas widget.

Tasks:
  7.1  Load + render image via QPainter
  7.2  Zoom  — Ctrl+Scroll, +/- keys, 10% → 3200%, centred on cursor
  7.3  Pan   — Middle-click drag  OR  Space+drag
  7.4  Pixel grid overlay — auto-show when zoom > 400%
  7.5  Crosshair + real-time pixel coordinate signal

Phase 8 will add draw/select/resize overlays on top of this widget.
"""
from __future__ import annotations

from pathlib import Path
from typing import Optional, Callable

import numpy as np
from PyQt6.QtWidgets import QWidget, QSizePolicy
from PyQt6.QtCore import (
    Qt, QPoint, QPointF, QRectF, QSize, pyqtSignal,
)
from PyQt6.QtGui import (
    QPainter, QPen, QBrush, QColor, QImage, QPixmap,
    QWheelEvent, QMouseEvent, QKeyEvent, QPaintEvent,
    QCursor,
)

# ── Constants ─────────────────────────────────────────────────────────────────
ZOOM_MIN        = 0.10          # 10%
ZOOM_MAX        = 32.0          # 3200%
ZOOM_STEP       = 0.15          # multiplicative step per scroll notch
GRID_ZOOM_THRESH = 4.0          # show pixel grid above this zoom level
GRID_COLOR      = QColor(80, 80, 140, 90)
CHECKER_A       = QColor(50, 50, 70)
CHECKER_B       = QColor(40, 40, 60)
CHECKER_SIZE    = 8             # px of each checker square in screen space


class ImageViewer(QWidget):
    """
    Core canvas.  Signals:
      pixel_hovered(x, y)     — emitted on mouse move (image pixel coords)
      pixel_left_image()       — emitted when cursor leaves image
      zoom_changed(float)      — emitted on zoom change (value = factor)
    """
    pixel_hovered:    pyqtSignal = pyqtSignal(int, int)
    pixel_left_image: pyqtSignal = pyqtSignal()
    zoom_changed:     pyqtSignal = pyqtSignal(float)

    def __init__(self, parent: QWidget | None = None) -> None:
        super().__init__(parent)

        # ── Image state ───────────────────────────────────────────────────────
        self._pixmap:    Optional[QPixmap] = None
        self._img_w:     int = 0
        self._img_h:     int = 0

        # ── View state ────────────────────────────────────────────────────────
        self._zoom:      float  = 1.0      # current zoom factor
        self._offset:    QPointF = QPointF(0, 0)   # image top-left in widget coords

        # ── Interaction state ─────────────────────────────────────────────────
        self._panning:     bool   = False
        self._space_held:  bool   = False
        self._pan_last:    QPoint = QPoint()
        self._cursor_img:  Optional[tuple[int, int]] = None   # (px, py) or None

        # ── Styling ───────────────────────────────────────────────────────────
        self.setMouseTracking(True)
        self.setFocusPolicy(Qt.FocusPolicy.StrongFocus)
        self.setSizePolicy(QSizePolicy.Policy.Expanding, QSizePolicy.Policy.Expanding)
        self.setStyleSheet("background:#0f0f1e;")
        self.setCursor(Qt.CursorShape.CrossCursor)

    # ── 7.1  Image loading ────────────────────────────────────────────────────

    def load_image(self, path: str | Path) -> bool:
        """Load image from disk. Returns True on success."""
        pixmap = QPixmap(str(path))
        if pixmap.isNull():
            return False
        self._pixmap = pixmap
        self._img_w  = pixmap.width()
        self._img_h  = pixmap.height()
        self._fit_in_view()
        self.update()
        return True

    def load_pixmap(self, pixmap: QPixmap) -> None:
        """Load from an already-created QPixmap."""
        self._pixmap = pixmap
        self._img_w  = pixmap.width()
        self._img_h  = pixmap.height()
        self._fit_in_view()
        self.update()

    def has_image(self) -> bool:
        return self._pixmap is not None

    def image_size(self) -> tuple[int, int]:
        return self._img_w, self._img_h

    # ── 7.2  Zoom ─────────────────────────────────────────────────────────────

    def zoom_factor(self) -> float:
        return self._zoom

    def set_zoom(self, factor: float,
                 anchor: Optional[QPointF] = None) -> None:
        """
        Set zoom, keeping `anchor` (widget coords) fixed in the view.
        If anchor is None, zoom around widget centre.
        """
        factor = max(ZOOM_MIN, min(ZOOM_MAX, factor))
        if abs(factor - self._zoom) < 1e-9:
            return

        if anchor is None:
            anchor = QPointF(self.width() / 2, self.height() / 2)

        # img-space point under the anchor before zoom
        img_pt = self._widget_to_image(anchor)

        self._zoom = factor

        # recompute offset so img_pt maps back to anchor
        self._offset = QPointF(
            anchor.x() - img_pt.x() * self._zoom,
            anchor.y() - img_pt.y() * self._zoom,
        )
        self._clamp_offset()
        self.zoom_changed.emit(self._zoom)
        self.update()

    def zoom_in(self) -> None:
        self.set_zoom(self._zoom * (1 + ZOOM_STEP))

    def zoom_out(self) -> None:
        self.set_zoom(self._zoom / (1 + ZOOM_STEP))

    def fit_in_view(self) -> None:
        self._fit_in_view()
        self.update()

    def zoom_to_100(self) -> None:
        self.set_zoom(1.0)

    # ── 7.3  Pan ──────────────────────────────────────────────────────────────

    def _start_pan(self, pos: QPoint) -> None:
        self._panning = True
        self._pan_last = pos
        self.setCursor(Qt.CursorShape.ClosedHandCursor)

    def _stop_pan(self) -> None:
        self._panning = False
        self.setCursor(Qt.CursorShape.CrossCursor)

    # ── Coordinate helpers ────────────────────────────────────────────────────

    def _widget_to_image(self, pt: QPointF) -> QPointF:
        """Convert widget-space point to image-space (fractional pixel)."""
        return QPointF(
            (pt.x() - self._offset.x()) / self._zoom,
            (pt.y() - self._offset.y()) / self._zoom,
        )

    def _image_to_widget(self, pt: QPointF) -> QPointF:
        return QPointF(
            pt.x() * self._zoom + self._offset.x(),
            pt.y() * self._zoom + self._offset.y(),
        )

    def _image_rect_in_widget(self) -> QRectF:
        """Bounding rect of the entire image in widget coords."""
        return QRectF(
            self._offset.x(),
            self._offset.y(),
            self._img_w * self._zoom,
            self._img_h * self._zoom,
        )

    def cursor_image_pos(self) -> Optional[tuple[int, int]]:
        return self._cursor_img

    # ── Fit / clamp ───────────────────────────────────────────────────────────

    def _fit_in_view(self) -> None:
        """Scale image to fill widget while keeping aspect ratio, centred."""
        if not self._pixmap or self.width() == 0 or self.height() == 0:
            return
        scale_x = self.width()  / self._img_w
        scale_y = self.height() / self._img_h
        self._zoom = max(ZOOM_MIN, min(scale_x, scale_y, 1.0))
        self._centre_image()
        self.zoom_changed.emit(self._zoom)

    def _centre_image(self) -> None:
        self._offset = QPointF(
            (self.width()  - self._img_w * self._zoom) / 2,
            (self.height() - self._img_h * self._zoom) / 2,
        )

    def _clamp_offset(self) -> None:
        """Allow panning slightly beyond the image edge for comfort."""
        if not self._pixmap:
            return
        margin_x = min(self.width()  * 0.8, self._img_w * self._zoom * 0.8)
        margin_y = min(self.height() * 0.8, self._img_h * self._zoom * 0.8)

        img_w_s = self._img_w * self._zoom
        img_h_s = self._img_h * self._zoom

        self._offset = QPointF(
            max(self.width()  - img_w_s - margin_x,
                min(self._offset.x(), margin_x)),
            max(self.height() - img_h_s - margin_y,
                min(self._offset.y(), margin_y)),
        )

    # ── 7.1 + 7.4 + 7.5  Paint ───────────────────────────────────────────────

    def paintEvent(self, event: QPaintEvent) -> None:
        painter = QPainter(self)
        painter.setRenderHint(QPainter.RenderHint.SmoothPixmapTransform,
                              self._zoom < 1.0)

        # Checkerboard background (transparency indicator)
        self._draw_checker(painter)

        if not self._pixmap:
            return

        # 7.1 Draw image
        img_rect = self._image_rect_in_widget()
        painter.drawPixmap(img_rect.toRect(), self._pixmap)

        # 7.4 Pixel grid
        if self._zoom >= GRID_ZOOM_THRESH:
            self._draw_pixel_grid(painter, img_rect)

        # 7.5 Crosshair
        if self._cursor_img is not None:
            self._draw_crosshair(painter)

    def _draw_checker(self, painter: QPainter) -> None:
        """Dark checkerboard to indicate canvas area."""
        sz = CHECKER_SIZE
        cols = self.width()  // sz + 1
        rows = self.height() // sz + 1
        for r in range(rows):
            for c in range(cols):
                color = CHECKER_A if (r + c) % 2 == 0 else CHECKER_B
                painter.fillRect(c * sz, r * sz, sz, sz, color)

    def _draw_pixel_grid(self, painter: QPainter, img_rect: QRectF) -> None:
        """Draw 1px grid lines aligned to image pixels."""
        pen = QPen(GRID_COLOR)
        pen.setWidth(0)             # cosmetic (1 screen px regardless of zoom)
        painter.setPen(pen)

        x0 = img_rect.left()
        y0 = img_rect.top()
        x1 = img_rect.right()
        y1 = img_rect.bottom()

        # Vertical lines
        col = 0
        while col <= self._img_w:
            sx = x0 + col * self._zoom
            if x0 <= sx <= x1:
                painter.drawLine(QPointF(sx, y0), QPointF(sx, y1))
            col += 1

        # Horizontal lines
        row = 0
        while row <= self._img_h:
            sy = y0 + row * self._zoom
            if y0 <= sy <= y1:
                painter.drawLine(QPointF(x0, sy), QPointF(x1, sy))
            row += 1

    def _draw_crosshair(self, painter: QPainter) -> None:
        """Thin crosshair at current cursor pixel, snapped to pixel boundary."""
        px, py = self._cursor_img
        # top-left of hovered pixel in widget coords
        sx = self._offset.x() + px * self._zoom
        sy = self._offset.y() + py * self._zoom

        pen = QPen(QColor(255, 255, 100, 160))
        pen.setWidth(0)
        painter.setPen(pen)
        # Horizontal bar
        painter.drawLine(QPointF(self._offset.x(), sy),
                         QPointF(self._offset.x() + self._img_w * self._zoom, sy))
        # Vertical bar
        painter.drawLine(QPointF(sx, self._offset.y()),
                         QPointF(sx, self._offset.y() + self._img_h * self._zoom))

    # ── Qt event handling ─────────────────────────────────────────────────────

    def wheelEvent(self, event: QWheelEvent) -> None:
        if not self._pixmap:
            return
        if event.modifiers() & Qt.KeyboardModifier.ControlModifier:
            # 7.2  Zoom centred on cursor
            delta = event.angleDelta().y()
            if delta > 0:
                self.set_zoom(self._zoom * (1 + ZOOM_STEP),
                              anchor=QPointF(event.position()))
            elif delta < 0:
                self.set_zoom(self._zoom / (1 + ZOOM_STEP),
                              anchor=QPointF(event.position()))
            event.accept()
        else:
            # Scroll pan (vertical or horizontal with Shift)
            dx = event.angleDelta().x()
            dy = event.angleDelta().y()
            self._offset += QPointF(dx * 0.5, dy * 0.5)
            self._clamp_offset()
            self.update()
            event.accept()

    def mousePressEvent(self, event: QMouseEvent) -> None:
        # Middle-click → pan
        if event.button() == Qt.MouseButton.MiddleButton:
            self._start_pan(event.pos())
            return
        # Space+left → pan
        if self._space_held and event.button() == Qt.MouseButton.LeftButton:
            self._start_pan(event.pos())
            return
        super().mousePressEvent(event)

    def mouseMoveEvent(self, event: QMouseEvent) -> None:
        # Pan
        if self._panning:
            delta = event.pos() - self._pan_last
            self._pan_last  = event.pos()
            self._offset   += QPointF(delta)
            self._clamp_offset()
            self.update()
            return

        # 7.5  Update crosshair + emit coord signal
        if self._pixmap:
            img_pt = self._widget_to_image(QPointF(event.pos()))
            px, py = int(img_pt.x()), int(img_pt.y())
            if 0 <= px < self._img_w and 0 <= py < self._img_h:
                if self._cursor_img != (px, py):
                    self._cursor_img = (px, py)
                    self.pixel_hovered.emit(px, py)
                    self.update()
            else:
                if self._cursor_img is not None:
                    self._cursor_img = None
                    self.pixel_left_image.emit()
                    self.update()

        super().mouseMoveEvent(event)

    def mouseReleaseEvent(self, event: QMouseEvent) -> None:
        if self._panning:
            self._stop_pan()
            return
        super().mouseReleaseEvent(event)

    def leaveEvent(self, event) -> None:  # type: ignore[override]
        if self._cursor_img is not None:
            self._cursor_img = None
            self.pixel_left_image.emit()
            self.update()

    def keyPressEvent(self, event: QKeyEvent) -> None:
        key = event.key()
        if key == Qt.Key.Key_Space:
            self._space_held = True
            if not self._panning:
                self.setCursor(Qt.CursorShape.OpenHandCursor)
        elif key == Qt.Key.Key_Plus or key == Qt.Key.Key_Equal:
            self.zoom_in()
        elif key == Qt.Key.Key_Minus:
            self.zoom_out()
        elif key == Qt.Key.Key_0 and event.modifiers() & Qt.KeyboardModifier.ControlModifier:
            self.fit_in_view()
        else:
            super().keyPressEvent(event)

    def keyReleaseEvent(self, event: QKeyEvent) -> None:
        if event.key() == Qt.Key.Key_Space:
            self._space_held = False
            if not self._panning:
                self.setCursor(Qt.CursorShape.CrossCursor)
        else:
            super().keyReleaseEvent(event)

    def resizeEvent(self, event) -> None:  # type: ignore[override]
        if self._pixmap and self._zoom == ZOOM_MIN:
            self._fit_in_view()
        super().resizeEvent(event)
