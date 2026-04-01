#pragma once
#include <vector>
#include <cstdint>

// Tile type — mở rộng dần khi cần
enum class TileType : uint8_t {
    Empty  = 0,
    Ground = 1,
    Brick  = 2,
    // TODO: thêm QuestionBlock, Pipe... sau
};

struct Tile {
    TileType type = TileType::Empty;
    // TODO: sau này thêm texture_id, flags (solid, one-way...)
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

private:
    int _cols, _rows, _tile_size;
    std::vector<Tile> _tiles; // TODO: tại sao dùng 1D vector thay vì 2D?
};
