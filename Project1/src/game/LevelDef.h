#pragma once
#include <string>

struct LevelDef {
    std::string map_file;
    float       player_start_x;
    float       player_start_y;
    float       bg_r, bg_g, bg_b;   // background clear color
};
