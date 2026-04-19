"""
Unit tests for core/exporter.py — Phase 3 Verification
Run: pytest tests/test_exporter.py -v
"""
import json
import sys
import os
import tempfile
from pathlib import Path

import pytest

sys.path.insert(0, os.path.join(os.path.dirname(__file__), ".."))
from core.models import SpriteProject, SpriteRegion, AnimGroup
from core.exporter import (
    export_json, import_json,
    project_to_json_str, project_from_json_str,
    region_to_cpp, group_to_cpp, project_to_cpp,
)


# ─── Fixtures ─────────────────────────────────────────────────────────────────

@pytest.fixture
def mario_project() -> SpriteProject:
    """Mirrors actual Player.cpp animation data."""
    p = SpriteProject("mario.png", (413, 1637))
    p.add_sprite(SpriteRegion("idle",   245, 154, 16, 26, "idle_anim"))
    p.add_sprite(SpriteRegion("walk_1", 275, 154, 16, 26, "walk_anim"))
    p.add_sprite(SpriteRegion("walk_2", 305, 154, 16, 26, "walk_anim"))
    p.add_sprite(SpriteRegion("walk_3", 335, 154, 16, 26, "walk_anim"))
    p.add_sprite(SpriteRegion("jump",   246, 233, 16, 26, "jump_anim"))
    p.add_group(AnimGroup("idle_anim", ["idle"],                        0.4))
    p.add_group(AnimGroup("walk_anim", ["walk_1", "walk_2", "walk_3"],  0.15))
    p.add_group(AnimGroup("jump_anim", ["jump"],                        1.0,  looping=False))
    return p


@pytest.fixture
def score_project() -> SpriteProject:
    """Mirrors actual ScoreRenderer.cpp Define calls."""
    p = SpriteProject("misc.png", (640, 480))
    p.add_sprite(SpriteRegion("heart", 596, 193, 16, 14))
    p.add_sprite(SpriteRegion("digit_0", 496, 224, 8, 8))
    p.add_sprite(SpriteRegion("digit_1", 504, 224, 8, 8))
    # no groups — all ungrouped
    return p


# ─── Task 3.1: JSON Export ────────────────────────────────────────────────────

class TestJsonExport:

    def test_export_creates_file(self, mario_project, tmp_path):
        out = tmp_path / "mario.json"
        export_json(mario_project, out)
        assert out.exists()
        assert out.stat().st_size > 0

    def test_export_creates_parent_dirs(self, mario_project, tmp_path):
        out = tmp_path / "deep" / "nested" / "mario.json"
        export_json(mario_project, out)
        assert out.exists()

    def test_export_valid_json(self, mario_project, tmp_path):
        out = tmp_path / "mario.json"
        export_json(mario_project, out)
        data = json.loads(out.read_text())
        assert "sprites" in data
        assert "animations" in data
        assert "image" in data

    def test_export_correct_sprite_count(self, mario_project, tmp_path):
        out = tmp_path / "mario.json"
        export_json(mario_project, out)
        data = json.loads(out.read_text())
        assert len(data["sprites"]) == 5
        assert len(data["animations"]) == 3

    def test_export_json_str(self, mario_project):
        text = project_to_json_str(mario_project)
        data = json.loads(text)
        assert data["image"] == "mario.png"
        assert data["image_size"] == [413, 1637]


# ─── Task 3.2: JSON Import ────────────────────────────────────────────────────

class TestJsonImport:

    def test_roundtrip_file(self, mario_project, tmp_path):
        """Task 3.2: export → import → data identical"""
        out = tmp_path / "mario.json"
        export_json(mario_project, out)
        restored = import_json(out)

        assert restored.image_path == "mario.png"
        assert restored.image_size == (413, 1637)
        assert len(restored.sprites) == 5
        assert len(restored.groups) == 3

    def test_roundtrip_sprite_coords(self, mario_project, tmp_path):
        out = tmp_path / "mario.json"
        export_json(mario_project, out)
        restored = import_json(out)

        idle = restored.get_sprite("idle")
        assert idle is not None
        assert idle.x == 245 and idle.y == 154
        assert idle.w == 16  and idle.h == 26

        walk1 = restored.get_sprite("walk_1")
        assert walk1.x == 275

    def test_roundtrip_animation_groups(self, mario_project, tmp_path):
        out = tmp_path / "mario.json"
        export_json(mario_project, out)
        restored = import_json(out)

        walk = restored.get_group("walk_anim")
        assert walk.frames == ["walk_1", "walk_2", "walk_3"]
        assert walk.frame_duration == 0.15
        assert walk.looping is True

        jump = restored.get_group("jump_anim")
        assert jump.looping is False

    def test_roundtrip_validates_clean(self, mario_project, tmp_path):
        """Restored project should have zero validation errors"""
        out = tmp_path / "mario.json"
        export_json(mario_project, out)
        restored = import_json(out)
        assert restored.validate() == []

    def test_roundtrip_str(self, mario_project):
        text = project_to_json_str(mario_project)
        restored = project_from_json_str(text)
        assert restored.image_path == mario_project.image_path
        assert len(restored.sprites) == len(mario_project.sprites)


