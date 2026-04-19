"""
Core Grid Detector — Uniform-grid slicing for regular tilesets.

Tasks:
  5.1  Grid computation  (tile W×H + offset + padding)
  5.2  Empty-cell skip   (cells whose pixels are all background)
  5.3  Auto-naming       (row_col format, e.g. "tile_r00_c03")
"""
from __future__ import annotations

import numpy as np
from dataclasses import dataclass
from typing import Optional

from core.models import SpriteRegion
from core.cv_detector import (
    detect_background_color,
    build_foreground_mask,
    has_alpha,
)


# ─── Config ───────────────────────────────────────────────────────────────────

@dataclass
class GridConfig:
    """All parameters for a uniform-grid slice."""
    tile_w: int              # width of each tile in pixels
    tile_h: int              # height of each tile in pixels
    offset_x: int = 0        # left margin before first column
    offset_y: int = 0        # top margin before first row
    padding: int = 0         # gap between tiles (same H and V)
    padding_h: int = 0       # horizontal gap (overrides padding if > 0)
    padding_v: int = 0       # vertical gap   (overrides padding if > 0)
    skip_empty: bool = True  # skip cells whose pixels are all background
    tolerance: int = 15      # colour tolerance for "empty" detection
    name_prefix: str = "tile" # prefix for auto-names

    @property
    def gap_h(self) -> int:
        return self.padding_h if self.padding_h > 0 else self.padding

    @property
    def gap_v(self) -> int:
        return self.padding_v if self.padding_v > 0 else self.padding


# ─── Task 5.1: Grid computation ──────────────────────────────────────────────

def compute_grid(
    image_w: int,
    image_h: int,
    cfg: GridConfig,
) -> list[tuple[int, int, int, int, int, int]]:
    """
    Return a list of (x, y, w, h, row, col) for every grid cell
    that fits within the image bounds.

    Does NOT check for empty cells — that is done separately.
    """
    cells: list[tuple[int, int, int, int, int, int]] = []
    step_x = cfg.tile_w + cfg.gap_h
    step_y = cfg.tile_h + cfg.gap_v

    row = 0
    y = cfg.offset_y
    while y + cfg.tile_h <= image_h:
        col = 0
        x = cfg.offset_x
        while x + cfg.tile_w <= image_w:
            cells.append((x, y, cfg.tile_w, cfg.tile_h, row, col))
            x += step_x
            col += 1
        y += step_y
        row += 1

    return cells


# ─── Task 5.2: Empty-cell detection ──────────────────────────────────────────

def is_cell_empty(
    image: np.ndarray,
    x: int, y: int, w: int, h: int,
    bg_color_rgb: Optional[tuple[int, int, int]],
    tolerance: int,
) -> bool:
    """
    Return True if every pixel in the cell's crop is background
    (i.e. the foreground mask is all-zero).

    NOTE: bg_color_rgb must come from the FULL image corners, not the crop.
    Pass None to auto-detect from the full image before calling.
    """
    crop = image[y:y + h, x:x + w]
    if crop.size == 0:
        return True
    mask = build_foreground_mask(crop, bg_color_rgb, tolerance)
    return bool(mask.max() == 0)


# ─── Task 5.3: Auto-naming ────────────────────────────────────────────────────

def cell_name(
    row: int,
    col: int,
    prefix: str,
    num_rows: int,
    num_cols: int,
) -> str:
    """
    Generate a name like "tile_r02_c05".
    Padding width is based on the total grid dimensions.
    """
    row_digits = max(2, len(str(num_rows - 1)))
    col_digits = max(2, len(str(num_cols - 1)))
    return f"{prefix}_r{row:0{row_digits}d}_c{col:0{col_digits}d}"


# ─── Public API ───────────────────────────────────────────────────────────────

class GridDetector:
    """
    Slice a sprite sheet into a uniform grid.

    Usage:
        detector = GridDetector(GridConfig(tile_w=16, tile_h=16))
        detector.load_array(image_array)
        regions = detector.detect()
    """

    def __init__(self, config: GridConfig) -> None:
        self.config = config
        self._image: Optional[np.ndarray] = None
        self._bg_color: Optional[tuple[int, int, int]] = None

    def load_image(self, path: str) -> tuple[int, int]:
        """Load image from disk. Returns (width, height)."""
        import cv2
        img = cv2.imread(str(path), cv2.IMREAD_UNCHANGED)
        if img is None:
            raise FileNotFoundError(f"Cannot load image: {path}")
        self._image = img
        self._bg_color = None
        h, w = img.shape[:2]
        return w, h

    def load_array(self, array: np.ndarray) -> None:
        self._image = array.copy()
        self._bg_color = None

    def set_bg_color(self, r: int, g: int, b: int) -> None:
        self._bg_color = (r, g, b)

    def get_bg_color(self) -> tuple[int, int, int]:
        if self._image is None:
            raise RuntimeError("No image loaded")
        if self._bg_color is None:
            self._bg_color = detect_background_color(self._image)
        return self._bg_color

    def grid_dimensions(self) -> tuple[int, int]:
        """Return (num_cols, num_rows) based on image size and config."""
        if self._image is None:
            raise RuntimeError("No image loaded")
        h, w = self._image.shape[:2]
        cells = compute_grid(w, h, self.config)
        if not cells:
            return 0, 0
        max_row = max(c[4] for c in cells) + 1
        max_col = max(c[5] for c in cells) + 1
        return max_col, max_row   # (cols, rows)

    def detect(self) -> list[SpriteRegion]:
        """
        Slice the image into grid cells and return non-empty SpriteRegions.
        """
        if self._image is None:
            raise RuntimeError("No image loaded. Call load_image() or load_array() first.")

        cfg = self.config
        h, w = self._image.shape[:2]

        # 5.1 compute all cells
        all_cells = compute_grid(w, h, cfg)
        if not all_cells:
            return []

        num_rows = max(c[4] for c in all_cells) + 1
        num_cols = max(c[5] for c in all_cells) + 1

        bg = self.get_bg_color() if cfg.skip_empty else None

        regions: list[SpriteRegion] = []
        for (cx, cy, cw, ch, row, col) in all_cells:
            # 5.2 skip empty cells
            if cfg.skip_empty and is_cell_empty(
                self._image, cx, cy, cw, ch, bg, cfg.tolerance
            ):
                continue

            # 5.3 auto-name
            name = cell_name(row, col, cfg.name_prefix, num_rows, num_cols)
            regions.append(SpriteRegion(name, cx, cy, cw, ch))

        return regions
