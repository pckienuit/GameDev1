"""
Unit tests for core/models.py — Phase 2 Verification
Run: pytest tests/test_models.py -v
"""
import pytest
import sys
import os

sys.path.insert(0, os.path.join(os.path.dirname(__file__), ".."))
from core.models import SpriteRegion, AnimGroup, SpriteProject


# ─── SpriteRegion Tests ───────────────────────────────────────────────────────

class TestSpriteRegion:

    def test_create_basic(self):
        """Task 2.1: basic creation"""
        s = SpriteRegion(name="idle", x=245, y=154, w=16, h=26)
        assert s.name == "idle"
        assert s.x == 245 and s.y == 154
        assert s.w == 16 and s.h == 26
        assert s.group == ""

    def test_create_with_group(self):
        s = SpriteRegion(name="walk_1", x=275, y=154, w=16, h=26, group="walk_anim")
        assert s.group == "walk_anim"

    def test_validate_valid(self):
        """Task 2.4: valid region passes"""
        s = SpriteRegion("idle", 245, 154, 16, 26)
        assert s.validate(413, 1637) == []

    def test_validate_empty_name(self):
        s = SpriteRegion("  ", 0, 0, 16, 16)
        errors = s.validate()
        assert any("name" in e for e in errors)

    def test_validate_zero_width(self):
        s = SpriteRegion("test", 0, 0, 0, 16)
        errors = s.validate()
        assert any("width" in e for e in errors)

    def test_validate_zero_height(self):
        s = SpriteRegion("test", 0, 0, 16, 0)
        errors = s.validate()
        assert any("height" in e for e in errors)

    def test_validate_negative_coords(self):
        s = SpriteRegion("test", -1, -5, 16, 16)
        errors = s.validate()
        assert any("x" in e for e in errors)
        assert any("y" in e for e in errors)

    def test_validate_out_of_bounds(self):
        """Region extends beyond image bounds"""
        s = SpriteRegion("test", 400, 0, 50, 16)  # x+w = 450 > 413
        errors = s.validate(image_w=413, image_h=1637)
        assert any("width" in e for e in errors)

    def test_to_dict(self):
        s = SpriteRegion("idle", 245, 154, 16, 26, "idle_anim")
        d = s.to_dict()
        assert d == {"name": "idle", "x": 245, "y": 154, "w": 16, "h": 26, "group": "idle_anim"}

    def test_from_dict_roundtrip(self):
        original = SpriteRegion("walk_1", 275, 154, 16, 26, "walk_anim")
        restored = SpriteRegion.from_dict(original.to_dict())
        assert restored.name == original.name
        assert restored.x == original.x
        assert restored.y == original.y
        assert restored.w == original.w
        assert restored.h == original.h
        assert restored.group == original.group

    def test_from_dict_missing_group(self):
        """group field is optional in JSON"""
        d = {"name": "idle", "x": 0, "y": 0, "w": 16, "h": 16}
        s = SpriteRegion.from_dict(d)
        assert s.group == ""


# ─── AnimGroup Tests ──────────────────────────────────────────────────────────

class TestAnimGroup:

    def test_create_basic(self):
        """Task 2.2: basic creation"""
        g = AnimGroup(name="walk_anim", frames=["walk_1", "walk_2"], frame_duration=0.15)
        assert g.name == "walk_anim"
        assert len(g.frames) == 2
        assert g.frame_duration == 0.15
        assert g.looping is True

    def test_create_defaults(self):
        g = AnimGroup(name="idle")
        assert g.frames == []
        assert g.frame_duration == 0.15
        assert g.looping is True

    def test_validate_valid(self):
        g = AnimGroup("walk", ["walk_1", "walk_2"], 0.15)
        known = {"walk_1", "walk_2", "idle"}
        assert g.validate(known_sprites=known) == []

    def test_validate_empty_name(self):
        g = AnimGroup("", ["walk_1"])
        errors = g.validate()
        assert any("name" in e for e in errors)

    def test_validate_no_frames(self):
        g = AnimGroup("walk", [])
        errors = g.validate()
        assert any("no frames" in e for e in errors)

    def test_validate_bad_duration(self):
        g = AnimGroup("walk", ["walk_1"], frame_duration=0.0)
        errors = g.validate()
        assert any("frame_duration" in e for e in errors)

    def test_validate_unknown_sprite(self):
        g = AnimGroup("walk", ["walk_1", "MISSING_SPRITE"], 0.15)
        errors = g.validate(known_sprites={"walk_1"})
        assert any("MISSING_SPRITE" in e for e in errors)

    def test_to_dict(self):
        g = AnimGroup("walk", ["walk_1", "walk_2"], 0.15, looping=True)
        d = g.to_dict()
        assert d["name"] == "walk"
        assert d["frames"] == ["walk_1", "walk_2"]
        assert d["frame_duration"] == 0.15

    def test_from_dict_roundtrip(self):
        original = AnimGroup("jump", ["jump_1"], 0.4, looping=False)
        restored = AnimGroup.from_dict(original.to_dict())
        assert restored.name == original.name
        assert restored.frames == original.frames
        assert restored.frame_duration == original.frame_duration
        assert restored.looping == original.looping


