"""
Core CV Detector — OpenCV-based automatic sprite detection.

Tasks:
  4.1  Background detection (corner sampling + alpha channel)
  4.2  Binary mask generation (colour-distance threshold)
  4.3  Connected components → bounding boxes
  4.4  Filter (min area) + sort top-left → bottom-right
  4.5  Merge close bounding boxes (gap < N px)
"""
from __future__ import annotations

import numpy as np
import cv2
from dataclasses import dataclass, field
from pathlib import Path
from typing import Optional

from core.models import SpriteRegion


# ─── Config dataclass ─────────────────────────────────────────────────────────

@dataclass
class CVDetectorConfig:
    """All tunable parameters for the CV pipeline."""
    tolerance: int = 15          # colour distance to background (0-255)
    min_area: int = 16           # minimum bounding-box area in px²
    merge_gap: int = 0           # merge boxes whose gap is ≤ this many px (0 = disabled)
    name_prefix: str = "sprite"  # auto-name prefix: sprite_001, sprite_002 …


# ─── Task 4.1  Background detection ─────────────────────────────────────────

def detect_background_color(image: np.ndarray) -> tuple[int, int, int]:
    """
    Sample the four corners of the image and return the most-common colour
    as (R, G, B).  Ignores the alpha channel even if one is present.
    """
    h, w = image.shape[:2]
    corners = [
        image[0,   0,   :3],
        image[0,   w-1, :3],
        image[h-1, 0,   :3],
        image[h-1, w-1, :3],
    ]
    # Convert from BGR (OpenCV default) to RGB and find mode
    rgb_corners = [(int(c[2]), int(c[1]), int(c[0])) for c in corners]
    from collections import Counter
    most_common = Counter(rgb_corners).most_common(1)[0][0]
    return most_common  # (R, G, B)


def has_alpha(image: np.ndarray) -> bool:
    return image.shape[2] == 4 if len(image.shape) == 3 else False


# ─── Task 4.2  Binary mask ────────────────────────────────────────────────────

def build_foreground_mask(
    image: np.ndarray,
    bg_color_rgb: Optional[tuple[int, int, int]],
    tolerance: int,
) -> np.ndarray:
    """
    Return a uint8 mask where 255 = foreground pixel, 0 = background.

    If the image has an alpha channel, fully-transparent pixels are treated
    as background regardless of bg_color_rgb.
    If bg_color_rgb is None, auto-detect from corners.
    """
    if bg_color_rgb is None:
        bg_color_rgb = detect_background_color(image)

    # Alpha channel shortcut
    if has_alpha(image):
        alpha = image[:, :, 3]
        alpha_mask = (alpha > 0).astype(np.uint8) * 255
        # Also mask out bg-coloured pixels that happen to be opaque
        rgb = image[:, :, :3]
    else:
        rgb = image[:, :, :3]
        alpha_mask = None

    # Colour-distance mask in BGR (OpenCV stores BGR)
    bg_bgr = np.array([bg_color_rgb[2], bg_color_rgb[1], bg_color_rgb[0]],
                      dtype=np.int32)
    rgb_int = rgb.astype(np.int32)
    diff = np.abs(rgb_int - bg_bgr)           # per-channel absolute diff
    max_diff = diff.max(axis=2)                # Chebyshev distance
    color_mask = np.where(max_diff > tolerance, 255, 0).astype(np.uint8)

    if alpha_mask is not None:
        # foreground = opaque AND not bg colour
        return cv2.bitwise_and(alpha_mask, color_mask)
    return color_mask


# ─── Task 4.3  Connected components → bounding boxes ─────────────────────────

def _components_to_rects(mask: np.ndarray) -> list[tuple[int, int, int, int]]:
    """
    Run connectedComponentsWithStats and return raw (x, y, w, h) tuples,
    excluding component 0 (background).
    """
    num_labels, _, stats, _ = cv2.connectedComponentsWithStats(
        mask, connectivity=8, ltype=cv2.CV_32S
    )
    rects: list[tuple[int, int, int, int]] = []
    for label in range(1, num_labels):           # skip label 0 = background
        x = int(stats[label, cv2.CC_STAT_LEFT])
        y = int(stats[label, cv2.CC_STAT_TOP])
        w = int(stats[label, cv2.CC_STAT_WIDTH])
        h = int(stats[label, cv2.CC_STAT_HEIGHT])
        rects.append((x, y, w, h))
    return rects


