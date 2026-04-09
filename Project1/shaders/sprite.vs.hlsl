// Vertex Shader: chạy 1 lần per vertex
// Input: position (x,y) trong clip space [-1, 1]
// Output: position truyền xuống Pixel Shader

struct VSInput {
    float2 position : POSITION;
    float2 uv       : TEXCOORD;
    float4 color    : COLOR;
};

struct VSOutput {
    float4 position : SV_Position; // SV_ = System Value, bắt buộc
    float2 uv       : TEXCOORD;
    float4 color    : COLOR;
};

cbuffer ScreenBuffer : register(b0) {
    float screen_width;
    float screen_height;
    float cam_x;
    float cam_y;
};

VSOutput main(VSInput input) {
    VSOutput output;
    
    float world_x = input.position.x - cam_x;
    float world_y = input.position.y - cam_y;

    // Convert pixel coords -> clip space
    float cx = (world_x / screen_width)  * 2.0f - 1.0f;
    float cy = (world_y / screen_height) * (-2.0f) + 1.0f;
    
    output.position = float4(cx, cy, 0.0f, 1.0f);
    output.color    = input.color;
    output.uv       = input.uv;
    
    return output;
}


