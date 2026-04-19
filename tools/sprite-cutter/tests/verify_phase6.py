"""Phase 6 headless structural verification."""
import sys
sys.path.insert(0, 'd:/GameDev1/tools/sprite-cutter')

from desktop.toolbar import SpriteCutterToolbar, EditorMode
from desktop.main_window import MainWindow
from desktop.main import main as desktop_main

# 6.2 All 4 modes exist
modes = list(EditorMode)
assert len(modes) == 4
names = [m.name for m in modes]
for expected in ('SELECT', 'DRAW', 'AI', 'GRID'):
    assert expected in names, f"Missing mode: {expected}"
print("OK 6.2 Toolbar modes:", names)

# 6.1/6.4 MainWindow public API
for method in ('set_image_path', 'set_sprite_count', 'set_zoom',
               'set_cursor_coords', 'clear_cursor_coords'):
    assert hasattr(MainWindow, method), f"Missing: {method}"
print("OK 6.1/6.4 MainWindow public API")

# 6.2 Toolbar signal
assert hasattr(SpriteCutterToolbar, 'mode_changed')
print("OK 6.2 Toolbar signal mode_changed")

# Menu action constants exist in class methods
import inspect
src = inspect.getsource(MainWindow._setup_menu)
for item in ('Open Image', 'Export JSON', 'Import JSON', 'Quit'):
    assert item in src, f"Missing menu item: {item}"
print("OK 6.1 Menu items: Open/Export/Import/Quit")

# Status bar setup
src_sb = inspect.getsource(MainWindow._setup_statusbar)
for field in ('coords', 'zoom', 'count', 'mode', 'image'):
    assert field in src_sb, f"Missing status field: {field}"
print("OK 6.4 Status bar fields")

# Shortcuts 1-4 in source
src_sc = inspect.getsource(MainWindow._setup_shortcuts)
for key in ('Key_1', 'Key_2', 'Key_3', 'Key_4'):
    assert key in src_sc, f"Missing shortcut: {key}"
print("OK 6.5 Keyboard shortcuts 1-4")

print()
print("Phase 6: ALL STRUCTURAL CHECKS PASSED")
