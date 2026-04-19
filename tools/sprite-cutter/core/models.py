"""
Core data models for SpriteCutter.
Shared between Desktop App, Web App, and Exporter.
"""
from __future__ import annotations
from dataclasses import dataclass, field
from typing import Optional


# ─── Task 2.1: SpriteRegion ───────────────────────────────────────────────────

@dataclass
class SpriteRegion:
    """A named rectangular region within a sprite sheet."""
    name: str
    x: int
    y: int
    w: int
    h: int
    group: str = ""  # optional: animation group this sprite belongs to

    def validate(self, image_w: int = 0, image_h: int = 0) -> list[str]:
        """Return list of validation errors. Empty list = valid."""
        errors: list[str] = []

        if not self.name.strip():
            errors.append("name cannot be empty")
        if self.w <= 0:
            errors.append(f"width must be > 0, got {self.w}")
        if self.h <= 0:
            errors.append(f"height must be > 0, got {self.h}")
        if self.x < 0:
            errors.append(f"x must be >= 0, got {self.x}")
        if self.y < 0:
            errors.append(f"y must be >= 0, got {self.y}")

        if image_w > 0 and (self.x + self.w) > image_w:
            errors.append(f"region extends beyond image width ({image_w}): x+w={self.x + self.w}")
        if image_h > 0 and (self.y + self.h) > image_h:
            errors.append(f"region extends beyond image height ({image_h}): y+h={self.y + self.h}")

        return errors

    def to_dict(self) -> dict:
        return {"name": self.name, "x": self.x, "y": self.y,
                "w": self.w, "h": self.h, "group": self.group}

    @classmethod
    def from_dict(cls, d: dict) -> "SpriteRegion":
        return cls(
            name=d["name"], x=d["x"], y=d["y"],
            w=d["w"], h=d["h"], group=d.get("group", "")
        )


# ─── Task 2.2: AnimGroup ─────────────────────────────────────────────────────

@dataclass
class AnimGroup:
    """An ordered animation sequence composed of SpriteRegion names."""
    name: str
    frames: list[str] = field(default_factory=list)  # ordered list of SpriteRegion names
    frame_duration: float = 0.15                       # seconds per frame (uniform)
    looping: bool = True

    def validate(self, known_sprites: Optional[set[str]] = None) -> list[str]:
        errors: list[str] = []

        if not self.name.strip():
            errors.append("animation name cannot be empty")
        if len(self.frames) == 0:
            errors.append(f"animation '{self.name}' has no frames")
        if self.frame_duration <= 0:
            errors.append(f"frame_duration must be > 0, got {self.frame_duration}")

        if known_sprites is not None:
            for frame_name in self.frames:
                if frame_name not in known_sprites:
                    errors.append(f"frame '{frame_name}' not found in sprite list")

        return errors

    def to_dict(self) -> dict:
        return {
            "name": self.name,
            "frames": self.frames,
            "frame_duration": self.frame_duration,
            "looping": self.looping,
        }

    @classmethod
    def from_dict(cls, d: dict) -> "AnimGroup":
        return cls(
            name=d["name"],
            frames=d.get("frames", []),
            frame_duration=d.get("frame_duration", 0.15),
            looping=d.get("looping", True),
        )


# ─── Task 2.3: SpriteProject ─────────────────────────────────────────────────

@dataclass
class SpriteProject:
    """The root project: holds the image info, all regions, and animations."""
    image_path: str
    image_size: tuple[int, int]  # (width, height)
    sprites: list[SpriteRegion] = field(default_factory=list)
    groups: list[AnimGroup] = field(default_factory=list)

    # ─── Task 2.4: Validation ─────────────────────────────────────────────────

    def validate(self) -> list[str]:
        """Full project validation. Returns all errors found."""
        errors: list[str] = []
        image_w, image_h = self.image_size

        # Image sanity
        if image_w <= 0 or image_h <= 0:
            errors.append(f"invalid image size: {self.image_size}")

        # Sprite-level validation + unique name check
        seen_names: set[str] = set()
        for sprite in self.sprites:
            sprite_errors = sprite.validate(image_w, image_h)
            for err in sprite_errors:
                errors.append(f"[sprite '{sprite.name}'] {err}")

            if sprite.name in seen_names:
                errors.append(f"duplicate sprite name: '{sprite.name}'")
            seen_names.add(sprite.name)

        # Animation-level validation
        seen_group_names: set[str] = set()
        for group in self.groups:
            group_errors = group.validate(known_sprites=seen_names)
            for err in group_errors:
                errors.append(f"[anim '{group.name}'] {err}")

            if group.name in seen_group_names:
                errors.append(f"duplicate animation name: '{group.name}'")
            seen_group_names.add(group.name)

        return errors

    # ─── Sprite helpers ───────────────────────────────────────────────────────

    def add_sprite(self, sprite: SpriteRegion) -> None:
        self.sprites.append(sprite)

    def remove_sprite(self, name: str) -> bool:
        """Remove sprite by name. Also removes it from any animation groups."""
        original = len(self.sprites)
        self.sprites = [s for s in self.sprites if s.name != name]
        for group in self.groups:
            group.frames = [f for f in group.frames if f != name]
        return len(self.sprites) < original

    def get_sprite(self, name: str) -> Optional[SpriteRegion]:
        return next((s for s in self.sprites if s.name == name), None)

    def rename_sprite(self, old_name: str, new_name: str) -> bool:
        sprite = self.get_sprite(old_name)
        if sprite is None:
            return False
        sprite.name = new_name
        for group in self.groups:
            group.frames = [new_name if f == old_name else f for f in group.frames]
        return True

    def sprite_names(self) -> set[str]:
        return {s.name for s in self.sprites}

    # ─── Group helpers ────────────────────────────────────────────────────────

    def add_group(self, group: AnimGroup) -> None:
        self.groups.append(group)

    def remove_group(self, name: str) -> bool:
        original = len(self.groups)
        self.groups = [g for g in self.groups if g.name != name]
        return len(self.groups) < original

    def get_group(self, name: str) -> Optional[AnimGroup]:
        return next((g for g in self.groups if g.name == name), None)

    # ─── Serialization ───────────────────────────────────────────────────────

    def to_dict(self) -> dict:
        return {
            "image": self.image_path,
            "image_size": list(self.image_size),
            "sprites": [s.to_dict() for s in self.sprites],
            "animations": [g.to_dict() for g in self.groups],
        }

    @classmethod
    def from_dict(cls, d: dict) -> "SpriteProject":
        return cls(
            image_path=d.get("image", ""),
            image_size=tuple(d.get("image_size", [0, 0])),
            sprites=[SpriteRegion.from_dict(s) for s in d.get("sprites", [])],
            groups=[AnimGroup.from_dict(g) for g in d.get("animations", [])],
        )
