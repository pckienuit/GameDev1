"""
SidebarWidget — Right-side panel for SpriteCutter Desktop.

Tasks:
  9.1  Sprite list       — QListWidget, click to select, shows name + coords
  9.2  Properties panel  — editable name, x, y, w, h, group (spinbox/line edit)
  9.3  Sync              — list selection  <->  canvas selection (bidirectional)
  9.4  Animation groups  — list, add / rename / remove, frame list
  9.5  Wired into MainWindow via signals
"""
from __future__ import annotations

from typing import Optional

from PyQt6.QtWidgets import (
    QWidget, QVBoxLayout, QHBoxLayout, QFormLayout,
    QLabel, QListWidget, QListWidgetItem, QPushButton,
    QLineEdit, QSpinBox, QGroupBox, QScrollArea,
    QSplitter, QSizePolicy, QInputDialog, QMessageBox,
)
from PyQt6.QtCore import Qt, pyqtSignal, QSignalBlocker
from PyQt6.QtGui import QColor, QFont

from core.models import SpriteRegion, AnimGroup


# ── Styles ────────────────────────────────────────────────────────────────────

_SIDEBAR_STYLE = """
QWidget {
    background: #12121f;
    color: #c8c8e8;
    font-family: 'Segoe UI', 'Inter', sans-serif;
    font-size: 12px;
}
QGroupBox {
    border: 1px solid #2a2a4a;
    border-radius: 4px;
    margin-top: 8px;
    padding-top: 4px;
    font-weight: bold;
    color: #8080c0;
}
QGroupBox::title {
    subcontrol-origin: margin;
    left: 8px;
    padding: 0 4px;
}
QListWidget {
    background: #0f0f1e;
    border: 1px solid #2a2a4a;
    border-radius: 3px;
    color: #c8c8e8;
    selection-background-color: #4040a0;
}
QListWidget::item:hover { background: #1e1e3a; }
QLineEdit, QSpinBox {
    background: #1a1a30;
    border: 1px solid #3a3a5c;
    border-radius: 3px;
    color: #c8c8e8;
    padding: 2px 4px;
}
QLineEdit:focus, QSpinBox:focus { border-color: #6060c0; }
QPushButton {
    background: #1e1e3a;
    border: 1px solid #3a3a5c;
    border-radius: 3px;
    color: #c8c8e8;
    padding: 3px 8px;
    min-width: 24px;
}
QPushButton:hover { background: #2a2a5a; border-color: #6060c0; }
QPushButton:pressed { background: #4040a0; }
QLabel { color: #8080c0; }
QScrollArea { border: none; }
"""


# ── Small helper: section header label ────────────────────────────────────────

def _header(text: str) -> QLabel:
    lbl = QLabel(text)
    lbl.setStyleSheet("color:#a0a0e0; font-weight:bold; padding:2px 0;")
    return lbl


def _icon_btn(text: str, tip: str) -> QPushButton:
    btn = QPushButton(text)
    btn.setToolTip(tip)
    btn.setFixedWidth(28)
    return btn


# ── 9.1  Sprite list ──────────────────────────────────────────────────────────

