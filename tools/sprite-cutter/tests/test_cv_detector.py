"""
Unit tests for core/cv_detector.py — Phase 4 Verification
Run: pytest tests/test_cv_detector.py -v
"""
from __future__ import annotations
import sys
import os
import numpy as np
import cv2
import pytest

sys.path.insert(0, os.path.join(os.path.dirname(__file__), ".."))
from core.cv_detector import (
    CVDetector, CVDetectorConfig,
    detect_background_color, has_alpha,
    build_foreground_mask,
    _filter_rects, _sort_rects, _merge_rects, _rect_gap,
)
from core.models import SpriteRegion


# ─── Helpers: synthetic test images ──────────────────────────────────────────

def white_canvas(w: int, h: int) -> np.ndarray:
    """BGR image, white background."""
    img = np.full((h, w, 3), 255, dtype=np.uint8)
    return img


def draw_rect(img: np.ndarray, x: int, y: int, w: int, h: int,
              color: tuple[int, int, int] = (255, 0, 0)) -> None:
    """Fill a rectangle with BGR color."""
    img[y:y+h, x:x+w] = color[::-1]   # RGB → BGR


def make_simple_sheet() -> np.ndarray:
    """
    White 200×200 canvas with 3 clearly separated colored rectangles:
      Rect A (red):   x=10, y=10, w=20, h=20
      Rect B (green): x=60, y=10, w=30, h=25
      Rect C (blue):  x=10, y=60, w=25, h=30
    """
    img = white_canvas(200, 200)
    draw_rect(img, 10, 10, 20, 20, (255, 0, 0))    # red
    draw_rect(img, 60, 10, 30, 25, (0, 255, 0))    # green
    draw_rect(img, 10, 60, 25, 30, (0, 0, 255))    # blue
    return img


def make_alpha_sheet() -> np.ndarray:
    """BGRA image: transparent bg, two opaque colored rects."""
    img = np.zeros((100, 100, 4), dtype=np.uint8)   # fully transparent
    img[10:30, 10:30] = [255, 0, 0, 255]            # blue rect, fully opaque
    img[50:70, 50:70] = [0, 255, 0, 255]            # green rect, fully opaque
    return img


# ─── Task 4.1: Background detection ─────────────────────────────────────────

class TestBackgroundDetection:

    def test_white_bg_detected(self):
        img = white_canvas(100, 100)
        bg = detect_background_color(img)
        assert bg == (255, 255, 255)

    def test_colored_bg_detected(self):
        """Magenta background (common in game sprites)."""
        img = np.full((50, 50, 3), [255, 0, 255], dtype=np.uint8)  # BGR magenta
        bg = detect_background_color(img)
        # BGR magenta → RGB (255, 0, 255)
        assert bg == (255, 0, 255)

    def test_corners_sampled_correctly(self):
        """Most-common corner colour wins."""
        img = white_canvas(100, 100)
        img[0, 0] = [0, 0, 0]          # one black corner
        bg = detect_background_color(img)
        assert bg == (255, 255, 255)   # 3 white corners win

    def test_has_alpha_true(self):
        img = np.zeros((10, 10, 4), dtype=np.uint8)
        assert has_alpha(img) is True

    def test_has_alpha_false(self):
        img = np.zeros((10, 10, 3), dtype=np.uint8)
        assert has_alpha(img) is False


# ─── Task 4.2: Foreground mask ───────────────────────────────────────────────