# ─── Task 3.3: C++ single-region snippet ─────────────────────────────────────

class TestRegionToCpp:

    def test_define_style(self):
        """Matches SpriteSheet::Define(name, x, y, w, h)"""
        region = SpriteRegion("heart", 596, 193, 16, 14)
        line = region_to_cpp(region, style="define")
        assert line == 'sheet.Define("heart", 596, 193, 16, 14);'

    def test_addframe_style_with_duration(self):
        """Matches Animation::AddFrame(x, y, w, h, dur)"""
        region = SpriteRegion("idle", 245, 154, 16, 26)
        line = region_to_cpp(region, style="addframe", duration=0.4)
        assert "245, 154, 16, 26, 0.40f" in line

    def test_addframe_style_has_comment(self):
        region = SpriteRegion("walk_1", 275, 154, 16, 26)
        line = region_to_cpp(region, style="addframe", duration=0.15)
        assert "// walk_1" in line

    def test_addframe_style_no_comment(self):
        region = SpriteRegion("walk_1", 275, 154, 16, 26)
        line = region_to_cpp(region, style="addframe", duration=0.15, include_comment=False)
        assert "//" not in line

    def test_addframe_default_duration(self):
        region = SpriteRegion("s", 0, 0, 16, 16)
        line = region_to_cpp(region, style="addframe")
        assert "0.15f" in line

    def test_unknown_style_raises(self):
        region = SpriteRegion("s", 0, 0, 16, 16)
        with pytest.raises(ValueError, match="Unknown style"):
            region_to_cpp(region, style="invalid")

    def test_score_define_matches_actual_code(self, score_project):
        """Verify output matches the real ScoreRenderer.cpp pattern."""
        heart = score_project.get_sprite("heart")
        line = region_to_cpp(heart, style="define")
        # actual: _sheet.Define("heart", 596, 193, HEART_W, HEART_H);
        assert '"heart"' in line
        assert "596" in line
        assert "193" in line


# ─── Task 3.4: C++ animation block ───────────────────────────────────────────

class TestGroupToCpp:

    def test_block_has_header_comment(self, mario_project):
        group = mario_project.get_group("walk_anim")
        block = group_to_cpp(group, mario_project)
        assert "// Animation: walk_anim" in block

    def test_block_has_correct_frame_count(self, mario_project):
        group = mario_project.get_group("walk_anim")
        block = group_to_cpp(group, mario_project)
        # 3 frames → 3 AddFrame lines
        assert block.count("AddFrame") == 3

    def test_block_correct_coords(self, mario_project):
        group = mario_project.get_group("walk_anim")
        block = group_to_cpp(group, mario_project)
        assert "275, 154, 16, 26" in block  # walk_1
        assert "305, 154, 16, 26" in block  # walk_2
        assert "335, 154, 16, 26" in block  # walk_3

    def test_block_duration_in_header(self, mario_project):
        group = mario_project.get_group("walk_anim")
        block = group_to_cpp(group, mario_project)
        assert "0.15" in block

    def test_looping_false_adds_setlooping(self, mario_project):
        group = mario_project.get_group("jump_anim")
        block = group_to_cpp(group, mario_project)
        assert "SetLooping(false)" in block

    def test_looping_true_no_setlooping(self, mario_project):
        group = mario_project.get_group("walk_anim")
        block = group_to_cpp(group, mario_project)
        assert "SetLooping" not in block

    def test_missing_sprite_shows_warning(self, mario_project):
        """If a frame references a missing sprite, emit a WARNING comment."""
        group = AnimGroup("broken", ["GHOST"], 0.15)
        block = group_to_cpp(group, mario_project)
        assert "WARNING" in block
        assert "GHOST" in block


class TestProjectToCpp:

    def test_full_project_contains_all_groups(self, mario_project):
        cpp = project_to_cpp(mario_project)
        assert "idle_anim" in cpp
        assert "walk_anim" in cpp
        assert "jump_anim" in cpp

    def test_full_project_has_header(self, mario_project):
        cpp = project_to_cpp(mario_project)
        assert "mario.png" in cpp
        assert "413" in cpp

    def test_ungrouped_sprites_use_define(self, score_project):
        """Ungrouped sprites should use sheet.Define style."""
        cpp = project_to_cpp(score_project)
        assert 'sheet.Define("heart"' in cpp
        assert "596, 193" in cpp

    def test_no_duplicate_sprites(self, mario_project):
        """Grouped sprites should NOT appear as Define calls too."""
        cpp = project_to_cpp(mario_project)
        # 'idle' is grouped — should not appear as Define
        assert 'sheet.Define("idle"' not in cpp


# ─── Run directly ─────────────────────────────────────────────────────────────

if __name__ == "__main__":
    import pytest
    pytest.main([__file__, "-v"])