class _SpriteListPanel(QWidget):
    """
    Shows all SpriteRegions in a QListWidget.
    Emits `selection_changed(SpriteRegion | None)` when user clicks.
    """
    selection_changed: pyqtSignal = pyqtSignal(object)

    def __init__(self, parent: QWidget | None = None) -> None:
        super().__init__(parent)
        layout = QVBoxLayout(self)
        layout.setContentsMargins(0, 0, 0, 0)
        layout.setSpacing(4)

        # Header row
        hdr = QHBoxLayout()
        hdr.addWidget(_header("SPRITES"))
        hdr.addStretch()
        self._count_lbl = QLabel("0")
        self._count_lbl.setStyleSheet("color:#6060a0;")
        hdr.addWidget(self._count_lbl)
        layout.addLayout(hdr)

        # List
        self._list = QListWidget()
        self._list.setAlternatingRowColors(True)
        self._list.currentRowChanged.connect(self._on_row_changed)
        layout.addWidget(self._list)

    def _make_item(self, region: SpriteRegion) -> QListWidgetItem:
        text = f"{region.name}\n  ({region.x}, {region.y})  {region.w}×{region.h}"
        item = QListWidgetItem(text)
        item.setData(Qt.ItemDataRole.UserRole, id(region))
        if region.group:
            item.setForeground(QColor("#80c0ff"))
        return item

    def refresh(self, regions: list[SpriteRegion]) -> None:
        with QSignalBlocker(self._list):
            self._list.clear()
            for r in regions:
                self._list.addItem(self._make_item(r))
        self._count_lbl.setText(str(len(regions)))

    def highlight(self, region: Optional[SpriteRegion],
                  regions: list[SpriteRegion]) -> None:
        """Highlight the row corresponding to `region` without emitting signal."""
        with QSignalBlocker(self._list):
            if region is None:
                self._list.clearSelection()
                self._list.setCurrentRow(-1)
                return
            rid = id(region)
            for row in range(self._list.count()):
                item = self._list.item(row)
                if item and item.data(Qt.ItemDataRole.UserRole) == rid:
                    self._list.setCurrentRow(row)
                    return

    def _on_row_changed(self, row: int) -> None:
        self.selection_changed.emit(row)   # row index into current list


# ── 9.2  Properties panel ─────────────────────────────────────────────────────

class _PropertiesPanel(QWidget):
    """
    Editable fields for the selected SpriteRegion.
    Emits `region_changed(SpriteRegion)` on any field edit.
    """
    region_changed: pyqtSignal = pyqtSignal(object)

    def __init__(self, parent: QWidget | None = None) -> None:
        super().__init__(parent)
        layout = QVBoxLayout(self)
        layout.setContentsMargins(0, 0, 0, 0)
        layout.setSpacing(4)
        layout.addWidget(_header("PROPERTIES"))

        form = QFormLayout()
        form.setSpacing(4)
        form.setContentsMargins(0, 0, 0, 0)
        form.setLabelAlignment(Qt.AlignmentFlag.AlignRight)

        self._name  = QLineEdit()
        self._name.setPlaceholderText("sprite name")
        self._group = QLineEdit()
        self._group.setPlaceholderText("animation group (optional)")
        self._x = QSpinBox(); self._x.setRange(0, 99999)
        self._y = QSpinBox(); self._y.setRange(0, 99999)
        self._w = QSpinBox(); self._w.setRange(1, 99999)
        self._h = QSpinBox(); self._h.setRange(1, 99999)

        form.addRow("Name:",  self._name)
        form.addRow("Group:", self._group)
        form.addRow("X:",     self._x)
        form.addRow("Y:",     self._y)
        form.addRow("W:",     self._w)
        form.addRow("H:",     self._h)
        layout.addLayout(form)

        self._region: Optional[SpriteRegion] = None
        self._blocked = False

        # Connect all fields
        self._name.editingFinished.connect(self._push)
        self._group.editingFinished.connect(self._push)
        for sb in (self._x, self._y, self._w, self._h):
            sb.valueChanged.connect(self._push)

        self.setEnabled(False)

    def load(self, region: Optional[SpriteRegion]) -> None:
        self._region  = region
        self._blocked = True
        if region is None:
            self.setEnabled(False)
            self._name.clear()
            self._group.clear()
            for sb in (self._x, self._y, self._w, self._h):
                sb.setValue(0)
        else:
            self.setEnabled(True)
            self._name.setText(region.name)
            self._group.setText(region.group)
            self._x.setValue(region.x)
            self._y.setValue(region.y)
            self._w.setValue(region.w)
            self._h.setValue(region.h)
        self._blocked = False

    def _push(self) -> None:
        if self._blocked or self._region is None:
            return
        self._region.name  = self._name.text().strip() or self._region.name
        self._region.group = self._group.text().strip()
        self._region.x     = self._x.value()
        self._region.y     = self._y.value()
        self._region.w     = self._w.value()
        self._region.h     = self._h.value()
        self.region_changed.emit(self._region)


