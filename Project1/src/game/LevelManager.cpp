#include "LevelManager.h"
#include <cassert>

LevelManager::LevelManager() {
    _levels = {
        // map_file               start_x   start_y   bg_r   bg_g   bg_b
        { "assets/level1.txt",    200.0f,   100.0f,   0.40f, 0.60f, 1.00f },  // Blue sky
        { "assets/level2.txt",    100.0f,   100.0f,   0.15f, 0.10f, 0.25f },  // Night/Underground
        { "assets/level3.txt",    100.0f,   100.0f,   0.85f, 0.45f, 0.20f },  // Sunset/Castle
    };
}

const LevelDef& LevelManager::GetCurrent() const {
    assert(_current >= 0 && _current < (int)_levels.size());
    return _levels[_current];
}

bool LevelManager::HasNextLevel() const {
    return _current + 1 < (int)_levels.size();
}

void LevelManager::NextLevel() {
    assert(HasNextLevel());
    ++_current;
}

void LevelManager::Reset() {
    _current = 0;
}

int LevelManager::GetLevelIndex() const {
    return _current;
}

int LevelManager::GetTotalLevels() const {
    return (int)_levels.size();
}
