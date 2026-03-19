// vertex
struct VsInput
{
    float3 position : POSITION;
    float3 color : COLOR0;
};

struct VsOutput
{
    float4 positon : SV_Position;
    float3 color : COLOR0;
};

[shader("vertex")]
VsOutput VsMain(VsInput input)
{
    VsOutput output = (VsOutput) 0;
    output.positon = float4(input.position, 1.0f);
    output.color = input.color;
    return output;
}

struct PsOutPut
{
    float4 color : SV_Target0;
};

// fragment
[shader("fragment")]
PsOutPut PsMain(VsOutput input)
{
    PsOutPut output;
    output.color = float4(input.color, 1.0);
    return output;
}