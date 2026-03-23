// vertex
struct VsInput
{
    float3 position : POSITION;
    float2 uv : TEXCOORD0;
};

struct VsOutput
{
    float4 positon : SV_Position;
    float2 uv : TEXCOORD0;
};


VsOutput VsMain(VsInput input)
{
    VsOutput output = (VsOutput) 0;
    float4 pos = float4(input.position, 1.0f);
    output.positon = pos;
    output.uv = input.uv;
    return output;
}

// fragment
sampler LinearSampler : register(s0);
Texture2D Texture : register(t0);

float4 PsMain(VsOutput input) : SV_Target
{
    float4 tixel = Texture.Sample(LinearSampler, input.uv);
    return tixel;
}