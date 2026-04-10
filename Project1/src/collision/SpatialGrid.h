#pragma once
#include <vector>
#include <cstdint>
#include "../ecs/EntityManager.h"

class SpatialGrid {
public:
    SpatialGrid(int world_w, int world_h, int cell_size);


    void Clear();

    void Insert(EntityID id, float x, float y, float w, float h);

    void Query(float x, float y, float w, float h,
               std::vector<EntityID>& out_result) const;

private:
    int _world_w;
    int _world_h;
    int _cell_size;
    int _cols;
    int _rows;

    std::vector<std::vector<EntityID>> _cells;

    int to_cell_index(int col, int row) const;
};
