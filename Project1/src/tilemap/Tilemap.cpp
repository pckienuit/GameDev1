#include "Tilemap.h"

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