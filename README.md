# Mario Platformer - DirectX 11 & C++ Game Engine

Dự án game Mario Platformer được phát triển từ đầu bằng C++ và DirectX 11. Project xây dựng một Game Engine 2D tự chế (Custom 2D Engine) tối ưu cao với cơ chế vật lý tùy chỉnh, hệ thống va chạm Swept AABB & Spatial Grid, và quản lý âm thanh thông qua API XAudio2.

---

## 🗺️ Sơ đồ hệ thống (System Architecture)

Dưới đây là sơ đồ kiến trúc hoạt động của Game Loop và luồng tương tác giữa các hệ thống cốt lõi:

```text
+-----------------------------------------------------------------+
|                      WinMain / GameLoop (50Hz)                  |
+-------------------------------+---------------------------------+
                                |
                                v
                       +-----------------+
                       |   Input::Poll   |
                       +--------+--------+
                                |
                                v
                  +-----------------------------+
                  |   Player::PrepareVelocity   |
                  +-------------+---------------+
                                |
                                v
                    +-----------------------+
                    | EnemyManager::Update  |
                    +-----------+-----------+
                                |
                                v
                  +-----------------------------+
                  |  CollisionSystem::BeginFrame|
                  +-------------+---------------+
                                | (Register Entities)
                                v
                  +-----------------------------+
                  |  CollisionSystem::Detect    |
                  |  (Swept AABB + SpatialGrid) |
                  +-------------+---------------+
                                |
                                v
                +-------------------------------+
                | EnemyManager::HandleCollisions|
                +---------------+---------------+
                                |
                                v
             +-------------------------------------+
             | Player::Move & Resolve Tilemap Coll |
             +------------------+------------------+
                                |
                                v
                    +-----------------------+
                    | Camera::Follow & Clamp|
                    +-----------+-----------+
                                |
                                v
                     +---------------------+
                     | Renderer::BeginFrame|
                     +----------+----------+
                                |
                                v
             +-------------------------------------+
             | SpriteBatch::Draw Map/Entities/HUD  |
             +------------------+------------------+
                                |
                                v
                      +-------------------+
                      | Renderer::EndFrame|
                      +---------+---------+
                                |
                                +---------------------------+
                                                            |
                                                            v (Loop Back)
```

---

## 🛠️ Các cơ chế cốt lõi (Core Mechanics)

