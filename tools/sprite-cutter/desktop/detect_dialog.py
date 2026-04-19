"""
DetectWorker + DetectionDialog — CV auto-detect integration.

Tasks:
  10.1  "Auto Detect" toolbar action  (added to MainWindow)
  10.2  CVDetectorConfig dialog        (tolerance / min-area / merge-gap sliders)
  10.3  QThread background worker      (non-blocking detection)
  10.4  Results pushed to canvas       (set_regions or append)
  10.5  Progress indicator             (indeterminate bar while running)
"""
from __future__ import annotations

from pathlib import Path
from typing import Optional

import numpy as np

from PyQt6.QtWidgets import (
    QDialog, QVBoxLayout, QHBoxLayout, QFormLayout,
    QLabel, QSlider, QSpinBox, QPushButton, QCheckBox,
    QProgressBar, QGroupBox, QWidget, QSizePolicy,
    QColorDialog,
)
from PyQt6.QtCore import (
    Qt, QThread, pyqtSignal, QObject,
)
from PyQt6.QtGui import QColor, QPixmap, QPainter

from core.cv_detector import CVDetector, CVDetectorConfig
from core.models import SpriteRegion


# ── 10.3  Background worker ───────────────────────────────────────────────────

class DetectWorker(QObject):
    """
    Runs CVDetector.detect() in a background QThread.

    Signals:
      finished(list[SpriteRegion])  — emitted when detection is complete
      error(str)                    — emitted on exception
    """
    finished: pyqtSignal = pyqtSignal(list)
    error:    pyqtSignal = pyqtSignal(str)

    def __init__(
        self,
        image: np.ndarray,
        config: CVDetectorConfig,
        bg_color: Optional[tuple[int, int, int]],
    ) -> None:
        super().__init__()
        self._image    = image
        self._config   = config
        self._bg_color = bg_color

    def run(self) -> None:
        try:
            detector = CVDetector(self._config)
            detector.load_array(self._image)
            if self._bg_color:
                detector.set_bg_color(*self._bg_color)
            regions = detector.detect()
            self.finished.emit(regions)
        except Exception as e:
            self.error.emit(str(e))


# ── Color swatch helper ───────────────────────────────────────────────────────

def _color_swatch(r: int, g: int, b: int, size: int = 20) -> QPixmap:
    px = QPixmap(size, size)
    px.fill(QColor(r, g, b))
    painter = QPainter(px)
    painter.setPen(QColor(80, 80, 120))
    painter.drawRect(0, 0, size - 1, size - 1)
    painter.end()
    return px


# ── 10.2  CVDetectorConfig Dialog ────────────────────────────────────────────