class TestForegroundMask:

    def test_mask_shape(self):
        img = white_canvas(100, 80)
        draw_rect(img, 10, 10, 20, 20)
        mask = build_foreground_mask(img, (255, 255, 255), tolerance=15)
        assert mask.shape == (80, 100)

    def test_white_bg_pixels_are_zero(self):
        img = white_canvas(50, 50)
        mask = build_foreground_mask(img, (255, 255, 255), tolerance=15)
        assert mask.max() == 0           # all background

    def test_foreground_rect_is_255(self):
        img = white_canvas(50, 50)
        draw_rect(img, 10, 10, 20, 20, (255, 0, 0))  # red rect
        mask = build_foreground_mask(img, (255, 255, 255), tolerance=15)
        assert mask[20, 20] == 255       # inside red rect → foreground

    def test_tolerance_controls_sensitivity(self):
        """Near-white pixel: low tolerance → fg, high tolerance → bg."""
        img = white_canvas(50, 50)
        draw_rect(img, 5, 5, 10, 10, (230, 230, 230))   # very light grey
        # tolerance < 25 → detected as fg
        mask_tight = build_foreground_mask(img, (255, 255, 255), tolerance=10)
        assert mask_tight[10, 10] == 255
        # tolerance >= 25 → treated as bg
        mask_loose = build_foreground_mask(img, (255, 255, 255), tolerance=30)
        assert mask_loose[10, 10] == 0

    def test_alpha_transparent_pixels_are_bg(self):
        img = make_alpha_sheet()
        mask = build_foreground_mask(img, None, tolerance=15)
        assert mask[0, 0] == 0          # transparent corner → background
        assert mask[20, 20] == 255      # opaque rect → foreground

    def test_auto_bg_when_none(self):
        img = white_canvas(50, 50)
        draw_rect(img, 5, 5, 10, 10, (255, 0, 0))
        mask = build_foreground_mask(img, None, tolerance=15)
        assert mask[10, 10] == 255


# ─── Task 4.3: Connected components / detect ─────────────────────────────────

class TestDetectRects:

    def _detect(self, img, **kwargs) -> list[SpriteRegion]:
        cfg = CVDetectorConfig(**kwargs)
        d = CVDetector(cfg)
        d.load_array(img)
        return d.detect()

    def test_three_rects_detected(self):
        img = make_simple_sheet()
        regions = self._detect(img, tolerance=15, min_area=16)
        assert len(regions) == 3

    def test_rect_coords_correct(self):
        """Verify the detected bounding box matches what we drew."""
        img = white_canvas(100, 100)
        draw_rect(img, 10, 10, 20, 20, (255, 0, 0))
        regions = self._detect(img, tolerance=15, min_area=16)
        assert len(regions) == 1
        r = regions[0]
        assert r.x == 10 and r.y == 10
        assert r.w == 20 and r.h == 20

    def test_rect_b_coords_correct(self):
        img = white_canvas(200, 100)
        draw_rect(img, 60, 10, 30, 25, (0, 200, 0))
        regions = self._detect(img, tolerance=15, min_area=4)
        assert len(regions) == 1
        assert regions[0].x == 60 and regions[0].y == 10

    def test_auto_names_assigned(self):
        img = make_simple_sheet()
        regions = self._detect(img, name_prefix="sp")
        names = [r.name for r in regions]
        assert "sp_001" in names
        assert "sp_002" in names
        assert "sp_003" in names


# ─── Task 4.4: Filter + sort ─────────────────────────────────────────────────

class TestFilterSort:

    def test_min_area_filters_small_rects(self):
        img = white_canvas(100, 100)
        draw_rect(img, 5,  5,  2, 2, (255, 0, 0))   # 4 px² — tiny
        draw_rect(img, 20, 20, 20, 20, (0, 255, 0)) # 400 px² — large
        cfg = CVDetectorConfig(tolerance=15, min_area=16)
        d = CVDetector(cfg)
        d.load_array(img)
        regions = d.detect()
        assert len(regions) == 1
        assert regions[0].w == 20

    def test_sort_top_left_first(self):
        """Row-major: top rect first, then bottom rect."""
        img = white_canvas(200, 200)
        draw_rect(img, 10, 100, 20, 20, (255, 0, 0))   # bottom
        draw_rect(img, 10, 10,  20, 20, (0, 255, 0))   # top
        cfg = CVDetectorConfig(tolerance=15, min_area=4)
        d = CVDetector(cfg)
        d.load_array(img)
        regions = d.detect()
        assert regions[0].y < regions[1].y          # top first

    def test_sort_left_before_right_same_row(self):
        img = white_canvas(200, 100)
        draw_rect(img, 120, 10, 20, 20, (255, 0, 0))  # right
        draw_rect(img, 10,  10, 20, 20, (0, 255, 0))  # left
        cfg = CVDetectorConfig(tolerance=15, min_area=4)
        d = CVDetector(cfg)
        d.load_array(img)
        regions = d.detect()
        assert regions[0].x < regions[1].x          # left first

    def test_filter_rects_function(self):
        rects = [(0, 0, 3, 3), (0, 0, 5, 5), (0, 0, 10, 10)]
        result = _filter_rects(rects, min_area=16)
        assert (0, 0, 3, 3) not in result   # 9 px² filtered
        assert len(result) == 2

    def test_sort_rects_function(self):
        rects = [(100, 100, 10, 10), (10, 10, 10, 10), (10, 50, 10, 10)]
        sorted_r = _sort_rects(rects)
        assert sorted_r[0] == (10, 10, 10, 10)


