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

//DONE : 0.8. Modify pixel shader input structure to match vertex shader output
struct PSInput
{
    float4 pos : SV_POSITION;
    float globalPos : POSITION;
    float4 normal : NORMAL;
    float4 view : VIEW;
};

float4 main(PSInput input) : SV_TARGET
{
	//DONE : 0.9. Calculate output color using Phong Illumination Model
	
    float ka = lighting.surface[0];
    float4 kd = lighting.surface[1] * surfaceColor;
    float ks = lighting.surface[2];
    float m = lighting.surface[3];
    
    // ambient
    float4 col = lighting.ambient * ka;
    
    for (int i = 0; i < 3; ++i)
    {
        float4 n = input.normal;
        float4 l = normalize(lighting.lights[i].position - input.globalPos);
        float4 v = normalize(input.view);
        float4 r = normalize(reflect(-v, n));
              
        col += lighting.lights[i].color * kd * (n * l);
        col += lighting.lights[i].color * ks * (pow(r * v, m));
    }
	
    return saturate(col); // Replace with correct implementation
}