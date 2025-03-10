cbuffer cbWorld : register(b0) //Vertex Shader constant buffer slot 0 - matches slot in vsBilboard.hlsl
{
	matrix worldMatrix;
};

cbuffer cbView : register(b1) //Vertex Shader constant buffer slot 1 - matches slot in vsBilboard.hlsl
{
	matrix viewMatrix;
	matrix invViewMatrix;
};

cbuffer cbProj : register(b2) //Vertex Shader constant buffer slot 2 - matches slot in vsBilboard.hlsl
{
	matrix projMatrix;
};

//DONE : 0.4. Change vertex shader input structure to match new vertex type
struct VSInput
{
	float3 pos : POSITION;
    float3 normal : NORMAL;
};

//DONE : 0.5. Change vertex shader output structure to store position, normal and view vectors in global coordinates
struct PSInput
{
	float4 pos : SV_POSITION;
    float4 globalPos : POSITION;
    float4 normal : NORMAL;
    float4 view : VIEW;
};

PSInput main(VSInput i)
{
	PSInput o;
	float4 pos = mul(worldMatrix, float4(i.pos, 1.0f));

	//DONE : 0.6. Store global (world) position in the output
    o.globalPos = pos;
	pos = mul(viewMatrix, pos);
	o.pos = mul(projMatrix, pos);

	//DONE : 0.7. Calculate output normal and view vectors in global coordinates frame
	//Hint: you can calculate camera position from inverted view matrix
    o.normal = normalize(mul(worldMatrix, float4(i.normal, 0.f)));
    o.view = normalize(mul(invViewMatrix, float4(0.f, 0.f, 0.f, 1.f)));

	return o;
}