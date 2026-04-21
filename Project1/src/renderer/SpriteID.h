#pragma once

enum class SpriteID : int {
    // Tilemap
    BrickTile,
    GroundTile,
    QBlockTile0, QBlockTile1,
    PipeTL, PipeTR, PipeL, PipeR,

    // Decorations
    Cloud, Bush, Hill,
    
    // Background
    BgMountain, BgClouds, BgTrees, BgCastle, BgStars,

    // Entities
    MarioIdle,
    MarioWalk0, MarioWalk1, MarioWalk2,  // 3 walk frames
    MarioJump,

    // Enemies — Goomba
    GoombaWalk0, GoombaWalk1,
    GoombaDead,

    // Enemies — Koopa
    KoopaWalk0, KoopaWalk1,
    KoopaDead,
    KoopaShell,

    // Objects
    Flag, 
    Coin0, Coin1, Coin2,

    // HUD
    Heart,
    Digit0, Digit1, Digit2, Digit3, Digit4,
    Digit5, Digit6, Digit7, Digit8, Digit9,

    Count
};
