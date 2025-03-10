struct Light
{
	float4 position;
	float4 color;
};

struct Lighting
{
	float4 ambient;
	float4 surface;
	Light lights[3];
};

cbuffer cbSurfaceColor : register(b0) //Pixel Shader constant buffer slot 0 - matches slot in psBilboard.hlsl
{
	float4 surfaceColor;
}

cbuffer cbLighting : register(b1) //Pixel Shader constant buffer slot 1
{
	Lighting lighting;
}

//TODO : 0.8. Modify pixel shader input structure to match vertex shader output
struct PSInput
{
	float4 pos : SV_POSITION;
	float4 col : COLOR;
};

float4 main(PSInput i) : SV_TARGET
{
	//TODO : 0.9. Calculate output color using Phong Illumination Model

	return i.col; // Replace with correct implementation
}