# ─── SpriteProject Tests ─────────────────────────────────────────────────────

class TestSpriteProject:

    def _make_project(self) -> SpriteProject:
        """Helper: valid project matching actual mario.png data"""
        p = SpriteProject(image_path="mario.png", image_size=(413, 1637))
        p.add_sprite(SpriteRegion("idle",   245, 154, 16, 26, "idle_anim"))
        p.add_sprite(SpriteRegion("walk_1", 275, 154, 16, 26, "walk_anim"))
        p.add_sprite(SpriteRegion("walk_2", 305, 154, 16, 26, "walk_anim"))
        p.add_sprite(SpriteRegion("walk_3", 335, 154, 16, 26, "walk_anim"))
        p.add_sprite(SpriteRegion("jump",   246, 233, 16, 26, "jump_anim"))
        p.add_group(AnimGroup("idle_anim", ["idle"],                 0.4))
        p.add_group(AnimGroup("walk_anim", ["walk_1","walk_2","walk_3"], 0.15))
        p.add_group(AnimGroup("jump_anim", ["jump"],                 1.0, looping=False))
        return p

    def test_create(self):
        """Task 2.3: basic creation"""
        p = SpriteProject("mario.png", (413, 1637))
        assert p.image_path == "mario.png"
        assert p.image_size == (413, 1637)
        assert p.sprites == []
        assert p.groups == []

    def test_add_and_get_sprite(self):
        p = self._make_project()
        assert p.get_sprite("idle") is not None
        assert p.get_sprite("MISSING") is None

    def test_sprite_names(self):
        p = self._make_project()
        names = p.sprite_names()
        assert "idle" in names
        assert "walk_1" in names

    def test_remove_sprite_also_cleans_groups(self):
        p = self._make_project()
        p.remove_sprite("walk_1")
        assert p.get_sprite("walk_1") is None
        walk_group = p.get_group("walk_anim")
        assert "walk_1" not in walk_group.frames

    def test_rename_sprite_updates_groups(self):
        p = self._make_project()
        p.rename_sprite("walk_1", "walk_a")
        assert p.get_sprite("walk_a") is not None
        assert p.get_sprite("walk_1") is None
        walk_group = p.get_group("walk_anim")
        assert "walk_a" in walk_group.frames
        assert "walk_1" not in walk_group.frames

    def test_validate_valid_project(self):
        """Task 2.4: valid project has no errors"""
        p = self._make_project()
        errors = p.validate()
        assert errors == [], f"Unexpected errors: {errors}"

    def test_validate_duplicate_sprite_name(self):
        p = SpriteProject("test.png", (100, 100))
        p.add_sprite(SpriteRegion("idle", 0, 0, 16, 16))
        p.add_sprite(SpriteRegion("idle", 20, 0, 16, 16))  # duplicate
        errors = p.validate()
        assert any("duplicate" in e and "idle" in e for e in errors)

    def test_validate_duplicate_group_name(self):
        p = SpriteProject("test.png", (100, 100))
        p.add_sprite(SpriteRegion("s1", 0, 0, 16, 16))
        p.add_group(AnimGroup("walk", ["s1"]))
        p.add_group(AnimGroup("walk", ["s1"]))  # duplicate
        errors = p.validate()
        assert any("duplicate" in e and "walk" in e for e in errors)

    def test_validate_group_references_missing_sprite(self):
        p = SpriteProject("test.png", (100, 100))
        p.add_sprite(SpriteRegion("idle", 0, 0, 16, 16))
        p.add_group(AnimGroup("walk", ["idle", "GHOST"]))  # GHOST doesn't exist
        errors = p.validate()
        assert any("GHOST" in e for e in errors)

    def test_to_dict_structure(self):
        p = self._make_project()
        d = p.to_dict()
        assert d["image"] == "mario.png"
        assert d["image_size"] == [413, 1637]
        assert len(d["sprites"]) == 5
        assert len(d["animations"]) == 3
        assert d["sprites"][0]["name"] == "idle"
        assert d["animations"][1]["name"] == "walk_anim"

    def test_from_dict_roundtrip(self):
        """Task 2.3 + 2.4: full serialization roundtrip"""
        original = self._make_project()
        restored = SpriteProject.from_dict(original.to_dict())

        assert restored.image_path == original.image_path
        assert restored.image_size == original.image_size
        assert len(restored.sprites) == len(original.sprites)
        assert len(restored.groups) == len(original.groups)

        # Check individual sprites preserved
        for orig_s, rest_s in zip(original.sprites, restored.sprites):
            assert orig_s.name == rest_s.name
            assert orig_s.x == rest_s.x
            assert orig_s.y == rest_s.y

        # Check animations preserved
        walk = restored.get_group("walk_anim")
        assert walk is not None
        assert walk.frames == ["walk_1", "walk_2", "walk_3"]
        assert walk.frame_duration == 0.15

        # Re-validate restored project
        assert restored.validate() == []


# ─── Run directly ─────────────────────────────────────────────────────────────

if __name__ == "__main__":
    pytest.main([__file__, "-v"])