# ── 9.4  Animation group panel ────────────────────────────────────────────────

class _AnimGroupPanel(QWidget):
    """
    Manages AnimGroups.
    Emits `groups_changed()` on add/remove/rename/reorder.
    """
    groups_changed: pyqtSignal = pyqtSignal()

    def __init__(self, parent: QWidget | None = None) -> None:
        super().__init__(parent)
        layout = QVBoxLayout(self)
        layout.setContentsMargins(0, 0, 0, 0)
        layout.setSpacing(4)

        # Header + add button
        hdr = QHBoxLayout()
        hdr.addWidget(_header("ANIMATIONS"))
        hdr.addStretch()
        add_btn = _icon_btn("+", "Add animation group")
        add_btn.clicked.connect(self._add_group)
        hdr.addWidget(add_btn)
        layout.addLayout(hdr)

        # Group list
        self._group_list = QListWidget()
        self._group_list.setMaximumHeight(120)
        self._group_list.currentRowChanged.connect(self._on_group_selected)
        layout.addWidget(self._group_list)

        # Frame list (for selected group)
        frame_hdr = QHBoxLayout()
        frame_hdr.addWidget(_header("FRAMES"))
        frame_hdr.addStretch()
        self._add_frame_btn = _icon_btn("+", "Add selected sprite to group")
        self._rem_frame_btn = _icon_btn("-", "Remove frame from group")
        self._add_frame_btn.setEnabled(False)
        self._rem_frame_btn.setEnabled(False)
        frame_hdr.addWidget(self._add_frame_btn)
        frame_hdr.addWidget(self._rem_frame_btn)
        layout.addLayout(frame_hdr)

        self._frame_list = QListWidget()
        self._frame_list.setMaximumHeight(100)
        layout.addWidget(self._frame_list)

        # Control row
        ctrl = QHBoxLayout()
        self._del_group_btn = QPushButton("Delete Group")
        self._del_group_btn.setEnabled(False)
        self._del_group_btn.clicked.connect(self._delete_group)
        ctrl.addWidget(self._del_group_btn)
        ctrl.addStretch()
        layout.addLayout(ctrl)

        self._groups:          list[AnimGroup]   = []
        self._available_sprites: list[str]       = []
        self._cur_group:       Optional[AnimGroup] = None

        self._add_frame_btn.clicked.connect(self._add_frame)
        self._rem_frame_btn.clicked.connect(self._remove_frame)

    def refresh_groups(self, groups: list[AnimGroup]) -> None:
        self._groups = groups
        with QSignalBlocker(self._group_list):
            self._group_list.clear()
            for g in groups:
                self._group_list.addItem(
                    f"{g.name}  ({len(g.frames)} frames)"
                )
        self._cur_group = None
        self._frame_list.clear()
        self._del_group_btn.setEnabled(False)
        self._add_frame_btn.setEnabled(False)
        self._rem_frame_btn.setEnabled(False)

    def set_available_sprites(self, names: list[str]) -> None:
        self._available_sprites = names

    def _on_group_selected(self, row: int) -> None:
        if row < 0 or row >= len(self._groups):
            self._cur_group = None
            self._frame_list.clear()
            self._del_group_btn.setEnabled(False)
            self._add_frame_btn.setEnabled(False)
            return
        self._cur_group = self._groups[row]
        self._frame_list.clear()
        for f in self._cur_group.frames:
            self._frame_list.addItem(f)
        self._del_group_btn.setEnabled(True)
        self._add_frame_btn.setEnabled(bool(self._available_sprites))
        self._rem_frame_btn.setEnabled(bool(self._cur_group.frames))

    def _add_group(self) -> None:
        name, ok = QInputDialog.getText(self, "New Animation",
                                        "Animation name:", text="anim")
        if not ok or not name.strip():
            return
        g = AnimGroup(name.strip())
        self._groups.append(g)
        self.refresh_groups(self._groups)
        self.groups_changed.emit()

    def _delete_group(self) -> None:
        if self._cur_group is None:
            return
        self._groups.remove(self._cur_group)
        self.refresh_groups(self._groups)
        self.groups_changed.emit()

    def _add_frame(self) -> None:
        if not self._cur_group or not self._available_sprites:
            return
        name, ok = QInputDialog.getItem(
            self, "Add Frame", "Choose sprite:",
            self._available_sprites, editable=False
        )
        if not ok:
            return
        self._cur_group.frames.append(name)
        self._frame_list.addItem(name)
        self._rem_frame_btn.setEnabled(True)
        self.groups_changed.emit()

    def _remove_frame(self) -> None:
        if not self._cur_group:
            return
        row = self._frame_list.currentRow()
        if row < 0:
            return
        self._cur_group.frames.pop(row)
        self._frame_list.takeItem(row)
        self.groups_changed.emit()


