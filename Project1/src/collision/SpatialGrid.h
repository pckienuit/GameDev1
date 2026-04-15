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

    void Resize(int world_w, int world_h, int cell_size) {
        _world_w = world_w;
        _world_h = world_h;
        _cell_size = cell_size;
        _cols = world_w / cell_size;
        _rows = world_h / cell_size;
        _cells.assign(_cols * _rows, {});
    }

private:
    int _world_w;
    int _world_h;
    int _cell_size;
    int _cols;
    int _rows;

    std::vector<std::vector<EntityID>> _cells;

    int to_cell_index(int col, int row) const;
};
