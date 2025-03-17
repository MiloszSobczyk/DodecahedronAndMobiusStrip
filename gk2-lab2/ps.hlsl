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
    float4 pos_global : POSITION;
    float4 normal : NORMAL;
    float4 view : VIEW;

};

float4 calculatePhong(float4 surface, Light light, PSInput i)
{
    float surf1 = surface[1];
    float4 kd = surf1 * surfaceColor;
    float ks = surface[2];
    float m = surface[3];
    
    // diffuse
    float4 lightDir = normalize(light.position - i.pos_global);
    float4 diff = max(dot(i.normal, lightDir), 0.0f);

    // specular
    float4 viewDir = normalize(i.view - i.pos_global);
    float4 reflectDir = normalize(reflect(-lightDir, i.normal));
    
    float spec1 = pow(max(dot(viewDir, reflectDir), 0.0f), m);
    
    return saturate(light.color * kd * diff + light.color * ks * spec1);
}

float4 main(PSInput i) : SV_TARGET
{
    //TODO : 0.9. Calculate output color using Phong Illumination Model
    float4 color = lighting.ambient * lighting.surface[0];
    for (int j = 0; j < 3; j++)
    {
        color += calculatePhong(lighting.surface, lighting.lights[j], i);
    }

    return color;
}