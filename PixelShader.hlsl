// TODO: Part 2b -- Mirror scene data struct in shaders	(OBJ_ATTRIBUTES from C++)
#define MAX_SUBMESH_PER_DRAW 1024
struct OBJ_ATTRIBUTES
{
	float3    Kd;										// diffuse reflectivity
	float	  d;										// dissolve (transparency) 
	float3    Ks;										// specular reflectivity
	float     Ns;										// specular exponent
	float3    Ka;										// ambient reflectivity
	float     sharpness;								// local reflection map sharpness
	float3    Tf;										// transmission filter
	float     Ni;										// optical density (index of refraction)
	float3    Ke;										// emissive reflectivity
	uint	  illum;									// illumination model
};
	
struct SHADER_MODEL_DATA								// Mirror SHADER_MODEL_DATA from C++
{
	float3 sunDirection, sunColor, sunAmbient, camPos;						// light info
	matrix viewMatrix, projMatrix;						// view info

	matrix matricies[MAX_SUBMESH_PER_DRAW];				// world space transforms
	OBJ_ATTRIBUTES materials[MAX_SUBMESH_PER_DRAW];		// color/texture of surface
};

// TODO: Part 2i -- Add a structured buffer for scene data
StructuredBuffer<SHADER_MODEL_DATA> SceneData;

// TODO: Part 3e -- To get push constants to work in HLSL you have to prepend to a cbuffer
[[vk::push_constant]]
cbuffer MESH_INDEX
{
	// now the bytes that were uploaded by the push constant command should overwrite this buffer
	uint meshID;
};

// TODO: Part 4b
struct V_OUT
{
    float4 projectedPos : SV_POSITION;
    float3 viewDir      : TEXCOORD1;
    float3 norm			: NORMAL;			// normal in world space, for lighting
    float3 posW			: WORLD;			// position in world space, for lighting
};

// actual main
float4 main(V_OUT input) : SV_TARGET 
{	
		// TODO: Part 3a
	//return float4(SceneData[0].materials[meshID].Kd.xyz, 1.0f);

	// TODO: Part 4c -- diffuse shading
	// For lambertian, we need the dot product between the surface norm and direction to light(aka -lightdir), as well as the light ratio
	float4 diffuseColor = float4(SceneData[0].materials[meshID].Kd.xyz, 1.0f);						// diffuse color if the material (surfaceColor, fragColor)
	float3 surfaceNorm	= normalize(input.norm);													// re-normalize input norm
    float lightRatio	= saturate(dot(surfaceNorm, -SceneData[0].sunDirection.xyz));				// Get light ratio (direct light)
	
	// lambertian shading = ratio * light color * surface color
	float4 diffuseLight = lightRatio * diffuseColor * float4(SceneData[0].sunColor.xyz, 1);			// suncolor and direction must be float4
	
	// TODO: Part 4g -- Calculate ambient light (indirect/bounced light)
	float4 ambientTerm	= float4(SceneData[0].sunAmbient.xyz, 1);
    float4 ambientLight = saturate(lightRatio * float4(SceneData[0].sunColor.xyz, 1) + ambientTerm) * diffuseColor; // final output of diffuse with ambient lighting
	
	// 4g -- calculate specular component (half-vector or reflect method your choice)
	float3 viewDir		= normalize(SceneData[0].camPos - input.posW);								// eye (view) direction vector
	float3 halfVec		= normalize((-SceneData[0].sunDirection.xyz) + viewDir);
    float4 gloss		= float4(SceneData[0].materials[meshID].Ks, 1.0);
    float specPow		= SceneData[0].materials[meshID].Ns;
	
	// compute intensity
    float3 intensity	= max(pow(saturate(dot(surfaceNorm, halfVec)), specPow), 0.0f);
   
	// combine it all: light color * shininess * intensity
    float4 specular = float4(SceneData[0].sunColor, 1) * gloss * float4(intensity, 1);
	
    return ambientLight + specular;
}