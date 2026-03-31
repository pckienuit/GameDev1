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
};

VSOutput main(VSInput input) {
    VSOutput output;
    
    // Convert pixel coords → clip space
    float cx = (input.position.x / screen_width)  * 2.0f - 1.0f;
    float cy = (input.position.y / screen_height) * (-2.0f) + 1.0f;
    
    output.position = float4(cx, cy, 0.0f, 1.0f);
    output.color    = input.color;
    output.uv       = input.uv;
    
    return output;
}


