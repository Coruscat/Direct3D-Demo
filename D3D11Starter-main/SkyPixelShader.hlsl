TextureCube textureCube : register(t0);
SamplerState sampleState : register(s0);

struct VertexToPixel
{
    float4 position : SV_POSITION;
    float3 sampleDir : DIRECTION;
};

float4 main( VertexToPixel input ) : SV_TARGET
{
    return textureCube.Sample(sampleState, input.sampleDir);
}