# ─── Task 4.4  Filter + sort ──────────────────────────────────────────────────

def _filter_rects(
    rects: list[tuple[int, int, int, int]],
    min_area: int,
) -> list[tuple[int, int, int, int]]:
    return [r for r in rects if r[2] * r[3] >= min_area]


def _sort_rects(
    rects: list[tuple[int, int, int, int]],
) -> list[tuple[int, int, int, int]]:
    """Sort top-to-bottom, then left-to-right (row-major order)."""
    return sorted(rects, key=lambda r: (r[1], r[0]))


# ─── Task 4.5  Merge close boxes ─────────────────────────────────────────────

def _rect_gap(
    a: tuple[int, int, int, int],
    b: tuple[int, int, int, int],
) -> int:
    """
    Axis-aligned gap between two rectangles.
    Returns 0 if they touch or overlap.
    """
    ax1, ay1, aw, ah = a
    ax2, ay2 = ax1 + aw, ay1 + ah
    bx1, by1, bw, bh = b
    bx2, by2 = bx1 + bw, by1 + bh

    dx = max(0, max(ax1, bx1) - min(ax2, bx2))
    dy = max(0, max(ay1, by1) - min(ay2, by2))
    return max(dx, dy)


def _merge_rects(
    rects: list[tuple[int, int, int, int]],
    gap: int,
) -> list[tuple[int, int, int, int]]:
    """
    Iteratively merge any two rectangles whose gap ≤ `gap` px into their
    combined bounding box.  Repeats until stable.
    """
    if gap <= 0 or len(rects) < 2:
        return rects

    changed = True
    while changed:
        changed = False
        merged: list[tuple[int, int, int, int]] = []
        used = [False] * len(rects)

        for i, a in enumerate(rects):
            if used[i]:
                continue
            ax1, ay1, aw, ah = a
            ax2, ay2 = ax1 + aw, ay1 + ah

            for j in range(i + 1, len(rects)):
                if used[j]:
                    continue
                if _rect_gap(a, rects[j]) <= gap:
                    bx1, by1, bw, bh = rects[j]
                    bx2, by2 = bx1 + bw, by1 + bh
                    ax1 = min(ax1, bx1)
                    ay1 = min(ay1, by1)
                    ax2 = max(ax2, bx2)
                    ay2 = max(ay2, by2)
                    used[j] = True
                    changed = True

            merged.append((ax1, ay1, ax2 - ax1, ay2 - ay1))
            used[i] = True

        rects = merged

    return rects


# ─── Auto-naming ──────────────────────────────────────────────────────────────

def _auto_name(index: int, prefix: str) -> str:
    return f"{prefix}_{index:03d}"


# ─── Public API ───────────────────────────────────────────────────────────────

