// Mirror scene data struct in shaders	(OBJ_ATTRIBUTES from C++)
#define MAX_SUBMESH_PER_DRAW 1024
struct OBJ_ATTRIBUTES
{
	float3	Kd;											// diffuse reflectivity
	float	d;											// dissolve (transparency) 
	float3  Ks;											// specular reflectivity
	float   Ns;											// specular exponent
	float3  Ka;											// ambient reflectivity
	float   sharpness;									// local reflection map sharpness
	float3  Tf;											// transmission filter
	float   Ni;											// optical density (index of refraction)
	float3  Ke;											// emissive reflectivity
	uint	illum;										// illumination model
};
	
struct SHADER_MODEL_DATA								// Mirror SHADER_MODEL_DATA from C++
{
    float3 sunDirection, sunColor, sunAmbient, camPos,  // Light info
			pointPos, pointCol;									
	matrix viewMatrix, projMatrix;						// view info
	matrix matricies[MAX_SUBMESH_PER_DRAW];				// world space transforms
	OBJ_ATTRIBUTES materials[MAX_SUBMESH_PER_DRAW];		// color/texture of surface
};

// Add a structured buffer for scene data
StructuredBuffer<SHADER_MODEL_DATA> SceneData;

// To get push constants to work in HLSL you have to prepend to a cbuffer
[[vk::push_constant]]
cbuffer MESH_INDEX
{
	// now the bytes that were uploaded by the push constant command should overwrite this buffer
	uint meshID;
};

struct V_OUT
{
    float4 projectedPos : SV_POSITION;
    float3 viewDir      : TEXCOORD1;
    float3 norm			: NORMAL;			// normal in world space, for lighting
    float3 posW			: WORLD;			// position in world space, for lighting
};

float4 main(V_OUT input) : SV_TARGET 
{	
	// DIFFUSE
	// For lambertian, we need the dot product between the surface norm and direction to light(-lightDir), as well as the light ratio
	float4 diffuseColor = float4(SceneData[0].materials[meshID].Kd.xyz, 1.0f);						// diffuse color if the material (surfaceColor, fragColor)
	float3 surfaceNorm	= normalize(input.norm);													// re-normalize input norm
    float lightRatio	= saturate(dot(surfaceNorm, -SceneData[0].sunDirection.xyz));				// Get light ratio (direct light)
	
	// lambertian shading = ratio * light color * surface color
	float4 diffuseLight = lightRatio * diffuseColor * float4(SceneData[0].sunColor.xyz, 1);			// suncolor and direction must be float4
	
	// AMBIENT LIGHT (indirect/bounced light)
	float4 ambientTerm	= float4(SceneData[0].sunAmbient.xyz, 1);
    float4 ambientLight = saturate(lightRatio * float4(SceneData[0].sunColor.xyz, 1) + ambientTerm) * diffuseColor; // diffuse with ambient lighting
	
	// SPECULAR
	float3 viewDir		= normalize(SceneData[0].camPos - input.posW);								// eye (view) direction vector
	float3 halfVec		= normalize((-SceneData[0].sunDirection.xyz) + viewDir);
    float4 gloss		= float4(SceneData[0].materials[meshID].Ks, 1.0);
    float specPow		= SceneData[0].materials[meshID].Ns;
	
	// Compute intensity
    float3 intensity	= max(pow(saturate(dot(surfaceNorm, halfVec)), specPow), 0.0f);
   
	// Combine it all: light color * shininess * intensity
    float4 specular		= float4(SceneData[0].sunColor, 1) * gloss * float4(intensity, 1);
	
	// Point light
    float4 pLight = (0);
    float pLightRatio = 0.0f;
    float pIntensity = 25.0f;
	
    if (SceneData[0].pointPos.y != 0)
    {
        float pLightRadius	= 5.0f;
        float3 pLightDir	= normalize(SceneData[0].pointPos - input.posW);									// calculate direction to light source from surface
        pLightRatio			= saturate(dot(pLightDir, surfaceNorm));
        float atten			= 1 - saturate(length(SceneData[0].pointPos - input.posW) / pLightRadius);			// multiply this by amount of light reaching the surface
        pLight = float4((atten * pIntensity * pLightRatio) * SceneData[0].pointCol.xyz * diffuseColor.xyz, 1);	// lightRatio * lightColor * surfaceColor
    }     
    return ambientLight + specular + pLight;
}