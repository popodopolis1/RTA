#pragma pack_matrix(row_major)

struct IN
{
	float4 posIn : POSITION;
	float4 colorIn : COLOR;
	float3 uvIn : UVW;
};

struct OUT
{
	float4 posOut : SV_POSITION;
	float4 colorOut : COLOR;
	float3 uvOut : UVW;
};

cbuffer WORLD : register(b0)
{
	float4x4 WorldMatrix;
}

cbuffer OBJECT : register(b1)
{
	float4x4 ViewMatrix;
	float4x4 ProjectionMatrix;
}

OUT main( IN input )
{
	OUT output = (OUT)0;
	// ensures translation is preserved during matrix multiply  
	float4 localH = float4(input.posIn.xyz,1);
		// move local space vertex from vertex buffer into world space.
		localH = mul(localH, WorldMatrix);

	// TODO: Move into view space, then projection space

	localH = mul(localH, ViewMatrix);
	localH = mul(localH, ProjectionMatrix);
	output.posOut = localH;
	output.colorOut = input.colorIn;
	output.uvOut = input.posIn.xyz;

	return output; // send projected vertex to the rasterizer stage
}