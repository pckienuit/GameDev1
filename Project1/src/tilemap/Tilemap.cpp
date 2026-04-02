#include "Tilemap.h"
#include <fstream>
#include <sstream>
#include <string>

Tilemap::Tilemap(int cols, int rows, int tile_size) :
    _cols(cols), _rows(rows), _tile_size(tile_size)
{
    _tiles.resize(cols * rows);
}

Tile& Tilemap::GetTile(int col, int row) {
    return _tiles[row*_cols + col];
}

const Tile& Tilemap::GetTile(int col, int row) const {
    return _tiles[row*_cols + col];
}

int Tilemap::PixelToCol(float x) const {
    return static_cast<int>(x / _tile_size);
}

int Tilemap::PixelToRow(float y) const {
    return static_cast<int>(y / _tile_size);
}
bool Tilemap::LoadFromFile(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) return false;

    file >> _cols >> _rows >> _tile_size;
    _tiles.resize(_cols * _rows);

    for (int row = 0; row < _rows; ++row) {
        for (int col = 0; col < _cols; ++col) {
            int tile_id;
            file >> tile_id;
            _tiles[row*_cols + col].type = static_cast<TileType>(tile_id);
        }
    }

    return true;
}
