// Pixel Shader: chạy 1 lần per pixel
struct PSInput {
    float4 position : SV_Position;
    float4 color    : COLOR;
};

float4 main(PSInput input) : SV_Target {
    // TODO: Trả về màu của pixel này
    return input.color;
}