# ─── Task 4.5: Merge close boxes ─────────────────────────────────────────────

class TestMergeRects:

    def test_rect_gap_touching(self):
        a = (0, 0, 10, 10)   # right edge at x=10
        b = (10, 0, 10, 10)  # left edge at x=10  → gap = 0
        assert _rect_gap(a, b) == 0

    def test_rect_gap_separated(self):
        a = (0, 0, 10, 10)
        b = (15, 0, 10, 10)  # gap = 15-10 = 5
        assert _rect_gap(a, b) == 5

    def test_rect_gap_overlapping(self):
        a = (0, 0, 20, 20)
        b = (10, 10, 20, 20)  # overlapping
        assert _rect_gap(a, b) == 0

    def test_merge_close_rects(self):
        """Two rects 3px apart should merge when gap=5."""
        rects = [(0, 0, 10, 10), (13, 0, 10, 10)]
        merged = _merge_rects(rects, gap=5)
        assert len(merged) == 1
        x, y, w, h = merged[0]
        assert x == 0 and w == 23

    def test_dont_merge_faraway_rects(self):
        rects = [(0, 0, 10, 10), (50, 0, 10, 10)]
        merged = _merge_rects(rects, gap=5)
        assert len(merged) == 2

    def test_merge_gap_zero_disabled(self):
        rects = [(0, 0, 10, 10), (11, 0, 10, 10)]  # gap=1
        merged = _merge_rects(rects, gap=0)
        assert len(merged) == 2          # unchanged

    def test_merge_via_detector(self):
        """End-to-end: two nearby rects merge into one."""
        img = white_canvas(200, 100)
        draw_rect(img, 10, 10, 20, 20, (255, 0, 0))   # rect A
        draw_rect(img, 33, 10, 20, 20, (0, 0, 255))   # rect B, gap=3px
        cfg = CVDetectorConfig(tolerance=15, min_area=4, merge_gap=5)
        d = CVDetector(cfg)
        d.load_array(img)
        regions = d.detect()
        assert len(regions) == 1   # merged into one

    def test_no_merge_via_detector(self):
        img = white_canvas(200, 100)
        draw_rect(img, 10, 10, 20, 20, (255, 0, 0))
        draw_rect(img, 80, 10, 20, 20, (0, 0, 255))   # far apart
        cfg = CVDetectorConfig(tolerance=15, min_area=4, merge_gap=5)
        d = CVDetector(cfg)
        d.load_array(img)
        regions = d.detect()
        assert len(regions) == 2


# ─── Real sheet smoke-test ────────────────────────────────────────────────────

class TestRealSheet:
    """Smoke-tests against the actual project sprite sheets."""

    MARIO_PNG = r"d:\GameDev1\Project1\assets\mario.png"
    ENEMIES_PNG = r"d:\GameDev1\Project1\assets\enemies.png"

    def _can_load(self, path: str) -> bool:
        return cv2.imread(path, cv2.IMREAD_UNCHANGED) is not None

    def test_mario_detects_many_sprites(self):
        if not self._can_load(self.MARIO_PNG):
            pytest.skip("mario.png not accessible from test environment")
        d = CVDetector(CVDetectorConfig(tolerance=20, min_area=64))
        d.load_image(self.MARIO_PNG)
        regions = d.detect()
        # mario.png is a large sheet — expect at least 50 sprites
        assert len(regions) >= 50, f"Expected ≥50, got {len(regions)}"

    def test_enemies_detects_sprites(self):
        if not self._can_load(self.ENEMIES_PNG):
            pytest.skip("enemies.png not accessible from test environment")
        d = CVDetector(CVDetectorConfig(tolerance=20, min_area=64))
        d.load_image(self.ENEMIES_PNG)
        regions = d.detect()
        assert len(regions) >= 5


# ─── Run directly ─────────────────────────────────────────────────────────────

if __name__ == "__main__":
    pytest.main([__file__, "-v"])
