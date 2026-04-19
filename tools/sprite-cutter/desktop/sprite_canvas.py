"""
SpriteCanvas — ImageViewer + Region overlay + Draw/Select interaction.

Tasks:
  8.1  RegionOverlay paint    — draws SpriteRegion rects on image coords
  8.2  Draw mode              — click+drag creates new SpriteRegion
  8.3  Select mode            — click to select, Esc to deselect
  8.4  Resize via corner/edge handles (8 handle points)
  8.5  Delete selected region (Del/Backspace)
  8.6  Signals for MainWindow (region_added, region_selected, region_deleted,
                                regions_changed)
"""
from __future__ import annotations

import uuid
from enum import Enum, auto
from typing import Optional

from PyQt6.QtWidgets import QWidget, QInputDialog
from PyQt6.QtCore import Qt, QPointF, QRectF, pyqtSignal
from PyQt6.QtGui import (
    QPainter, QPen, QBrush, QColor,
    QMouseEvent, QKeyEvent, QPaintEvent,
)

from desktop.image_viewer import ImageViewer
from desktop.toolbar import EditorMode
from core.models import SpriteRegion
from core.grid_detector import GridConfig


# ── Visual constants ──────────────────────────────────────────────────────────

CLR_REGION_FILL   = QColor(80,  140, 255, 40)
CLR_REGION_BORDER = QColor(80,  140, 255, 200)
CLR_HOVER_FILL    = QColor(80,  200, 255, 55)
CLR_HOVER_BORDER  = QColor(80,  200, 255, 230)
CLR_SEL_FILL      = QColor(255, 200,  60, 55)
CLR_SEL_BORDER    = QColor(255, 200,  60, 230)
CLR_DRAW_FILL     = QColor(100, 255, 100, 40)
CLR_DRAW_BORDER   = QColor(100, 255, 100, 200)
CLR_HANDLE        = QColor(255, 200,  60, 230)
CLR_HANDLE_BG     = QColor(20,  20,  40, 200)
CLR_GRID_LINE     = QColor(255, 120,  60, 160)  # orange grid lines
CLR_GRID_CELL     = QColor(255, 120,  60,  20)  # very faint cell fill

HANDLE_SIZE  = 8    # screen px
HIT_SLOP     = 6    # extra px tolerance for clicking near a border


# ── Handle positions  ────────────────────────────────────────────────────────

class _Handle(Enum):
    TL = auto(); TC = auto(); TR = auto()
    ML = auto();              MR = auto()
    BL = auto(); BC = auto(); BR = auto()


_CURSOR_FOR_HANDLE = {
    _Handle.TL: Qt.CursorShape.SizeFDiagCursor,
    _Handle.TR: Qt.CursorShape.SizeBDiagCursor,
    _Handle.BL: Qt.CursorShape.SizeBDiagCursor,
    _Handle.BR: Qt.CursorShape.SizeFDiagCursor,
    _Handle.TC: Qt.CursorShape.SizeVerCursor,
    _Handle.BC: Qt.CursorShape.SizeVerCursor,
    _Handle.ML: Qt.CursorShape.SizeHorCursor,
    _Handle.MR: Qt.CursorShape.SizeHorCursor,
}


def _handle_positions(rect: QRectF) -> dict[_Handle, QPointF]:
    """Return 8 handle centre points for a screen-space rect."""
    x0, y0 = rect.left(),  rect.top()
    x1, y1 = rect.right(), rect.bottom()
    mx, my = (x0 + x1) / 2, (y0 + y1) / 2
    return {
        _Handle.TL: QPointF(x0, y0), _Handle.TC: QPointF(mx, y0),
        _Handle.TR: QPointF(x1, y0), _Handle.ML: QPointF(x0, my),
        _Handle.MR: QPointF(x1, my), _Handle.BL: QPointF(x0, y1),
        _Handle.BC: QPointF(mx, y1), _Handle.BR: QPointF(x1, y1),
    }


# ── SpriteCanvas ─────────────────────────────────────────────────────────────

