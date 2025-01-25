
//Vertex Layout for texture mapping
struct VertexIn
{
	float3 pos : POSITION;
	float2 tex : TEXCOORD;
};

//PixelInput structure
struct PixelInput
{
	float4 pos : SV_POSITION;
	float2 tex : TEXCOORD;
};

//screen rect shader
PixelInput VSMain(VertexIn input)
{	
	PixelInput output = (PixelInput)0;
    output.pos = float4(input.pos, 1.0f);
	output.tex = input.tex;
	return output;
}