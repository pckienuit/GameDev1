"""Phase 9 headless structural verification."""
import sys
sys.path.insert(0, 'd:/GameDev1/tools/sprite-cutter')

from PyQt6.QtWidgets import QApplication
app = QApplication.instance() or QApplication(sys.argv)

from desktop.sidebar import SidebarWidget, _SpriteListPanel, _PropertiesPanel, _AnimGroupPanel
from core.models import SpriteRegion, AnimGroup
import inspect

# 9.1 Sprite list panel
panel = _SpriteListPanel()
regions = [
    SpriteRegion("idle",   245, 154, 16, 26, "idle_anim"),
    SpriteRegion("walk_1", 275, 154, 16, 26, "walk_anim"),
    SpriteRegion("walk_2", 305, 154, 16, 26, "walk_anim"),
]
panel.refresh(regions)
assert panel._list.count() == 3
print("OK 9.1 Sprite list: 3 items loaded")

# Count label
assert panel._count_lbl.text() == "3"
print("OK 9.1 Count label: '3'")

# 9.2 Properties panel
props = _PropertiesPanel()
assert not props.isEnabled()     # disabled when no region
props.load(regions[0])
assert props.isEnabled()
assert props._name.text()  == "idle"
assert props._x.value()    == 245
assert props._y.value()    == 154
assert props._w.value()    == 16
assert props._h.value()    == 26
assert props._group.text() == "idle_anim"
print("OK 9.2 Properties panel loads region fields correctly")

# Simulate editing x
props._x.setValue(300)
props._push()
assert regions[0].x == 300
print("OK 9.2 Editing X field pushes to model: x =", regions[0].x)

props.load(None)
assert not props.isEnabled()
print("OK 9.2 Cleared when region=None")

# 9.4 Animation group panel
anim = _AnimGroupPanel()
groups = [
    AnimGroup("idle_anim",  ["idle"],                      0.4),
    AnimGroup("walk_anim",  ["walk_1", "walk_2"],          0.15),
]
anim.refresh_groups(groups)
assert anim._group_list.count() == 2
print("OK 9.4 Group list: 2 groups loaded")

anim.set_available_sprites(["idle", "walk_1", "walk_2"])
assert len(anim._available_sprites) == 3
print("OK 9.4 Available sprites set")

# 9.5 SidebarWidget
sidebar = SidebarWidget()
assert sidebar.width() == 290 or sidebar.maximumWidth() >= 290
print("OK 9.5 SidebarWidget created")

# Signals exist
for sig in ('sprite_list_selection_changed', 'properties_changed', 'groups_changed'):
    assert hasattr(SidebarWidget, sig), f"Missing signal: {sig}"
print("OK 9.5 Signals: sprite_list_selection_changed / properties_changed / groups_changed")

# Public API
for m in ('refresh_sprites', 'select_sprite', 'refresh_groups'):
    assert hasattr(SidebarWidget, m), f"Missing method: {m}"
print("OK 9.5 Public API: refresh_sprites / select_sprite / refresh_groups")

# 9.3 Bidirectional sync test
sidebar.refresh_sprites(regions)
sidebar.select_sprite(regions[1], regions)
highlighted_row = sidebar._sprite_panel._list.currentRow()
assert highlighted_row == 1, f"Expected row 1, got {highlighted_row}"
print("OK 9.3 select_sprite highlights correct row:", highlighted_row)

sidebar.select_sprite(None, regions)
assert sidebar._sprite_panel._list.currentRow() == -1
print("OK 9.3 select_sprite(None) clears highlight")

print()
print("Phase 9: ALL STRUCTURAL CHECKS PASSED")
