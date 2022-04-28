#pragma pack_matrix(row_major)

// Mirror scene data struct in shaders	(OBJ_ATTRIBUTES from c++)
#define MAX_SUBMESH_PER_DRAW 1024
struct OBJ_ATTRIBUTES
{
    float3  Kd;                                         // diffuse reflectivity
    float   d;                                          // dissolve (transparency) 
    float3  Ks;                                         // specular reflectivity
    float   Ns;                                         // specular exponent
    float3  Ka;                                         // ambient reflectivity
    float   sharpness;                                  // local reflection map sharpness
    float3  Tf;                                         // transmission filter
    float   Ni;                                         // optical density (index of refraction)
    float3  Ke;                                         // emissive reflectivity
    uint    illum;                                      // illumination model
};
	
struct SHADER_MODEL_DATA							    // Mirror SHADER_MODEL_DATA from C++
{
    float3 sunDirection, sunColor, sunAmbient, camPos,  // lighting info
            pointPos, pointCol;
    matrix viewMatrix, projMatrix;                      // View and projection matrices

    matrix matricies[MAX_SUBMESH_PER_DRAW];             // world space transforms
    OBJ_ATTRIBUTES materials[MAX_SUBMESH_PER_DRAW];     // color/texture of surface
};

// create structured buffer
StructuredBuffer<SHADER_MODEL_DATA> SceneData;

// To get push constants to work in HLSL you have to prepend to a cbuffer
[[vk::push_constant]]
cbuffer MESH_INDEX
{
	// now the bytes that were uploaded by the push constant command should overwrite this buffer
	uint meshID; // should always be 0
    // uint materialID to be added in the future
};
 
// Adjust vertex shader to take in Position, UV, and Normal, and tweak output in main()
struct V_IN
{
    float3 localPos     : POSITION;
    float3 tex          : TEXCOORD0;
    float3 norm         : NORMAL;
};

// Adjust output so it outputs V_OUT struct
struct V_OUT
{
    float4 projectedPos : SV_POSITION;
    float3 tex          : TEXCOORD1;
    float3 norm			: NORMAL;			// normal in world space, for lighting
    float3 posW			: WORLD;			// position in world space, for lighting
};

V_OUT main(V_IN inputVertex)
{
    V_OUT output = (V_OUT) 0;
    
	// multiply stuff , set matrices to meshID
    output.projectedPos = mul(float4(inputVertex.localPos, 1), SceneData[0].matricies[0]);
	output.posW			= output.projectedPos.xyz;	// Save the normal's world position before it gets moved into view/projection space (for normals)
    output.projectedPos = mul(output.projectedPos, SceneData[0].viewMatrix);
	output.projectedPos = mul(output.projectedPos, SceneData[0].projMatrix); 
	output.tex		    = inputVertex.tex;

	//  Get normal into world space
	output.norm			= mul(float4(inputVertex.norm, 0), SceneData[0].matricies[0]).xyz;		// output normal = inputNorm * world (putting it into world space)

    return output;
}