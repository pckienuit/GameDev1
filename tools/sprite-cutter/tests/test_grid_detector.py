"""
Unit tests for core/grid_detector.py — Phase 5 Verification
Run: pytest tests/test_grid_detector.py -v
"""
from __future__ import annotations
import sys, os
import numpy as np
import pytest

sys.path.insert(0, os.path.join(os.path.dirname(__file__), ".."))
from core.grid_detector import (
    GridDetector, GridConfig,
    compute_grid, is_cell_empty, cell_name,
)
from core.models import SpriteRegion


# ─── Helpers ─────────────────────────────────────────────────────────────────

def white_canvas(w: int, h: int) -> np.ndarray:
    return np.full((h, w, 3), 255, dtype=np.uint8)

def fill_cell(img: np.ndarray, x: int, y: int, w: int, h: int,
              color=(128, 128, 0)) -> None:
    """Fill a rect with BGR color (default: dark cyan, clearly non-white)."""
    img[y:y+h, x:x+w] = color

def make_tileset(
    cols: int, rows: int,
    tile_w: int, tile_h: int,
    filled: set[tuple[int,int]] | None = None,  # (row, col) pairs to fill
    padding: int = 0,
) -> np.ndarray:
    """
    Create a white canvas with a uniform grid.
    Cells listed in `filled` are painted dark cyan; rest remain white (empty).
    `filled=None` means fill every cell.
    """
    gap = padding
    total_w = cols * tile_w + (cols - 1) * gap
    total_h = rows * tile_h + (rows - 1) * gap
    img = white_canvas(total_w, total_h)
    for r in range(rows):
        for c in range(cols):
            if filled is None or (r, c) in filled:
                x = c * (tile_w + gap)
                y = r * (tile_h + gap)
                fill_cell(img, x, y, tile_w, tile_h, (0, 180, 0))  # green-ish
    return img


# ─── Task 5.1: compute_grid ──────────────────────────────────────────────────

class TestComputeGrid:

    def test_simple_2x3_grid(self):
        cfg = GridConfig(tile_w=16, tile_h=16)
        cells = compute_grid(image_w=48, image_h=32, cfg=cfg)
        # 3 cols × 2 rows = 6 cells
        assert len(cells) == 6

    def test_cell_zero_position(self):
        cfg = GridConfig(tile_w=16, tile_h=16)
        cells = compute_grid(64, 32, cfg)
        x, y, w, h, row, col = cells[0]
        assert x == 0 and y == 0 and w == 16 and h == 16
        assert row == 0 and col == 0

    def test_cells_with_offset(self):
        cfg = GridConfig(tile_w=16, tile_h=16, offset_x=4, offset_y=4)
        cells = compute_grid(36, 36, cfg)
        x, y, *_ = cells[0]
        assert x == 4 and y == 4

    def test_cells_with_padding(self):
        """2×2 grid, 16×16 tiles, 2px gap → step=18."""
        cfg = GridConfig(tile_w=16, tile_h=16, padding=2)
        # need 2*16 + 1*2 = 34px wide
        cells = compute_grid(34, 34, cfg)
        assert len(cells) == 4
        # second cell x = 16+2 = 18
        second_in_row = [c for c in cells if c[4] == 0 and c[5] == 1]
        assert second_in_row[0][0] == 18

    def test_asymmetric_padding(self):
        """Horizontal gap = 4, vertical gap = 2."""
        cfg = GridConfig(tile_w=10, tile_h=10, padding_h=4, padding_v=2)
        cells = compute_grid(24, 22, cfg)  # 2 cols × 2 rows
        assert len(cells) == 4
        second_col = [c for c in cells if c[5] == 1]
        assert second_col[0][0] == 14   # x = 10 + 4

    def test_partial_tile_excluded(self):
        """Image width of 20 with tile_w=16 → only 1 col fits."""
        cfg = GridConfig(tile_w=16, tile_h=16)
        cells = compute_grid(20, 16, cfg)
        assert len(cells) == 1

    def test_row_col_indices(self):
        cfg = GridConfig(tile_w=8, tile_h=8)
        cells = compute_grid(24, 16, cfg)
        indices = [(c[4], c[5]) for c in cells]
        assert (0, 0) in indices
        assert (0, 1) in indices
        assert (0, 2) in indices
        assert (1, 0) in indices

    def test_empty_if_tile_larger_than_image(self):
        cfg = GridConfig(tile_w=64, tile_h=64)
        cells = compute_grid(32, 32, cfg)
        assert len(cells) == 0


# ─── Task 5.2: is_cell_empty ─────────────────────────────────────────────────

class TestIsCellEmpty:

    def test_blank_cell_is_empty(self):
        img = white_canvas(50, 50)
        assert is_cell_empty(img, 0, 0, 16, 16, (255, 255, 255), 15) == True

    def test_filled_cell_is_not_empty(self):
        img = white_canvas(50, 50)
        fill_cell(img, 0, 0, 16, 16, (0, 180, 0))  # green in BGR
        assert is_cell_empty(img, 0, 0, 16, 16, (255, 255, 255), 15) == False

    def test_partial_fill_is_not_empty(self):
        """Even 1 fg pixel → not empty."""
        img = white_canvas(50, 50)
        img[5, 5] = [0, 180, 0]    # single green pixel
        assert is_cell_empty(img, 0, 0, 16, 16, (255, 255, 255), 15) == False

    def test_near_bg_color_respects_tolerance(self):
        """Light grey within tolerance → empty."""
        img = white_canvas(50, 50)
        fill_cell(img, 0, 0, 16, 16, (240, 240, 240))  # light grey, diff=15
        # tolerance=20 → diff(255,240)=15 < 20 → light grey is bg
        assert is_cell_empty(img, 0, 0, 16, 16, (255, 255, 255), 20) == True
        # tolerance=10 → diff(255,240)=15 > 10 → light grey is fg
        assert is_cell_empty(img, 0, 0, 16, 16, (255, 255, 255), 10) == False

    def test_out_of_bounds_crop_is_empty(self):
        img = white_canvas(10, 10)
        assert is_cell_empty(img, 5, 5, 20, 20, (255, 255, 255), 15) == True