class CVDetector:
    """
    Full OpenCV detection pipeline.

    Usage:
        detector = CVDetector()
        detector.load_image("mario.png")
        regions = detector.detect()
    """

    def __init__(self, config: Optional[CVDetectorConfig] = None) -> None:
        self.config = config or CVDetectorConfig()
        self._image: Optional[np.ndarray] = None
        self._bg_color: Optional[tuple[int, int, int]] = None

    # ── Image loading ─────────────────────────────────────────────────────────

    def load_image(self, path: str | Path) -> tuple[int, int]:
        """Load an image from disk.  Returns (width, height)."""
        self._image = cv2.imread(str(path), cv2.IMREAD_UNCHANGED)
        if self._image is None:
            raise FileNotFoundError(f"Cannot load image: {path}")
        self._bg_color = None   # reset cached bg
        h, w = self._image.shape[:2]
        return w, h

    def load_array(self, array: np.ndarray) -> None:
        """Load directly from a numpy array (BGRA or BGR)."""
        self._image = array.copy()
        self._bg_color = None

    # ── Background colour ─────────────────────────────────────────────────────

    def set_bg_color(self, r: int, g: int, b: int) -> None:
        """Override auto-detected background colour."""
        self._bg_color = (r, g, b)

    def get_bg_color(self) -> tuple[int, int, int]:
        if self._image is None:
            raise RuntimeError("No image loaded")
        if self._bg_color is None:
            self._bg_color = detect_background_color(self._image)
        return self._bg_color

    # ── Main pipeline ─────────────────────────────────────────────────────────

    def detect(self) -> list[SpriteRegion]:
        """
        Run the full pipeline and return detected SpriteRegions.
        Uses self.config for all parameters.
        """
        if self._image is None:
            raise RuntimeError("No image loaded. Call load_image() first.")

        cfg = self.config

        # 4.1 + 4.2  mask
        bg = self.get_bg_color()
        mask = build_foreground_mask(self._image, bg, cfg.tolerance)

        # 4.3  components
        rects = _components_to_rects(mask)

        # 4.4  filter + sort
        rects = _filter_rects(rects, cfg.min_area)
        rects = _sort_rects(rects)

        # 4.5  merge
        rects = _merge_rects(rects, cfg.merge_gap)
        rects = _sort_rects(rects)   # re-sort after merge

        # name + wrap
        regions = [
            SpriteRegion(_auto_name(i + 1, cfg.name_prefix), x, y, w, h)
            for i, (x, y, w, h) in enumerate(rects)
        ]
        return regions

    # ── Debug helper ──────────────────────────────────────────────────────────

    def debug_image(self, regions: list[SpriteRegion]) -> np.ndarray:
        """Return a copy of the loaded image with bounding boxes drawn (BGR)."""
        if self._image is None:
            raise RuntimeError("No image loaded")
        vis = cv2.cvtColor(self._image, cv2.COLOR_BGRA2BGR) \
            if has_alpha(self._image) else self._image.copy()
        for r in regions:
            cv2.rectangle(vis, (r.x, r.y), (r.x + r.w, r.y + r.h),
                          (0, 255, 255), 1)
        return vis


# ─── Smart Click Detect (AI mode — no ML required) ────────────────────────────

def smart_click_detect(
    image: np.ndarray,
    click_x: int,
    click_y: int,
    tolerance: int = 20,
    min_area: int = 16,
    padding: int = 1,
    name: str = "sprite",
) -> Optional[tuple[int, int, int, int]]:
    """
    Given a click point, find the sprite bounding box around it.

    Strategy:
      1. Build foreground mask (background subtraction by colour).
      2. Find the connected component that contains (click_x, click_y).
      3. Return its bounding rectangle (x, y, w, h) expanded by `padding`.

    Returns (x, y, w, h) in image coordinates, or None if click is on background.
    """
    h, w = image.shape[:2]
    click_x = max(0, min(click_x, w - 1))
    click_y = max(0, min(click_y, h - 1))

    # 1. Foreground mask
    bg_color = detect_background_color(image)
    mask = build_foreground_mask(image, bg_color, tolerance)

    # If the clicked pixel is background → expand tolerance and try again
    if mask[click_y, click_x] == 0:
        mask = build_foreground_mask(image, bg_color, tolerance * 2)
        if mask[click_y, click_x] == 0:
            return None   # truly background

    # 2. Connected components
    num_labels, labels = cv2.connectedComponents(mask, connectivity=8)
    target_label = int(labels[click_y, click_x])
    if target_label == 0:
        return None   # background component

    # 3. Bounding rect of target component
    component_mask = (labels == target_label).astype(np.uint8)
    ys, xs = np.where(component_mask)
    if len(xs) < min_area:
        return None   # too small

    x0 = max(0, int(xs.min()) - padding)
    y0 = max(0, int(ys.min()) - padding)
    x1 = min(w,  int(xs.max()) + padding + 1)
    y1 = min(h,  int(ys.max()) + padding + 1)
    rw, rh = x1 - x0, y1 - y0

    if rw * rh < min_area:
        return None

    return x0, y0, rw, rh