### 1. Cơ chế Vật lý (Physics)
Vật lý của người chơi được cài đặt trong lớp [Player](file:///d:/GameDev1/Project1/src/game/Player.h) thông qua các phương thức [PrepareVelocity](file:///d:/GameDev1/Project1/src/game/Player.cpp#L116) và [Move](file:///d:/GameDev1/Project1/src/game/Player.cpp#L166):
- **Gia tốc & Ma sát (Acceleration & Friction)**: Mario tăng tốc mượt mà dựa trên lực đẩy ngang (`ACCELERATION`) và giảm tốc tự nhiên nhờ (`DECELERATION`).
- **Skidding (Trượt khi đổi hướng)**: Khi đổi hướng đột ngột, Mario sẽ áp dụng lực hãm lớn hơn (`SKID_DECEL`) để tạo cảm giác trượt quán tính đặc trưng của các dòng game cổ điển.
- **Nhảy biến thiên (Variable Jump Height)**: Mario có thể kiểm soát độ cao nhảy bằng cách giữ nút nhảy. Khi nhả nút sớm, vận tốc nhảy đứng sẽ bị triệt tiêu theo hệ số `JUMP_CUT_FACTOR`.
- **Coyote Time**: Cho phép người chơi nhảy thêm một khoảng thời gian ngắn sau khi vừa rời khỏi rìa nền đất (`COYOTE_TIME = 0.1s`), tăng cảm giác điều khiển mượt mà.
- **Jump Buffer**: Lưu trữ lượt nhấn nhảy của người chơi trong `JUMP_BUFFER_TIME = 0.1s` trước khi tiếp đất để tự động nhảy ngay khi vừa chạm đất.

---

### 2. Cơ chế Va chạm (Collision)
Hệ thống va chạm được chia làm hai loại chính: Va chạm Thực thể - Thực thể (lớp [CollisionSystem](file:///d:/GameDev1/Project1/src/collision/CollisionSystem.h)) và Va chạm Thực thể - Tilemap (trong lớp [Player](file:///d:/GameDev1/Project1/src/game/Player.h) và [EnemyManager](file:///d:/GameDev1/Project1/src/game/EnemyManager.h)).

- **Swept AABB (AABB quét)**:
  - Khác với kiểm tra AABB tĩnh (dễ bị xuyên thấu - tunneling khi di chuyển với tốc độ cao), phương thức `AABB::Swept` trong [AABB.cpp](file:///d:/GameDev1/Project1/src/collision/AABB.cpp) tính toán thời điểm va chạm (`hit_time` từ 0 đến 1) và vector pháp tuyến (`normal_x`, `normal_y`) dựa trên vận tốc tương đối giữa hai vật thể.
  - Vận tốc thực thể được cắt giảm (clamp) chính xác theo góc tiếp xúc để tránh đè lấn.
- **Phân loại Tilemap Solid vs OneWay**:
  - **Khối Solid (Ground, Brick, QBlock, Pipe)**: Chặn đứng mọi chuyển động ngang và dọc của thực thể (sử dụng hàm `Tilemap::IsSolid`).
  - **Khối OneWay (3)**: Chỉ chặn chiều rơi xuống (`_vel_y > 0.0f` qua hàm `Tilemap::IsBlockingFall`). Người chơi có thể nhảy xuyên từ dưới lên hoặc đi qua ngang thân khối này bình thường, tạo ra các lối đi một chiều độc đáo.

---

### 3. Cơ chế Quản lý (Management Mechanisms)

Hệ thống sử dụng các bộ quản lý (Manager) tách biệt nhằm giảm thiểu sự phụ thuộc lẫn nhau:
- **[Game](file:///d:/GameDev1/Project1/src/game/Game.h)**: Đóng vai trò bộ điều phối (Orchestrator), quản lý Máy trạng thái trò chơi (Title, LevelIntro, Playing, Dying, GameOver, Victory, LevelComplete) và vòng lặp trò chơi chính.
- **[LevelManager](file:///d:/GameDev1/Project1/src/game/LevelManager.h)**: Lưu trữ và chuyển đổi giữa các định nghĩa màn chơi (LevelDef), chỉ định file map, toạ độ điểm bắt đầu và màu nền của từng màn.
- **[Tilemap](file:///d:/GameDev1/Project1/src/tilemap/Tilemap.h)**: Đọc sơ đồ màn chơi từ các tệp văn bản (như [level1.txt](file:///d:/GameDev1/Project1/assets/level1.txt)). Nó trực tiếp quản lý lưới ô gạch `Tile` chứa thuộc tính loại gạch (`type`) và cờ trạng thái bị đập phá (`used`).
- **[EnemyManager](file:///d:/GameDev1/Project1/src/game/EnemyManager.h)**: Đảm nhận việc spawn và quản lý vòng đời của kẻ địch. Nó xử lý trạng thái chiến đấu (như Goomba bị dẫm bẹp, Koopa co mai rùa rúc trong mai và trượt đi gây sát thương).
- **[SoundManager](file:///d:/GameDev1/Project1/src/core/SoundManager.h)**: Tích hợp DirectXTK / XAudio2 để load trước các tệp âm thanh `.wav` vào bộ nhớ đệm và chơi đa kênh không chặn (non-blocking channels).

---

## ⚡ Cơ chế Tối ưu hóa (Optimization)

Để duy trì tốc độ khung hình 60 FPS ổn định trên phần cứng yếu, dự án áp dụng các kỹ thuật tối ưu hóa sau:

### 1. Phân hoạch Không gian bằng Spatial Grid
- **Vấn đề**: Kiểm tra va chạm giữa tất cả các thực thể với nhau theo cách truyền thống tốn độ phức tạp $O(N^2)$.
- **Giải pháp**: Lớp [SpatialGrid](file:///d:/GameDev1/Project1/src/collision/SpatialGrid.h) chia màn chơi thành lưới ô vuông kích thước cố định (`CELL_SIZE = 32`). Thực thể chỉ đăng ký vào ô chứa nó. Khi kiểm tra va chạm, thực thể chỉ truy vấn các thực thể khác nằm chung ô hoặc ô lân cận.
- **Kết quả**: Giảm độ phức tạp từ $O(N^2)$ xuống gần như $O(N)$, giữ hiệu năng mượt mà kể cả khi màn chơi cực dài và nhiều quái vật.

### 2. Gom cụm bản vẽ Sprite Batching
- **Vấn đề**: Tạo ra một lệnh vẽ (Draw Call) riêng cho từng sprite sẽ làm nghẽn băng thông CPU-GPU (Draw Call Overhead).
- **Giải pháp**: Lớp [SpriteBatch](file:///d:/GameDev1/Project1/src/renderer/SpriteBatch.h) thu thập thông tin của tất cả thực thể cần vẽ trong một khung hình, gom chúng lại theo cấu trúc Vertex Buffer chung cho cùng một tấm Texture Atlas lớn (như `misc.png` hay `enemies.png`), và gửi toàn bộ dữ liệu lên GPU trong duy nhất một lần vẽ.

### 3. Hạn chế Cấp phát Động (Object Pool & Zero Allocation)
- Trong các vòng lặp xử lý chính (`Tick`), dự án không gọi `new` hoặc `delete` để tránh phân mảnh bộ nhớ và dọn rác (Garbage Collection spikes).
- Bộ đệm va chạm `CollisionEventPool` trong [CollisionSystem.h](file:///d:/GameDev1/Project1/src/collision/CollisionSystem.h) được cấp phát tĩnh một lần duy nhất khi khởi động trò chơi và tái sử dụng bộ nhớ liên tục trong mỗi frame.

### 4. Cơ chế QBlock không reset tự nhiên (Persistent State)
- Trạng thái `used` của Question Block được lưu trữ trực tiếp trong lưới mảng gạch tĩnh của [Tilemap](file:///d:/GameDev1/Project1/src/tilemap/Tilemap.h) thay vì các đối tượng động riêng biệt. Trạng thái này không bị nạp lại khi reset màn để duy trì bộ nhớ cache trạng thái tự nhiên, giúp tiết kiệm bộ nhớ quản lý.