# ─── Task 5.3: cell_name ─────────────────────────────────────────────────────

class TestCellName:

    def test_basic_name(self):
        assert cell_name(0, 0, "tile", 4, 4) == "tile_r00_c00"

    def test_padded_name(self):
        assert cell_name(2, 5, "tile", 4, 10) == "tile_r02_c05"

    def test_large_grid_name(self):
        """100-row grid → 3-digit row index."""
        name = cell_name(99, 9, "sp", 100, 10)
        assert "r99" in name
        assert "c09" in name

    def test_custom_prefix(self):
        name = cell_name(0, 0, "mario", 2, 2)
        assert name.startswith("mario_")

    def test_single_digit_grid_still_two_digits(self):
        """Even 1×1 grid → at least 2 digits."""
        name = cell_name(0, 0, "t", 1, 1)
        assert "r00" in name and "c00" in name


# ─── Integration: GridDetector.detect() ──────────────────────────────────────

class TestGridDetector:

    def test_detect_all_filled_cells(self):
        """All 6 cells filled. Use skip_empty=False to avoid bg-color ambiguity."""
        img = make_tileset(cols=3, rows=2, tile_w=16, tile_h=16)
        d = GridDetector(GridConfig(tile_w=16, tile_h=16, skip_empty=False))
        d.load_array(img)
        regions = d.detect()
        assert len(regions) == 6

    def test_detect_skip_empty_cells(self):
        """4-cell grid with only 2 filled → detect 2."""
        filled = {(0, 0), (1, 1)}
        img = make_tileset(cols=2, rows=2, tile_w=16, tile_h=16, filled=filled)
        d = GridDetector(GridConfig(tile_w=16, tile_h=16, skip_empty=True))
        d.load_array(img)
        regions = d.detect()
        assert len(regions) == 2

    def test_detect_no_skip_returns_all(self):
        """skip_empty=False → all cells returned including empties."""
        filled = {(0, 0)}
        img = make_tileset(cols=2, rows=2, tile_w=16, tile_h=16, filled=filled)
        d = GridDetector(GridConfig(tile_w=16, tile_h=16, skip_empty=False))
        d.load_array(img)
        regions = d.detect()
        assert len(regions) == 4

    def test_region_coords_correct(self):
        img = make_tileset(cols=3, rows=1, tile_w=16, tile_h=16,
                           filled={(0, 1)})  # only middle cell
        d = GridDetector(GridConfig(tile_w=16, tile_h=16, skip_empty=True))
        d.load_array(img)
        regions = d.detect()
        assert len(regions) == 1
        r = regions[0]
        assert r.x == 16 and r.y == 0
        assert r.w == 16 and r.h == 16

    def test_region_names_have_row_col(self):
        img = make_tileset(cols=3, rows=2, tile_w=16, tile_h=16)
        d = GridDetector(GridConfig(tile_w=16, tile_h=16, skip_empty=False))
        d.load_array(img)
        regions = d.detect()
        names = {r.name for r in regions}
        assert "tile_r00_c00" in names
        assert "tile_r01_c02" in names

    def test_with_padding(self):
        """3 cols, 2px gap → cells at x=0, 18, 36."""
        img = make_tileset(cols=3, rows=1, tile_w=16, tile_h=16, padding=2)
        d = GridDetector(GridConfig(tile_w=16, tile_h=16, padding=2, skip_empty=False))
        d.load_array(img)
        regions = d.detect()
        xs = sorted(r.x for r in regions)
        assert xs == [0, 18, 36]

    def test_with_offset(self):
        """Sheet has 8px margin before first tile."""
        img = white_canvas(56, 24)   # 8 offset + 3*16
        for c in range(3):
            fill_cell(img, 8 + c * 16, 4, 16, 16, (0, 0, 200))
        d = GridDetector(GridConfig(tile_w=16, tile_h=16,
                                    offset_x=8, offset_y=4, skip_empty=True))
        d.load_array(img)
        regions = d.detect()
        assert len(regions) == 3
        assert regions[0].x == 8

    def test_grid_dimensions(self):
        img = make_tileset(cols=4, rows=3, tile_w=8, tile_h=8)
        d = GridDetector(GridConfig(tile_w=8, tile_h=8))
        d.load_array(img)
        cols, rows = d.grid_dimensions()
        assert cols == 4 and rows == 3

    def test_empty_image_returns_empty(self):
        d = GridDetector(GridConfig(tile_w=64, tile_h=64))
        d.load_array(white_canvas(32, 32))  # image too small for 1 cell
        regions = d.detect()
        assert regions == []

    def test_regions_are_sprite_region_instances(self):
        img = make_tileset(cols=2, rows=2, tile_w=8, tile_h=8)
        d = GridDetector(GridConfig(tile_w=8, tile_h=8))
        d.load_array(img)
        regions = d.detect()
        for r in regions:
            assert isinstance(r, SpriteRegion)

    def test_all_regions_pass_validation(self):
        img = make_tileset(cols=3, rows=3, tile_w=16, tile_h=16)
        h, w = img.shape[:2]
        d = GridDetector(GridConfig(tile_w=16, tile_h=16))
        d.load_array(img)
        regions = d.detect()
        for r in regions:
            errors = r.validate(w, h)
            assert errors == [], f"{r.name}: {errors}"


if __name__ == "__main__":
    pytest.main([__file__, "-v"])
