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
            std::string token;
            file >> token;
            if (std::isdigit(token[0])) {
                int id = std::stoi(token);
                _tiles[row*_cols + col].type = static_cast<TileType>(id);
            } else {
                _tiles[row*_cols + col].type = TileType::Empty;
                _spawn_points.push_back({token[0], col * _tile_size, row * _tile_size});
            }
        } 
    }

    return true;
}

bool Tilemap::IsSolid(int col, int row) const {
    if (col < 0 || col >= _cols || row < 0 || row >= _rows) return false;
    return _tiles[row*_cols + col].type == TileType::Ground || _tiles[row*_cols + col].type == TileType::Brick;
}

bool Tilemap::IsOneWay(int col, int row) const {
    if (col < 0 || col >= _cols || row < 0 || row >= _rows) return false;
    return _tiles[row*_cols + col].type == TileType::OneWay;
}

bool Tilemap::IsBlockingFall(int col, int row) const {
    if (col < 0 || col >= _cols || row < 0 || row >= _rows) return false;
    return (_tiles[row*_cols + col].type == TileType::Ground || 
            _tiles[row*_cols + col].type == TileType::Brick  || 
            _tiles[row*_cols + col].type == TileType::OneWay);
}