class DetectionDialog(QDialog):
    """
    Modal config + run dialog for CV auto-detection.

    After exec() == Accepted, call .result_regions() to get detected regions.
    """

    def __init__(
        self,
        image: np.ndarray,
        initial_bg: Optional[tuple[int, int, int]] = None,
        parent: QWidget | None = None,
    ) -> None:
        super().__init__(parent)
        self.setWindowTitle("Auto Detect Sprites")
        self.setMinimumWidth(400)
        self.setModal(True)

        self._image    = image
        self._regions: list[SpriteRegion] = []
        self._thread:  Optional[QThread]   = None
        self._worker:  Optional[DetectWorker] = None

        # Auto-detect bg if not given
        if initial_bg is None:
            from core.cv_detector import detect_background_color
            initial_bg = detect_background_color(image)
        self._bg_color: tuple[int, int, int] = initial_bg

        self._build_ui()
        self._apply_style()

    # ── UI construction ───────────────────────────────────────────────────────

    def _build_ui(self) -> None:
        layout = QVBoxLayout(self)
        layout.setSpacing(12)

        # ── Background colour ─────────────────────────────────────────────
        bg_box = QGroupBox("Background Color")
        bg_layout = QHBoxLayout(bg_box)

        self._bg_swatch = QLabel()
        self._bg_swatch.setFixedSize(24, 24)
        self._bg_swatch.setPixmap(_color_swatch(*self._bg_color))

        r, g, b = self._bg_color
        self._bg_label = QLabel(f"  RGB ({r}, {g}, {b})")
        self._bg_label.setStyleSheet("color:#8080c0;")

        pick_btn = QPushButton("Change...")
        pick_btn.setFixedWidth(80)
        pick_btn.clicked.connect(self._pick_bg_color)

        bg_layout.addWidget(self._bg_swatch)
        bg_layout.addWidget(self._bg_label)
        bg_layout.addStretch()
        bg_layout.addWidget(pick_btn)
        layout.addWidget(bg_box)

        # ── Parameter sliders ─────────────────────────────────────────────
        param_box = QGroupBox("Detection Parameters")
        form = QFormLayout(param_box)
        form.setSpacing(8)

        # Tolerance
        self._tol_slider = self._make_slider(0, 80, 15)
        self._tol_val    = QLabel("15")
        self._tol_slider.valueChanged.connect(
            lambda v: self._tol_val.setText(str(v))
        )
        tol_row = QHBoxLayout()
        tol_row.addWidget(self._tol_slider)
        tol_row.addWidget(self._tol_val)
        form.addRow("Tolerance:", tol_row)

        # Min area
        self._area_spin = QSpinBox()
        self._area_spin.setRange(1, 10000)
        self._area_spin.setValue(64)
        self._area_spin.setSuffix(" px²")
        form.addRow("Min area:", self._area_spin)

        # Merge gap
        self._gap_slider = self._make_slider(0, 30, 0)
        self._gap_val    = QLabel("0 (off)")
        self._gap_slider.valueChanged.connect(self._on_gap_changed)
        gap_row = QHBoxLayout()
        gap_row.addWidget(self._gap_slider)
        gap_row.addWidget(self._gap_val)
        form.addRow("Merge gap:", gap_row)

        # Name prefix
        from PyQt6.QtWidgets import QLineEdit
        self._prefix = QLineEdit("sprite")
        form.addRow("Name prefix:", self._prefix)
        layout.addWidget(param_box)

        # ── Options ───────────────────────────────────────────────────────
        self._replace_chk = QCheckBox("Replace existing regions")
        self._replace_chk.setChecked(True)
        layout.addWidget(self._replace_chk)

        # ── Result preview ────────────────────────────────────────────────
        self._result_lbl = QLabel("Press Detect to run.")
        self._result_lbl.setStyleSheet("color:#8080c0; font-style:italic;")
        layout.addWidget(self._result_lbl)

        # ── Progress bar (10.5) ───────────────────────────────────────────
        self._progress = QProgressBar()
        self._progress.setRange(0, 0)      # indeterminate
        self._progress.setVisible(False)
        layout.addWidget(self._progress)

        # ── Buttons ───────────────────────────────────────────────────────
        btn_row = QHBoxLayout()

        self._detect_btn = QPushButton("Detect")
        self._detect_btn.setDefault(True)
        self._detect_btn.clicked.connect(self._run_detect)

        self._accept_btn = QPushButton("Apply")
        self._accept_btn.setEnabled(False)
        self._accept_btn.clicked.connect(self.accept)

        cancel_btn = QPushButton("Cancel")
        cancel_btn.clicked.connect(self.reject)

        btn_row.addWidget(self._detect_btn)
        btn_row.addStretch()
        btn_row.addWidget(self._accept_btn)
        btn_row.addWidget(cancel_btn)
        layout.addLayout(btn_row)

    def _make_slider(self, lo: int, hi: int, val: int) -> QSlider:
        s = QSlider(Qt.Orientation.Horizontal)
        s.setRange(lo, hi)
        s.setValue(val)
        s.setTickPosition(QSlider.TickPosition.TicksBelow)
        s.setTickInterval((hi - lo) // 8)
        return s

    def _apply_style(self) -> None:
        self.setStyleSheet("""
            QDialog, QWidget {
                background: #1a1a2e;
                color: #c8c8e8;
            }
            QGroupBox {
                border: 1px solid #2a2a4a;
                border-radius: 4px;
                margin-top: 8px;
                color: #8080c0;
                font-weight: bold;
            }
            QGroupBox::title { subcontrol-origin: margin; left:8px; }
            QPushButton {
                background: #1e1e3a;
                border: 1px solid #3a3a5c;
                border-radius: 3px;
                color: #c8c8e8;
                padding: 4px 12px;
            }
            QPushButton:hover { background: #2a2a5a; }
            QPushButton:default { border-color: #6060c0; }
            QSlider::groove:horizontal {
                height: 4px;
                background: #2a2a4a;
                border-radius: 2px;
            }
            QSlider::handle:horizontal {
                background: #6060c0;
                width: 14px; height: 14px;
                margin: -5px 0;
                border-radius: 7px;
            }
            QProgressBar {
                border: 1px solid #3a3a5c;
                border-radius: 3px;
                background: #0f0f1e;
                color: #c8c8e8;
                text-align: center;
            }
            QProgressBar::chunk { background: #4040a0; }
            QSpinBox, QLineEdit {
                background: #0f0f1e;
                border: 1px solid #3a3a5c;
                border-radius: 3px;
                color: #c8c8e8;
                padding: 2px 4px;
            }
        """)

    # ── Slots ─────────────────────────────────────────────────────────────────

    def _on_gap_changed(self, v: int) -> None:
        self._gap_val.setText(f"{v}" if v > 0 else "0 (off)")

    def _pick_bg_color(self) -> None:
        initial = QColor(*self._bg_color)
        color   = QColorDialog.getColor(initial, self, "Background Color")
        if color.isValid():
            self._bg_color = (color.red(), color.green(), color.blue())
            self._bg_swatch.setPixmap(_color_swatch(*self._bg_color))
            r, g, b = self._bg_color
            self._bg_label.setText(f"  RGB ({r}, {g}, {b})")

    # ── 10.3  Background detection ────────────────────────────────────────────

    def _run_detect(self) -> None:
        self._detect_btn.setEnabled(False)
        self._accept_btn.setEnabled(False)
        self._progress.setVisible(True)
        self._result_lbl.setText("Detecting sprites...")
        self._result_lbl.setStyleSheet("color:#8080ff; font-style:italic;")

        config = CVDetectorConfig(
            tolerance=self._tol_slider.value(),
            min_area=self._area_spin.value(),
            merge_gap=self._gap_slider.value(),
            name_prefix=self._prefix.text().strip() or "sprite",
        )

        self._worker = DetectWorker(self._image, config, self._bg_color)
        self._thread = QThread()
        self._worker.moveToThread(self._thread)

        self._thread.started.connect(self._worker.run)
        self._worker.finished.connect(self._on_detect_finished)
        self._worker.error.connect(self._on_detect_error)
        self._worker.finished.connect(self._thread.quit)
        self._worker.error.connect(self._thread.quit)
        self._thread.finished.connect(self._thread.deleteLater)

        self._thread.start()

    def _on_detect_finished(self, regions: list[SpriteRegion]) -> None:
        self._regions = regions
        self._progress.setVisible(False)
        self._detect_btn.setEnabled(True)
        self._accept_btn.setEnabled(True)
        count = len(regions)
        self._result_lbl.setText(
            f"Found {count} sprite{'s' if count != 1 else ''}.  "
            "Press Apply to add to canvas."
        )
        self._result_lbl.setStyleSheet("color:#80ff80; font-style:normal;")

    def _on_detect_error(self, msg: str) -> None:
        self._progress.setVisible(False)
        self._detect_btn.setEnabled(True)
        self._result_lbl.setText(f"Error: {msg}")
        self._result_lbl.setStyleSheet("color:#ff6060; font-style:normal;")

    # ── Public API ────────────────────────────────────────────────────────────

    def result_regions(self) -> list[SpriteRegion]:
        return self._regions

    def replace_existing(self) -> bool:
        return self._replace_chk.isChecked()
