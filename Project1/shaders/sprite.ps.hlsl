// Pixel Shader: chạy 1 lần per pixel
Texture2D    sprite_texture : register(t0);
SamplerState sprite_sampler : register(s0);

struct PSInput {
    float4 position : SV_Position;
    float2 uv       : TEXCOORD;
    float4 color    : COLOR;
};

float4 main(PSInput input) : SV_Target {
    // TODO: Trả về màu của pixel này
    float4 tex_color = sprite_texture.Sample(sprite_sampler, input.uv);
    return tex_color * input.color;
}