# ── 9.5  SidebarWidget ───────────────────────────────────────────────────────

class SidebarWidget(QWidget):
    """
    Full right-side panel.

    Public signals for MainWindow:
      sprite_list_selection_changed(int)   — row index in sprite list
      properties_changed(SpriteRegion)     — user edited a property field
      groups_changed()                     — anim groups modified
    """
    sprite_list_selection_changed: pyqtSignal = pyqtSignal(int)
    properties_changed:            pyqtSignal = pyqtSignal(object)
    groups_changed:                pyqtSignal = pyqtSignal()

    def __init__(self, parent: QWidget | None = None) -> None:
        super().__init__(parent)
        self.setFixedWidth(290)
        self.setStyleSheet(_SIDEBAR_STYLE)

        root_layout = QVBoxLayout(self)
        root_layout.setContentsMargins(8, 8, 8, 8)
        root_layout.setSpacing(8)

        # Splitter: sprite list on top, properties + anims below
        splitter = QSplitter(Qt.Orientation.Vertical)
        splitter.setChildrenCollapsible(False)

        # ── Sprite list panel ─────────────────────────────────────────────
        self._sprite_panel = _SpriteListPanel()
        self._sprite_panel.selection_changed.connect(
            lambda row: self.sprite_list_selection_changed.emit(int(row))
        )
        splitter.addWidget(self._sprite_panel)

        # ── Bottom scroll area (properties + anim groups) ─────────────────
        bottom = QWidget()
        bottom_layout = QVBoxLayout(bottom)
        bottom_layout.setContentsMargins(0, 0, 0, 0)
        bottom_layout.setSpacing(8)

        # Properties
        self._props = _PropertiesPanel()
        self._props.region_changed.connect(self.properties_changed)
        bottom_layout.addWidget(self._props)

        # Animation groups
        self._anim = _AnimGroupPanel()
        self._anim.groups_changed.connect(self.groups_changed)
        bottom_layout.addWidget(self._anim)
        bottom_layout.addStretch()

        scroll = QScrollArea()
        scroll.setWidget(bottom)
        scroll.setWidgetResizable(True)
        scroll.setHorizontalScrollBarPolicy(Qt.ScrollBarPolicy.ScrollBarAlwaysOff)
        splitter.addWidget(scroll)

        splitter.setSizes([250, 400])
        root_layout.addWidget(splitter)

    # ── 9.3  Public API (called by MainWindow) ────────────────────────────

    def refresh_sprites(self, regions: list[SpriteRegion]) -> None:
        """Rebuild sprite list from regions. Preserves current selection."""
        self._sprite_panel.refresh(regions)
        self._anim.set_available_sprites([r.name for r in regions])

    def select_sprite(self, region: Optional[SpriteRegion],
                      regions: list[SpriteRegion]) -> None:
        """Update list highlight + properties when canvas selection changes."""
        self._sprite_panel.highlight(region, regions)
        self._props.load(region)

    def refresh_groups(self, groups: list[AnimGroup]) -> None:
        self._anim.refresh_groups(groups)