class SpriteCanvas(ImageViewer):
    """
    Extends ImageViewer with:
    - Region overlay (all regions drawn as translucent rects)
    - Draw mode  (drag to create region, prompt for name)
    - Select mode (click to select, 8 resize handles)
    - Delete selected (Del/Backspace)

    Signals:
      region_added(SpriteRegion)
      region_selected(SpriteRegion | None)
      region_deleted(SpriteRegion)
      regions_changed()            — any CRUD change
    """
    region_added:    pyqtSignal = pyqtSignal(object)
    region_selected: pyqtSignal = pyqtSignal(object)
    region_deleted:  pyqtSignal = pyqtSignal(object)
    regions_changed: pyqtSignal = pyqtSignal()

    def __init__(self, parent: QWidget | None = None) -> None:
        super().__init__(parent)

        # ── Data ────────────────────────────────────────────────────────────
        self._regions:   list[SpriteRegion]    = []
        self._selected:  Optional[SpriteRegion] = None
        self._hovered:   Optional[SpriteRegion] = None
        self._mode:      EditorMode             = EditorMode.SELECT

        # ── Draw-mode state ─────────────────────────────────────────────────
        self._drawing:   bool      = False
        self._draw_start: Optional[QPointF] = None  # image coords
        self._draw_end:   Optional[QPointF] = None  # image coords

        # ── Resize-handle state ─────────────────────────────────────────────
        self._resizing:      bool              = False
        self._resize_handle: Optional[_Handle] = None
        self._resize_orig:   Optional[QRectF]  = None   # image-space rect before drag
        self._resize_anchor: Optional[QPointF] = None   # widget coords at drag start

        # ── Undo stack (simple list of snapshots) ───────────────────────────
        self._undo_stack: list[list[dict]] = []

        # ── Grid-mode overlay state (Phase 11) ──────────────────────────────
        self._grid_config: Optional[GridConfig] = None

    # ── Region management ────────────────────────────────────────────────────

    def set_regions(self, regions: list[SpriteRegion]) -> None:
        self._selected = None
        self._regions  = list(regions)
        self.update()

    def regions(self) -> list[SpriteRegion]:
        return list(self._regions)

    def clear_regions(self) -> None:
        self._push_undo()
        self._regions.clear()
        self._selected = None
        self.update()
        self.regions_changed.emit()

    def add_region(self, region: SpriteRegion) -> None:
        self._push_undo()
        self._regions.append(region)
        self.update()
        self.regions_changed.emit()

    def delete_selected(self) -> None:
        if self._selected is None:
            return
        self._push_undo()
        target = self._selected
        self._regions = [r for r in self._regions if r is not target]
        self._selected = None
        self.update()
        self.region_deleted.emit(target)
        self.regions_changed.emit()
        self.region_selected.emit(None)

    def select_region(self, region: Optional[SpriteRegion]) -> None:
        self._selected = region
        self.update()
        self.region_selected.emit(region)

    # ── Mode ──────────────────────────────────────────────────────────────────

    def set_mode(self, mode: EditorMode) -> None:
        self._mode = mode
        self._drawing = False
        self._resizing = False
        # Update cursor
        if mode == EditorMode.SELECT:
            self.setCursor(Qt.CursorShape.ArrowCursor)
        elif mode == EditorMode.DRAW:
            self.setCursor(Qt.CursorShape.CrossCursor)
        else:
            self.setCursor(Qt.CursorShape.CrossCursor)
        self.update()

    def set_grid_config(self, config: Optional[GridConfig]) -> None:
        """11.1: Set grid overlay config. Pass None to clear."""
        self._grid_config = config
        self.update()

    # ── Undo ──────────────────────────────────────────────────────────────────

    def _push_undo(self) -> None:
        snapshot = [r.to_dict() for r in self._regions]
        self._undo_stack.append(snapshot)
        if len(self._undo_stack) > 50:
            self._undo_stack.pop(0)

    def undo(self) -> bool:
        if not self._undo_stack:
            return False
        snapshot = self._undo_stack.pop()
        self._regions  = [SpriteRegion.from_dict(d) for d in snapshot]
        self._selected = None
        self.update()
        self.regions_changed.emit()
        self.region_selected.emit(None)
        return True

    # ── Coordinate helpers ────────────────────────────────────────────────────

    def _region_screen_rect(self, r: SpriteRegion) -> QRectF:
        """Convert image-space SpriteRegion to screen-space QRectF."""
        tl = self._image_to_widget(QPointF(r.x, r.y))
        br = self._image_to_widget(QPointF(r.x + r.w, r.y + r.h))
        return QRectF(tl, br)

    def _draw_rect_image(self) -> Optional[tuple[int, int, int, int]]:
        """Return normalised (x,y,w,h) in image pixels for current draw drag."""
        if self._draw_start is None or self._draw_end is None:
            return None
        x0, y0 = self._draw_start.x(), self._draw_start.y()
        x1, y1 = self._draw_end.x(),   self._draw_end.y()
        rx = int(min(x0, x1))
        ry = int(min(y0, y1))
        rw = max(1, abs(int(x1) - int(x0)))
        rh = max(1, abs(int(y1) - int(y0)))
        # Clamp to image bounds
        rx = max(0, min(rx, self._img_w - 1))
        ry = max(0, min(ry, self._img_h - 1))
        rw = min(rw, self._img_w - rx)
        rh = min(rh, self._img_h - ry)
        return rx, ry, rw, rh

    def _region_at(self, widget_pos: QPointF) -> Optional[SpriteRegion]:
        """Return the topmost region under widget_pos, or None."""
        for r in reversed(self._regions):
            sr = self._region_screen_rect(r)
            if sr.contains(widget_pos):
                return r
        return None

    def _handle_at(self, pos: QPointF) -> Optional[_Handle]:
        """Return resize handle under pos for selected region, or None."""
        if self._selected is None:
            return None
        sr = self._region_screen_rect(self._selected)
        hs = HANDLE_SIZE / 2 + HIT_SLOP
        for handle, centre in _handle_positions(sr).items():
            hx, hy = centre.x(), centre.y()
            if abs(pos.x() - hx) <= hs and abs(pos.y() - hy) <= hs:
                return handle
        return None

    # ── 8.1  Paint ───────────────────────────────────────────────────────────

    def paintEvent(self, event: QPaintEvent) -> None:
        super().paintEvent(event)           # draw image + grid + crosshair

        painter = QPainter(self)
        painter.setRenderHint(QPainter.RenderHint.Antialiasing, False)

        # 11.3  Grid overlay when in GRID mode
        if self._mode == EditorMode.GRID and self._grid_config and self._pixmap:
            self._paint_grid_overlay(painter)

        # Draw each region
        for region in self._regions:
            is_selected = region is self._selected
            is_hovered  = region is self._hovered and not is_selected
            self._paint_region(painter, region, is_selected, is_hovered)

        # Draw mode ghost rect
        if self._drawing and self._mode == EditorMode.DRAW:
            self._paint_draw_ghost(painter)

        painter.end()

    def _paint_region(self, painter: QPainter,
                      region: SpriteRegion,
                      selected: bool, hovered: bool) -> None:
        sr = self._region_screen_rect(region)

        fill   = CLR_SEL_FILL    if selected else (CLR_HOVER_FILL   if hovered else CLR_REGION_FILL)
        border = CLR_SEL_BORDER  if selected else (CLR_HOVER_BORDER if hovered else CLR_REGION_BORDER)

        painter.fillRect(sr, fill)
        pen = QPen(border, 1.5 if selected else 1.0)
        painter.setPen(pen)
        painter.drawRect(sr)

        # Label (only when zoom ≥ 1)
        if self._zoom >= 1.0 and sr.width() > 30:
            painter.setPen(QPen(QColor(220, 220, 255, 200)))
            painter.drawText(sr.adjusted(2, 2, 0, 0), region.name)

        # Resize handles for selected region
        if selected:
            self._paint_handles(painter, sr)

    def _paint_handles(self, painter: QPainter, sr: QRectF) -> None:
        hs = HANDLE_SIZE
        for _, centre in _handle_positions(sr).items():
            hx, hy = centre.x() - hs / 2, centre.y() - hs / 2
            painter.fillRect(QRectF(hx, hy, hs, hs), CLR_HANDLE_BG)
            pen = QPen(CLR_HANDLE, 1.5)
            painter.setPen(pen)
            painter.drawRect(QRectF(hx, hy, hs, hs))

    def _paint_draw_ghost(self, painter: QPainter) -> None:
        result = self._draw_rect_image()
        if not result:
            return
        rx, ry, rw, rh = result
        tl = self._image_to_widget(QPointF(rx, ry))
        br = self._image_to_widget(QPointF(rx + rw, ry + rh))
        sr = QRectF(tl, br)
        painter.fillRect(sr, CLR_DRAW_FILL)
        pen = QPen(CLR_DRAW_BORDER, 1.5, Qt.PenStyle.DashLine)
        painter.setPen(pen)
        painter.drawRect(sr)
        # Size label
        painter.setPen(QPen(QColor(200, 255, 200, 230)))
        painter.drawText(sr.adjusted(2, 2, 0, 0), f"{rw}×{rh}")

    # ── 11.3  Grid overlay ────────────────────────────────────────────────────

    def _paint_grid_overlay(self, painter: QPainter) -> None:
        """Draw uniform grid lines + faint cell fills over the image."""
        cfg = self._grid_config
        if not cfg:
            return

        step_x = cfg.tile_w + cfg.padding
        step_y = cfg.tile_h + (cfg.padding_y if cfg.padding_y else cfg.padding)

        pen = QPen(CLR_GRID_LINE, 0)   # cosmetic 1-screen-px line
        painter.setPen(pen)

        # Draw vertical cell boundaries
        x_img = cfg.offset_x
        while x_img <= self._img_w:
            sx = self._image_to_widget(QPointF(x_img, 0)).x()
            sy0 = self._image_to_widget(QPointF(0, cfg.offset_y)).y()
            sy1 = self._image_to_widget(QPointF(0, self._img_h)).y()
            painter.drawLine(QPointF(sx, sy0), QPointF(sx, sy1))
            x2 = x_img + cfg.tile_w
            sx2 = self._image_to_widget(QPointF(x2, 0)).x()
            painter.drawLine(QPointF(sx2, sy0), QPointF(sx2, sy1))
            x_img += step_x

        # Draw horizontal cell boundaries
        y_img = cfg.offset_y
        while y_img <= self._img_h:
            sy = self._image_to_widget(QPointF(0, y_img)).y()
            sx0 = self._image_to_widget(QPointF(cfg.offset_x, 0)).x()
            sx1 = self._image_to_widget(QPointF(self._img_w, 0)).x()
            painter.drawLine(QPointF(sx0, sy), QPointF(sx1, sy))
            y2 = y_img + cfg.tile_h
            sy2 = self._image_to_widget(QPointF(0, y2)).y()
            painter.drawLine(QPointF(sx0, sy2), QPointF(sx1, sy2))
            y_img += step_y

        # Faint cell fill
        painter.setPen(Qt.PenStyle.NoPen)
        x_img = cfg.offset_x
        row = 0
        while x_img + cfg.tile_w <= self._img_w:
            y_img = cfg.offset_y
            col = 0
            while y_img + cfg.tile_h <= self._img_h:
                if (row + col) % 2 == 0:
                    tl = self._image_to_widget(QPointF(x_img, y_img))
                    br = self._image_to_widget(QPointF(x_img + cfg.tile_w,
                                                       y_img + cfg.tile_h))
                    painter.fillRect(QRectF(tl, br), CLR_GRID_CELL)
                y_img += step_y
                col  += 1
            x_img += step_x
            row   += 1

    # ── 8.2  Draw mode interactions ───────────────────────────────────────────

    def _on_draw_press(self, pos: QPointF) -> None:
        img = self._widget_to_image(pos)
        self._draw_start = img
        self._draw_end   = img
        self._drawing    = True

    def _on_draw_move(self, pos: QPointF) -> None:
        if self._drawing:
            self._draw_end = self._widget_to_image(pos)
            self.update()

    def _on_draw_release(self, pos: QPointF) -> None:
        if not self._drawing:
            return
        self._draw_end = self._widget_to_image(pos)
        self._drawing  = False
        result = self._draw_rect_image()
        if result and result[2] >= 2 and result[3] >= 2:
            self._finish_draw(*result)
        self._draw_start = self._draw_end = None
        self.update()

    def _finish_draw(self, rx: int, ry: int, rw: int, rh: int) -> None:
        """Prompt for a name and add the new region."""
        # Build a default name
        idx  = len(self._regions) + 1
        default = f"sprite_{idx:03d}"
        name, ok = QInputDialog.getText(
            self, "Name Region",
            f"Name for region at ({rx},{ry})  [{rw}×{rh}]:",
            text=default,
        )
        if not ok or not name.strip():
            return
        region = SpriteRegion(name.strip(), rx, ry, rw, rh)
        self._push_undo()
        self._regions.append(region)
        self._selected = region
        self.update()
        self.region_added.emit(region)
        self.regions_changed.emit()
        self.region_selected.emit(region)

    # ── 8.3  Select mode interactions ────────────────────────────────────────

    def _on_select_press(self, pos: QPointF) -> None:
        # Check handle first (resize)
        handle = self._handle_at(pos)
        if handle is not None and self._selected is not None:
            self._resizing      = True
            self._resize_handle = handle
            self._resize_anchor = pos
            sr = self._selected
            self._resize_orig = QRectF(sr.x, sr.y, sr.w, sr.h)
            return
        # Click on region → select
        hit = self._region_at(pos)
        self.select_region(hit)

    def _on_select_move(self, pos: QPointF) -> None:
        if self._resizing and self._selected and self._resize_orig:
            self._apply_resize(pos)
            return
        # Update hover
        hit = self._region_at(pos)
        if hit is not self._hovered:
            self._hovered = hit
            self.update()
        # Update cursor for handle under pointer
        if self._selected:
            h = self._handle_at(pos)
            if h:
                self.setCursor(_CURSOR_FOR_HANDLE[h])
                return
        # Default cursor
        if hit:
            self.setCursor(Qt.CursorShape.PointingHandCursor)
        else:
            self.setCursor(Qt.CursorShape.ArrowCursor)

    def _on_select_release(self, _pos: QPointF) -> None:
        if self._resizing:
            self._resizing      = False
            self._resize_handle = None
            self._resize_orig   = None
            self.regions_changed.emit()
        self.setCursor(Qt.CursorShape.ArrowCursor)

    # ── 8.4  Resize logic ─────────────────────────────────────────────────────

    def _apply_resize(self, widget_pos: QPointF) -> None:
        assert self._selected and self._resize_orig and self._resize_anchor
        img_pos   = self._widget_to_image(widget_pos)
        img_orig  = self._widget_to_image(self._resize_anchor)
        dx = img_pos.x() - img_orig.x()
        dy = img_pos.y() - img_orig.y()

        o  = self._resize_orig
        x0, y0, x1, y1 = o.left(), o.top(), o.right(), o.bottom()

        h = self._resize_handle
        if h in (_Handle.TL, _Handle.ML, _Handle.BL): x0 += dx
        if h in (_Handle.TR, _Handle.MR, _Handle.BR): x1 += dx
        if h in (_Handle.TL, _Handle.TC, _Handle.TR): y0 += dy
        if h in (_Handle.BL, _Handle.BC, _Handle.BR): y1 += dy

        # Ensure minimum size (2×2 px) and normalise
        MIN = 2
        if x1 - x0 < MIN:
            if h in (_Handle.TL, _Handle.ML, _Handle.BL):
                x0 = x1 - MIN
            else:
                x1 = x0 + MIN
        if y1 - y0 < MIN:
            if h in (_Handle.TL, _Handle.TC, _Handle.TR):
                y0 = y1 - MIN
            else:
                y1 = y0 + MIN

        # Clamp to image
        x0 = max(0, min(x0, self._img_w - MIN))
        y0 = max(0, min(y0, self._img_h - MIN))
        x1 = max(x0 + MIN, min(x1, self._img_w))
        y1 = max(y0 + MIN, min(y1, self._img_h))

        r = self._selected
        r.x, r.y = int(x0), int(y0)
        r.w, r.h = max(MIN, int(x1 - x0)), max(MIN, int(y1 - y0))
        self.update()

    # ── Qt event routing ──────────────────────────────────────────────────────

    def mousePressEvent(self, event: QMouseEvent) -> None:
        # Let parent handle middle-click / space+drag pan
        if (event.button() == Qt.MouseButton.MiddleButton or
                self._space_held):
            super().mousePressEvent(event)
            return

        if not self._pixmap:
            return

        pos = QPointF(event.pos())
        if event.button() == Qt.MouseButton.LeftButton:
            if self._mode == EditorMode.DRAW:
                self._on_draw_press(pos)
            elif self._mode == EditorMode.SELECT:
                self._on_select_press(pos)

    def mouseMoveEvent(self, event: QMouseEvent) -> None:
        super().mouseMoveEvent(event)          # crosshair + pan
        if not self._pixmap or self._panning:
            return
        pos = QPointF(event.pos())
        if self._mode == EditorMode.DRAW:
            self._on_draw_move(pos)
        elif self._mode == EditorMode.SELECT:
            self._on_select_move(pos)

    def mouseReleaseEvent(self, event: QMouseEvent) -> None:
        super().mouseReleaseEvent(event)       # pan release
        if not self._pixmap or self._panning:
            return
        pos = QPointF(event.pos())
        if event.button() == Qt.MouseButton.LeftButton:
            if self._mode == EditorMode.DRAW:
                self._on_draw_release(pos)
            elif self._mode == EditorMode.SELECT:
                self._on_select_release(pos)

    def mouseDoubleClickEvent(self, event: QMouseEvent) -> None:
        """Double-click selected region → rename."""
        if self._mode == EditorMode.SELECT and self._selected:
            old_name = self._selected.name
            name, ok = QInputDialog.getText(
                self, "Rename Region", "New name:", text=old_name
            )
            if ok and name.strip() and name.strip() != old_name:
                self._push_undo()
                self._selected.name = name.strip()
                self.update()
                self.regions_changed.emit()

    def keyPressEvent(self, event: QKeyEvent) -> None:
        # 8.5  Delete selected
        if event.key() in (Qt.Key.Key_Delete, Qt.Key.Key_Backspace):
            self.delete_selected()
            return
        # Escape → deselect
        if event.key() == Qt.Key.Key_Escape:
            self.select_region(None)
            return
        super().keyPressEvent(event)
