#include "SpatialGrid.h"
#include <cmath>
#include <algorithm>

SpatialGrid::SpatialGrid(int world_w, int world_h, int cell_size):
    _world_w(world_w),
    _world_h(world_h),
    _cell_size(cell_size),
    _cols(world_w / cell_size),
    _rows(world_h / cell_size),
    _cells(_cols * _rows)
{    
}

void SpatialGrid::Clear() {
    for (auto& cell : _cells) {
        cell.clear();
    }
}

void SpatialGrid::Insert(EntityID id, float x, float y, float w, float h) {
    int min_col = std::max(0, static_cast<int>(floorf(x / _cell_size)));
    int max_col = std::min(_cols - 1, static_cast<int>(floorf((x + w) / _cell_size)));
    int min_row = std::max(0, static_cast<int>(floorf(y / _cell_size)));
    int max_row = std::min(_rows - 1, static_cast<int>(floorf((y + h) / _cell_size)));

    for (int row = min_row; row <= max_row; ++row) {
        for (int col = min_col; col <= max_col; ++col) {
            int cell_idx = to_cell_index(col, row);
            _cells[cell_idx].push_back(id);
        }
    }
}

void SpatialGrid::Query(float x, float y, float w, float h, std::vector<EntityID>& out_result) const {
    int min_col = std::max(0, (int)floorf(x / _cell_size));
    int max_col = std::min(_cols - 1, (int)floorf((x + w) / _cell_size));
    int min_row = std::max(0, (int)floorf(y / _cell_size));
    int max_row = std::min(_rows - 1, (int)floorf((y + h) / _cell_size));

    for (int row = min_row; row <= max_row; ++row) {
        for (int col = min_col; col <= max_col; ++col) {
            int cell_idx = to_cell_index(col, row);
            out_result.insert(out_result.end(), _cells[cell_idx].begin(), _cells[cell_idx].end());
        }
    }

    std::sort(out_result.begin(), out_result.end());
    out_result.erase(
        std::unique(out_result.begin(), out_result.end()),
        out_result.end()
    );
}

int SpatialGrid::to_cell_index(int col, int row) const {
    return (row * _cols) + col;
}