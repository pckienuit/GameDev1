"""Phase 7 headless structural verification."""
import sys
sys.path.insert(0, 'd:/GameDev1/tools/sprite-cutter')

from desktop.image_viewer import (
    ImageViewer, ZOOM_MIN, ZOOM_MAX, ZOOM_STEP, GRID_ZOOM_THRESH
)
from pathlib import Path

# 7.2 Zoom constants sane
assert ZOOM_MIN == 0.10, f"ZOOM_MIN={ZOOM_MIN}"
assert ZOOM_MAX == 32.0, f"ZOOM_MAX={ZOOM_MAX}"
assert GRID_ZOOM_THRESH == 4.0
print("OK 7.2 Zoom constants")

# Verify all required methods exist
for method in (
    'load_image', 'load_pixmap', 'has_image', 'image_size',   # 7.1
    'zoom_factor', 'set_zoom', 'zoom_in', 'zoom_out',         # 7.2
    'fit_in_view', 'zoom_to_100',
    'cursor_image_pos',                                         # 7.5
):
    assert hasattr(ImageViewer, method), f"Missing method: {method}"
print("OK 7.1-7.5 ImageViewer public API")

# Verify signals declared
assert hasattr(ImageViewer, 'pixel_hovered')
assert hasattr(ImageViewer, 'pixel_left_image')
assert hasattr(ImageViewer, 'zoom_changed')
print("OK 7.5 Signals: pixel_hovered, pixel_left_image, zoom_changed")

# Verify paint methods
import inspect
src = inspect.getsource(ImageViewer)
assert '_draw_checker'     in src,  "Missing _draw_checker"
assert '_draw_pixel_grid'  in src,  "Missing _draw_pixel_grid"
assert '_draw_crosshair'   in src,  "Missing _draw_crosshair"
print("OK 7.1/7.4/7.5 Paint methods: checker, grid, crosshair")

# Verify coordinate conversion
assert '_widget_to_image' in src and '_image_to_widget' in src
print("OK 7.5 Coordinate converters")

# Verify pan triggers
assert 'MiddleButton'  in src, "Missing middle-click pan"
assert 'Key_Space'     in src, "Missing space+drag pan"
print("OK 7.3 Pan triggers: middle-click + Space+drag")

# Verify grid threshold in paintEvent
assert 'GRID_ZOOM_THRESH' in src
print("OK 7.4 Grid auto-show at zoom threshold")

# Verify clamping
assert '_clamp_offset' in src
print("OK 7.2/7.3 Offset clamping")

# Real load test with mario.png
mario = Path('d:/GameDev1/Project1/assets/mario.png')
if mario.exists():
    from PyQt6.QtWidgets import QApplication
    import sys as _sys
    _app = QApplication.instance() or QApplication(_sys.argv)
    viewer = ImageViewer()
    ok = viewer.load_image(mario)
    assert ok, "Failed to load mario.png"
    w, h = viewer.image_size()
    assert w > 0 and h > 0, f"Bad size: {w}x{h}"
    assert viewer.has_image()
    assert viewer.zoom_factor() > 0
    print(f"OK 7.1 load_image(mario.png) -> {w}x{h} px, zoom={viewer.zoom_factor():.2f}")

print()
print("Phase 7: ALL STRUCTURAL CHECKS PASSED")
print("Run 'python -m desktop.main' to verify zoom/pan visually.")
