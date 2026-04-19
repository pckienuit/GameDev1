"""
Toolbar — mode buttons + action buttons for SpriteCutter desktop app.

Modes (mutually exclusive):
  Select  — click to select existing region
  Draw    — click+drag to create new region
  AI      — click to AI-segment a sprite (MobileSAM/SAM)
  Grid    — show grid overlay + grid config panel

Actions:
  Open Image, Export JSON, Import JSON
"""
from __future__ import annotations
from enum import Enum, auto

from PyQt6.QtWidgets import (
    QToolBar, QButtonGroup, QToolButton, QWidget, QSizePolicy,
)
from PyQt6.QtCore import Qt, pyqtSignal
from PyQt6.QtGui import QIcon, QFont


class EditorMode(Enum):
    SELECT = auto()
    DRAW   = auto()
    AI     = auto()
    GRID   = auto()


# ── Icon labels (emoji fallback when no icon pack is present) ──────────────────
_MODE_ICONS: dict[EditorMode, str] = {
    EditorMode.SELECT: "🖱",
    EditorMode.DRAW:   "✏",
    EditorMode.AI:     "🤖",
    EditorMode.GRID:   "📐",
}
_MODE_TIPS: dict[EditorMode, str] = {
    EditorMode.SELECT: "Select mode — click to select/move a region  [1]",
    EditorMode.DRAW:   "Draw mode — click+drag to create a region    [2]",
    EditorMode.AI:     "AI mode — click to AI-segment a sprite       [3]",
    EditorMode.GRID:   "Grid mode — slice sheet into uniform grid     [4]",
}


class SpriteCutterToolbar(QToolBar):
    """
    Top toolbar: mode toggle buttons + separator + action buttons.
    Emits `mode_changed(EditorMode)` when the user switches mode.
    """
    mode_changed: pyqtSignal = pyqtSignal(object)   # EditorMode

    def __init__(self, parent: QWidget | None = None) -> None:
        super().__init__("Tools", parent)
        self.setMovable(False)
        self.setToolButtonStyle(Qt.ToolButtonStyle.ToolButtonTextBesideIcon)

        self._mode_group = QButtonGroup(self)
        self._mode_group.setExclusive(True)
        self._mode_buttons: dict[EditorMode, QToolButton] = {}

        self._build_mode_buttons()
        self.addSeparator()
        self._build_action_buttons()

        # Default mode
        self._mode_buttons[EditorMode.SELECT].setChecked(True)
        self._current_mode = EditorMode.SELECT

    # ── Build helpers ─────────────────────────────────────────────────────────

    def _build_mode_buttons(self) -> None:
        for mode in EditorMode:
            btn = QToolButton(self)
            btn.setText(f"  {_MODE_ICONS[mode]}  {mode.name.capitalize()}")
            btn.setToolTip(_MODE_TIPS[mode])
            btn.setCheckable(True)
            btn.setMinimumWidth(110)
            btn.setStyleSheet(self._btn_style())
            self._mode_group.addButton(btn)
            self._mode_buttons[mode] = btn
            self.addWidget(btn)
            btn.clicked.connect(lambda checked, m=mode: self._on_mode_clicked(m))

    def _build_action_buttons(self) -> None:
        """Spacer and action buttons — wired up by MainWindow via signals."""
        spacer = QWidget()
        spacer.setSizePolicy(QSizePolicy.Policy.Expanding, QSizePolicy.Policy.Preferred)
        self.addWidget(spacer)

    @staticmethod
    def _btn_style() -> str:
        return """
            QToolButton {
                padding: 4px 8px;
                border: 1px solid #3a3a5c;
                border-radius: 4px;
                background: #1e1e3a;
                color: #c8c8e8;
                font-size: 12px;
            }
            QToolButton:checked {
                background: #4040a0;
                border-color: #8080ff;
                color: #ffffff;
            }
            QToolButton:hover:!checked {
                background: #2a2a50;
            }
        """

    # ── Public API ────────────────────────────────────────────────────────────

    def current_mode(self) -> EditorMode:
        return self._current_mode

    def set_mode(self, mode: EditorMode) -> None:
        self._mode_buttons[mode].setChecked(True)
        self._current_mode = mode

    def set_ai_enabled(self, enabled: bool) -> None:
        self._mode_buttons[EditorMode.AI].setEnabled(enabled)
        if not enabled:
            self._mode_buttons[EditorMode.AI].setToolTip(
                "AI mode — MobileSAM not installed (pip install mobile-sam)"
            )

    # ── Slots ─────────────────────────────────────────────────────────────────

    def _on_mode_clicked(self, mode: EditorMode) -> None:
        self._current_mode = mode
        self.mode_changed.emit(mode)
