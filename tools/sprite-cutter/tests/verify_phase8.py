"""Phase 8 headless structural verification."""
import sys
sys.path.insert(0, 'd:/GameDev1/tools/sprite-cutter')

import inspect
from PyQt6.QtWidgets import QApplication
app = QApplication.instance() or QApplication(sys.argv)

from desktop.sprite_canvas import SpriteCanvas, _Handle, _handle_positions
from desktop.toolbar import EditorMode
from core.models import SpriteRegion
from PyQt6.QtCore import QRectF, QPointF

# 8.1-8.6 All required methods
for method in (
    'set_regions', 'regions', 'clear_regions', 'add_region',   # 8.1 data
    'delete_selected', 'select_region',                          # 8.5 delete
    'set_mode', 'undo',                                          # 8.2/8.3
):
    assert hasattr(SpriteCanvas, method), f"Missing: {method}"
print("OK 8.1-8.6 SpriteCanvas public API")

# 8.2 Draw mode methods in source
src = inspect.getsource(SpriteCanvas)
for fn in ('_on_draw_press', '_on_draw_move', '_on_draw_release', '_finish_draw'):
    assert fn in src, f"Missing: {fn}"
print("OK 8.2 Draw mode methods")

# 8.3 Select methods
for fn in ('_on_select_press', '_on_select_move', '_on_select_release', '_region_at'):
    assert fn in src, f"Missing: {fn}"
print("OK 8.3 Select mode methods")

# 8.4 Handle resize
for fn in ('_handle_at', '_apply_resize', '_paint_handles'):
    assert fn in src, f"Missing: {fn}"
assert len(_Handle) == 8, f"Expected 8 handles, got {len(_Handle)}"
print(f"OK 8.4 Resize handles: {[h.name for h in _Handle]}")

# handle_positions returns 8 points
pts = _handle_positions(QRectF(0, 0, 100, 100))
assert len(pts) == 8
print("OK 8.4 _handle_positions returns 8 points")

# 8.5 Delete key in source
assert 'Key_Delete' in src and 'Key_Backspace' in src
assert 'Key_Escape' in src
print("OK 8.5 Delete (Del/Backspace) + Escape deselect")

# Signals
for sig in ('region_added', 'region_selected', 'region_deleted', 'regions_changed'):
    assert hasattr(SpriteCanvas, sig), f"Missing signal: {sig}"
print("OK 8.6 Signals: region_added/selected/deleted/changed")

# Undo stack
canvas = SpriteCanvas()
r1 = SpriteRegion("idle", 10, 10, 20, 20)
r2 = SpriteRegion("walk", 40, 10, 20, 20)
canvas.add_region(r1)
canvas.add_region(r2)
assert len(canvas.regions()) == 2
canvas.select_region(r1)
canvas.delete_selected()
assert len(canvas.regions()) == 1
canvas.undo()
assert len(canvas.regions()) == 2, f"Undo failed: {len(canvas.regions())} regions"
print("OK 8.3/8.5/Undo cycle: add->delete->undo = 2 regions restored")

# _draw_rect_image normalisation
from PyQt6.QtWidgets import QApplication as _A
canvas2 = SpriteCanvas()
canvas2._img_w = 100; canvas2._img_h = 100
canvas2._draw_start = QPointF(30, 20)
canvas2._draw_end   = QPointF(10, 5)   # inverted drag
result = canvas2._draw_rect_image()
assert result is not None
rx, ry, rw, rh = result
assert rx == 10 and ry == 5   # normalised to min
assert rw == 20 and rh == 15
print(f"OK 8.2 _draw_rect_image normalises inverted drag: ({rx},{ry}) {rw}×{rh}")

print()
print("Phase 8: ALL STRUCTURAL CHECKS PASSED")
