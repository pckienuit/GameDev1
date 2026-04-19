"""
Core exporter for SpriteCutter.

Tasks:
  3.1  JSON export  : SpriteProject → .json file
  3.2  JSON import  : .json → SpriteProject
  3.3  C++ snippet  : single region  → sheet.Define(...) / anim.AddFrame(...)
  3.4  C++ anim     : AnimGroup      → multi-line AddFrame block
"""
from __future__ import annotations
import json
from pathlib import Path
from typing import Optional

from core.models import SpriteProject, SpriteRegion, AnimGroup


# ─── Task 3.1  JSON Export ────────────────────────────────────────────────────

def export_json(project: SpriteProject, path: str | Path) -> None:
    """Write SpriteProject to a .json file."""
    path = Path(path)
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("w", encoding="utf-8") as f:
        json.dump(project.to_dict(), f, indent=2, ensure_ascii=False)


def project_to_json_str(project: SpriteProject) -> str:
    """Serialise SpriteProject to a JSON string (used by Web API)."""
    return json.dumps(project.to_dict(), indent=2, ensure_ascii=False)


# ─── Task 3.2  JSON Import ────────────────────────────────────────────────────

def import_json(path: str | Path) -> SpriteProject:
    """Load a SpriteProject from a .json file."""
    path = Path(path)
    with path.open("r", encoding="utf-8") as f:
        data = json.load(f)
    return SpriteProject.from_dict(data)


def project_from_json_str(text: str) -> SpriteProject:
    """Deserialise SpriteProject from a JSON string (used by Web API)."""
    return SpriteProject.from_dict(json.loads(text))


# ─── Task 3.3  C++ single-region snippet ─────────────────────────────────────

def region_to_cpp(
    region: SpriteRegion,
    style: str = "define",          # "define" | "addframe"
    duration: Optional[float] = None,
    include_comment: bool = True,
) -> str:
    """
    Generate a single C++ line for a sprite region.

    style="define"   → sheet.Define("name", x, y, w, h);
    style="addframe" → anim.AddFrame(x, y, w, h, dur);  // name

    Args:
        region:          The sprite region to emit.
        style:           Which API call to generate.
        duration:        Frame duration for AddFrame (required when style="addframe").
        include_comment: Append '// <name>' comment (AddFrame-style only).
    """
    x, y, w, h = region.x, region.y, region.w, region.h
    name = region.name

    if style == "define":
        return f'sheet.Define("{name}", {x}, {y}, {w}, {h});'

    if style == "addframe":
        dur = duration if duration is not None else 0.15
        line = f"anim.AddFrame({x}, {y}, {w}, {h}, {dur:.2f}f);"
        if include_comment:
            line += f"  // {name}"
        return line

    raise ValueError(f"Unknown style: '{style}'. Use 'define' or 'addframe'.")


# ─── Task 3.4  C++ animation block ───────────────────────────────────────────

def group_to_cpp(group: AnimGroup, project: SpriteProject) -> str:
    """
    Generate a complete C++ block for an AnimGroup.

    Example output:
        // Animation: walk_anim (0.15s/frame, looping)
        anim.AddFrame(275, 154, 16, 26, 0.15f);  // walk_1
        anim.AddFrame(305, 154, 16, 26, 0.15f);  // walk_2
        anim.AddFrame(335, 154, 16, 26, 0.15f);  // walk_3
    """
    loop_str = "looping" if group.looping else "one-shot"
    header = f"// Animation: {group.name} ({group.frame_duration:.2f}s/frame, {loop_str})"
    lines = [header]

    for frame_name in group.frames:
        region = project.get_sprite(frame_name)
        if region is None:
            lines.append(f"// WARNING: sprite '{frame_name}' not found")
            continue
        lines.append(region_to_cpp(region, style="addframe",
                                   duration=group.frame_duration))

    if not group.looping:
        lines.append("anim.SetLooping(false);")

    return "\n".join(lines)


def project_to_cpp(project: SpriteProject) -> str:
    """
    Generate full C++ snippet for entire project.

    Emits:
      1. All ungrouped sprites as sheet.Define(...)
      2. Each animation group as an AddFrame block
    """
    sections: list[str] = []

    # Header comment
    img_name = Path(project.image_path).name
    sections.append(
        f"// ── SpriteCutter export: {img_name} "
        f"({project.image_size[0]}×{project.image_size[1]}) ──"
    )

    # Ungrouped sprites → Define calls
    grouped_sprites = {
        frame for g in project.groups for frame in g.frames
    }
    ungrouped = [s for s in project.sprites if s.name not in grouped_sprites]
    if ungrouped:
        sections.append("\n// Sprites (ungrouped)")
        for region in ungrouped:
            sections.append(region_to_cpp(region, style="define"))

    # Animation groups → AddFrame blocks
    for group in project.groups:
        sections.append("")
        sections.append(group_to_cpp(group, project))

    return "\n".join(sections)
