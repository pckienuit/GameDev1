"""
GridDialog — UI for uniform grid slicing configuration.

Tasks:
  11.2  GridConfig dialog  — tile_w / tile_h / offset / padding / skip_empty
  11.4  Run GridDetector   — push results to canvas
"""
from __future__ import annotations

from typing import Optional

from PyQt6.QtWidgets import (
    QDialog, QVBoxLayout, QHBoxLayout, QFormLayout,
    QLabel, QSpinBox, QPushButton, QCheckBox,
    QGroupBox, QWidget, QDialogButtonBox,
)
from PyQt6.QtCore import Qt

from core.grid_detector import GridDetector, GridConfig
from core.models import SpriteRegion


class GridDialog(QDialog):
    """
    Configure and preview a uniform grid slice.

    After exec() == Accepted, call .result_regions() for the sliced regions.
    """

    def __init__(
        self,
        img_w: int,
        img_h: int,
        initial: Optional[GridConfig] = None,
        parent: QWidget | None = None,
    ) -> None:
        super().__init__(parent)
        self.setWindowTitle("Grid Slicer")
        self.setMinimumWidth(360)
        self.setModal(True)

        self._img_w = img_w
        self._img_h = img_h
        self._regions: list[SpriteRegion] = []
        cfg = initial or GridConfig(tile_w=16, tile_h=16)

        self._build_ui(cfg)
        self._apply_style()
        self._update_preview()   # initial count

    # ── UI ────────────────────────────────────────────────────────────────────

    def _build_ui(self, cfg: GridConfig) -> None:
        layout = QVBoxLayout(self)
        layout.setSpacing(12)

        # ── Tile size ─────────────────────────────────────────────────────
        tile_box = QGroupBox("Tile Size")
        form1 = QFormLayout(tile_box)
        form1.setSpacing(6)

        self._tile_w = self._spin(1, 2048, cfg.tile_w)
        self._tile_h = self._spin(1, 2048, cfg.tile_h)
        form1.addRow("Tile Width (px):",  self._tile_w)
        form1.addRow("Tile Height (px):", self._tile_h)
        layout.addWidget(tile_box)

        # ── Offset ────────────────────────────────────────────────────────
        off_box = QGroupBox("Sheet Offset")
        form2 = QFormLayout(off_box)
        form2.setSpacing(6)

        self._off_x = self._spin(0, 4096, cfg.offset_x)
        self._off_y = self._spin(0, 4096, cfg.offset_y)
        form2.addRow("Offset X (px):", self._off_x)
        form2.addRow("Offset Y (px):", self._off_y)
        layout.addWidget(off_box)

        # ── Padding ───────────────────────────────────────────────────────
        pad_box = QGroupBox("Gap Between Tiles")
        form3 = QFormLayout(pad_box)
        form3.setSpacing(6)

        self._pad_x    = self._spin(0, 256, cfg.padding_h if cfg.padding_h else cfg.padding)
        self._pad_y    = self._spin(0, 256, cfg.padding_v if cfg.padding_v else cfg.padding)
        form3.addRow("Horizontal gap (px):", self._pad_x)
        form3.addRow("Vertical gap (px):",   self._pad_y)
        layout.addWidget(pad_box)

        # ── Options ───────────────────────────────────────────────────────
        self._skip_empty = QCheckBox("Skip empty cells (auto-detect)")
        self._skip_empty.setChecked(cfg.skip_empty)

        self._prefix_label = QLabel("Name prefix:")
        from PyQt6.QtWidgets import QLineEdit
        self._prefix = QLineEdit("tile")
        row = QHBoxLayout()
        row.addWidget(self._prefix_label)
        row.addWidget(self._prefix)
        layout.addLayout(row)
        layout.addWidget(self._skip_empty)

        # ── Preview ───────────────────────────────────────────────────────
        self._preview_lbl = QLabel("")
        self._preview_lbl.setStyleSheet("color:#80c080; font-style:italic;")
        layout.addWidget(self._preview_lbl)

        # ── Buttons ───────────────────────────────────────────────────────
        btns = QDialogButtonBox(
            QDialogButtonBox.StandardButton.Ok |
            QDialogButtonBox.StandardButton.Cancel
        )
        btns.accepted.connect(self._on_accept)
        btns.rejected.connect(self.reject)
        layout.addWidget(btns)

        # Connect all spinboxes to preview update
        for w in (self._tile_w, self._tile_h, self._off_x, self._off_y,
                  self._pad_x, self._pad_y):
            w.valueChanged.connect(self._update_preview)
        self._skip_empty.stateChanged.connect(self._update_preview)

    def _spin(self, lo: int, hi: int, val: int) -> QSpinBox:
        sb = QSpinBox()
        sb.setRange(lo, hi)
        sb.setValue(val)
        return sb

    def _apply_style(self) -> None:
        self.setStyleSheet("""
            QDialog, QWidget {
                background: #1a1a2e;
                color: #c8c8e8;
                font-family: 'Segoe UI', sans-serif;
            }
            QGroupBox {
                border: 1px solid #2a2a4a;
                border-radius: 4px;
                margin-top: 8px;
                color: #8080c0;
                font-weight: bold;
            }
            QGroupBox::title { subcontrol-origin: margin; left: 8px; }
            QSpinBox, QLineEdit {
                background: #0f0f1e;
                border: 1px solid #3a3a5c;
                border-radius: 3px;
                color: #c8c8e8;
                padding: 2px 4px;
            }
            QPushButton {
                background: #1e1e3a;
                border: 1px solid #3a3a5c;
                border-radius: 3px;
                color: #c8c8e8;
                padding: 4px 12px;
            }
            QPushButton:hover { background: #2a2a5a; }
            QDialogButtonBox QPushButton { min-width: 80px; }
            QCheckBox { color: #c8c8e8; }
            QLabel { color: #8080c0; }
        """)

    # ── Preview ───────────────────────────────────────────────────────────────

    def _build_config(self) -> GridConfig:
        return GridConfig(
            tile_w      = self._tile_w.value(),
            tile_h      = self._tile_h.value(),
            offset_x    = self._off_x.value(),
            offset_y    = self._off_y.value(),
            padding_h   = self._pad_x.value(),
            padding_v   = self._pad_y.value(),
            skip_empty  = self._skip_empty.isChecked(),
            name_prefix = self._prefix.text().strip() or "tile",
        )

    def _update_preview(self) -> None:
        """Compute cell count without running the full detector."""
        cfg = self._build_config()
        step_x = cfg.tile_w + cfg.padding_h
        step_y = cfg.tile_h + cfg.padding_v

        cols = max(0, (self._img_w - cfg.offset_x + cfg.padding_h)) // step_x \
               if step_x > 0 else 0
        rows = max(0, (self._img_h - cfg.offset_y + cfg.padding_v)) // step_y \
               if step_y > 0 else 0
        total = cols * rows

        skip_note = "  (some may be skipped)" if cfg.skip_empty else ""
        self._preview_lbl.setText(
            f"Grid: {cols} cols × {rows} rows = {total} cells{skip_note}"
        )

    # ── Accept ────────────────────────────────────────────────────────────────

    def _on_accept(self) -> None:
        """Run GridDetector and cache results before accepting."""
        cfg = self._build_config()
        try:
            det = GridDetector(cfg)
            # GridDetector needs an image — pass a dummy (size only matters for clipping)
            import numpy as np
            dummy = np.zeros((self._img_h, self._img_w, 3), dtype=np.uint8)
            det.load_array(dummy)
            self._regions = det.detect()
        except Exception as e:
            # Fall back: generate regions purely from grid math
            self._regions = self._compute_grid_regions(cfg)
        self.accept()

    def _compute_grid_regions(self, cfg: GridConfig) -> list[SpriteRegion]:
        """Pure-math fallback if GridDetector fails."""
        step_x = cfg.tile_w + cfg.gap_h
        step_y = cfg.tile_h + cfg.gap_v
        regions = []
        row = col = 0
        y = cfg.offset_y
        while y + cfg.tile_h <= self._img_h:
            x = cfg.offset_x
            col = 0
            while x + cfg.tile_w <= self._img_w:
                name = f"{cfg.name_prefix}_r{row:02d}_c{col:02d}"
                regions.append(SpriteRegion(name, x, y, cfg.tile_w, cfg.tile_h))
                x += step_x
                col += 1
            y += step_y
            row += 1
        return regions

    # ── Public API ────────────────────────────────────────────────────────────

    def result_regions(self) -> list[SpriteRegion]:
        return self._regions

    def current_config(self) -> GridConfig:
        return self._build_config()
