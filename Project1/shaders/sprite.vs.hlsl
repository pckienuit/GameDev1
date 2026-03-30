// Vertex Shader: chạy 1 lần per vertex
// Input: position (x,y) trong clip space [-1, 1]
// Output: position truyền xuống Pixel Shader

struct VSInput {
    float2 position : POSITION;
    float4 color    : COLOR;
};

struct VSOutput {
    float4 position : SV_Position; // SV_ = System Value, bắt buộc
    float4 color    : COLOR;
};

VSOutput main(VSInput input) {
    VSOutput output;
    // TODO: Gán output.position từ input.position
    // Gợi ý: float4(x, y, z, w) — z=0 vì 2D, w=1 cho perspective divide
    output.position = float4(input.position.x, input.position.y, 0.0f, 1.0f);
    output.color    = input.color;
    return output;
}
