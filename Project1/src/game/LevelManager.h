#pragma once
#include "LevelDef.h"
#include <vector>

class LevelManager {
public:
    LevelManager();

    const LevelDef& GetCurrent() const;
    bool HasNextLevel() const;
    void NextLevel();
    void Reset();           // quay về level 1

    int GetLevelIndex()  const;  // 0-based
    int GetTotalLevels() const;

private:
    std::vector<LevelDef> _levels;
    int _current = 0;
};
