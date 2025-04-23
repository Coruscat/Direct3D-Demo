cbuffer DataFromCPU : register(b0)
{
    float4x4 viewMat;
    float4x4 projMat;
};

struct VertexShaderInput
{
    float3 localPosition : POSITION; 
    float3 normal : NORMAL;
    float3 tangent : TANGENT;
    float2 uv : TEXCOORD;
};

struct VertexToPixel
{
    float4 position : SV_POSITION;
    float3 sampleDir : DIRECTION;
};

VertexToPixel main( VertexShaderInput input ) 
{
    VertexToPixel output; 
    
    float4x4 viewNoTranslate = viewMat;
    viewNoTranslate._14 = 0;
    viewNoTranslate._24 = 0;
    viewNoTranslate._34 = 0;
    
    float4 worldPos = float4(input.localPosition, 1.0);
    matrix vp = mul(projMat, viewNoTranslate);
    
    output.position = mul(vp , worldPos);
    
    output.position.z = output.position.w;
    
    output.sampleDir = input.localPosition;
    
	return output;
}