struct INPUT
{
	float4 projCoord : SV_POSITION;
	float4 color : COLOR;
	float3 uvw : UVW;
};

TextureCube cubeTex;
//Texture2D tex;
SamplerState filter;

float4 main(INPUT input) : SV_TARGET
{
	return cubeTex.Sample(filter, input.uvw.xyz);
}