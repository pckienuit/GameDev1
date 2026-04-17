#pragma once
#include <vector>
#include <cstdint>
#include <string>
#include <cctype>   

enum class TileType : uint8_t {
    Empty  = 0,
    Ground = 1,
    Brick  = 2,
    OneWay = 3,
};

struct Tile {
    TileType type = TileType::Empty;
};

struct SpawnInfo {
    char  token;
    int x;
    int y;
};

class Tilemap {
public:
    Tilemap(int cols, int rows, int tile_size);

    // Đọc/ghi tile tại vị trí (col, row)
    Tile&       GetTile(int col, int row);
    const Tile& GetTile(int col, int row) const;

    // Convert pixel pos → tile coords
    int PixelToCol(float x) const;
    int PixelToRow(float y) const;

    int GetCols()     const { return _cols; }
    int GetRows()     const { return _rows; }
    int GetTileSize() const { return _tile_size; }

    float GetWidth()  const { return (float)_cols * _tile_size; }
    float GetHeight() const { return (float)_rows * _tile_size; }

    // Load map
    bool LoadFromFile(const std::string& path);

    bool IsSolid(int col, int row) const;
    bool IsOneWay(int col, int row) const;
    bool IsBlockingFall(int col, int row) const;

    const std::vector<SpawnInfo>& GetSpawnPoints() const { return _spawn_points; }

private:
    int _cols, _rows, _tile_size;
    std::vector<Tile>      _tiles;
    std::vector<SpawnInfo> _spawn_points;
};
