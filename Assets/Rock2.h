// "Rock2.h" generated by "Obj2Header.exe" [Version 1.9d] Author: L.Norri Fullsail University.
// Data is converted to left-handed coordinate system and UV data is V flipped for Direct3D/Vulkan.
// The companion file "Rock2.h2b" is a binary dump of this format(-padding) for more flexibility.
// Loading *.h2b: read version, sizes, handle strings(max len 260) by reading until null-terminated.
/***********************/
/*  Generator version  */
/***********************/
#ifndef _Rock2_version_
const char Rock2_version[4] = { '0','1','9','d' };
#define _Rock2_version_
#endif
/************************************************/
/*  This section contains the model's size data */
/************************************************/
#ifndef _Rock2_vertexcount_
const unsigned Rock2_vertexcount = 60;
#define _Rock2_vertexcount_
#endif
#ifndef _Rock2_indexcount_
const unsigned Rock2_indexcount = 60;
#define _Rock2_indexcount_
#endif
#ifndef _Rock2_materialcount_
const unsigned Rock2_materialcount = 1; // can be used for batched draws
#define _Rock2_materialcount_
#endif
#ifndef _Rock2_meshcount_
const unsigned Rock2_meshcount = 1;
#define _Rock2_meshcount_
#endif
/************************************************/
/*  This section contains the raw data to load  */
/************************************************/
#ifndef __OBJ_VEC3__
typedef struct _OBJ_VEC3_
{
	float x,y,z; // 3D Coordinate.
}OBJ_VEC3;
#define __OBJ_VEC3__
#endif
#ifndef __OBJ_VERT__
typedef struct _OBJ_VERT_
{
	OBJ_VEC3 pos; // Left-handed +Z forward coordinate w not provided, assumed to be 1.
	OBJ_VEC3 uvw; // D3D/Vulkan style top left 0,0 coordinate.
	OBJ_VEC3 nrm; // Provided direct from obj file, may or may not be normalized.
}OBJ_VERT;
#define __OBJ_VERT__
#endif
#ifndef _Rock2_vertices_
// Raw Vertex Data follows: Positions, Texture Coordinates and Normals.
const OBJ_VERT Rock2_vertices[60] =
{
	{	{ 0.348443f, 0.277310f, 0.219598f },	{ 0.000000f, 0.000000f, 0.000000f },	{ 0.028000f, 0.999600f, 0.010400f }	},
	{	{ 0.565671f, 0.273168f, 0.033805f },	{ 0.000000f, 0.000000f, 0.000000f },	{ 0.028000f, 0.999600f, 0.010400f }	},
	{	{ 0.246654f, 0.285791f, -0.320249f },	{ 0.000000f, 0.000000f, 0.000000f },	{ 0.028000f, 0.999600f, 0.010400f }	},
	{	{ 0.381297f, -0.016421f, -0.370617f },	{ 0.000000f, 0.000000f, 0.000000f },	{ 0.331500f, 0.296900f, -0.895500f }	},
	{	{ 0.035015f, 0.206382f, -0.424925f },	{ 0.000000f, 0.000000f, 0.000000f },	{ 0.331500f, 0.296900f, -0.895500f }	},
	{	{ 0.246654f, 0.285791f, -0.320249f },	{ 0.000000f, 0.000000f, 0.000000f },	{ 0.331500f, 0.296900f, -0.895500f }	},
	{	{ 0.381297f, -0.016421f, -0.370617f },	{ 0.000000f, 0.000000f, 0.000000f },	{ -0.007800f, -1.000000f, -0.000700f }	},
	{	{ 0.341627f, -0.016385f, 0.033250f },	{ 0.000000f, 0.000000f, 0.000000f },	{ -0.007800f, -1.000000f, -0.000700f }	},
	{	{ -0.518960f, -0.009676f, 0.041481f },	{ 0.000000f, 0.000000f, 0.000000f },	{ -0.007800f, -1.000000f, -0.000700f }	},
	{	{ 0.071776f, 0.401672f, 0.269787f },	{ 0.000000f, 0.000000f, 0.000000f },	{ 0.331200f, 0.390100f, 0.859100f }	},
	{	{ -0.018121f, -0.023634f, 0.497581f },	{ 0.000000f, 0.000000f, 0.000000f },	{ 0.331200f, 0.390100f, 0.859100f }	},
	{	{ 0.348443f, 0.277310f, 0.219598f },	{ 0.000000f, 0.000000f, 0.000000f },	{ 0.331200f, 0.390100f, 0.859100f }	},
	{	{ -0.209692f, 0.316202f, 0.044237f },	{ 0.000000f, 0.000000f, 0.000000f },	{ -0.635400f, 0.464600f, 0.616800f }	},
	{	{ -0.018121f, -0.023634f, 0.497581f },	{ 0.000000f, 0.000000f, 0.000000f },	{ -0.635400f, 0.464600f, 0.616800f }	},
	{	{ 0.071776f, 0.401672f, 0.269787f },	{ 0.000000f, 0.000000f, 0.000000f },	{ -0.635400f, 0.464600f, 0.616800f }	},
	{	{ -0.518960f, -0.009676f, 0.041481f },	{ 0.000000f, 0.000000f, 0.000000f },	{ -0.563200f, 0.529100f, 0.634700f }	},
	{	{ -0.018121f, -0.023634f, 0.497581f },	{ 0.000000f, 0.000000f, 0.000000f },	{ -0.563200f, 0.529100f, 0.634700f }	},
	{	{ -0.209692f, 0.316202f, 0.044237f },	{ 0.000000f, 0.000000f, 0.000000f },	{ -0.563200f, 0.529100f, 0.634700f }	},
	{	{ 0.341627f, -0.016385f, 0.033250f },	{ 0.000000f, 0.000000f, 0.000000f },	{ -0.008000f, -0.999700f, -0.021800f }	},
	{	{ -0.018121f, -0.023634f, 0.497581f },	{ 0.000000f, 0.000000f, 0.000000f },	{ -0.008000f, -0.999700f, -0.021800f }	},
	{	{ -0.518960f, -0.009676f, 0.041481f },	{ 0.000000f, 0.000000f, 0.000000f },	{ -0.008000f, -0.999700f, -0.021800f }	},
	{	{ 0.348443f, 0.277310f, 0.219598f },	{ 0.000000f, 0.000000f, 0.000000f },	{ 0.735600f, -0.375000f, 0.564100f }	},
	{	{ -0.018121f, -0.023634f, 0.497581f },	{ 0.000000f, 0.000000f, 0.000000f },	{ 0.735600f, -0.375000f, 0.564100f }	},
	{	{ 0.341627f, -0.016385f, 0.033250f },	{ 0.000000f, 0.000000f, 0.000000f },	{ 0.735600f, -0.375000f, 0.564100f }	},
	{	{ -0.038782f, -0.008745f, -0.568433f },	{ 0.000000f, 0.000000f, 0.000000f },	{ -0.649600f, 0.563400f, -0.510500f }	},
	{	{ -0.518960f, -0.009676f, 0.041481f },	{ 0.000000f, 0.000000f, 0.000000f },	{ -0.649600f, 0.563400f, -0.510500f }	},
	{	{ 0.035015f, 0.206382f, -0.424925f },	{ 0.000000f, 0.000000f, 0.000000f },	{ -0.649600f, 0.563400f, -0.510500f }	},
	{	{ 0.246654f, 0.285791f, -0.320249f },	{ 0.000000f, 0.000000f, 0.000000f },	{ -0.191500f, 0.928800f, -0.317300f }	},
	{	{ 0.035015f, 0.206382f, -0.424925f },	{ 0.000000f, 0.000000f, 0.000000f },	{ -0.191500f, 0.928800f, -0.317300f }	},
	{	{ -0.209692f, 0.316202f, 0.044237f },	{ 0.000000f, 0.000000f, 0.000000f },	{ -0.191500f, 0.928800f, -0.317300f }	},
	{	{ 0.381297f, -0.016421f, -0.370617f },	{ 0.000000f, 0.000000f, 0.000000f },	{ -0.012800f, -0.999900f, -0.011600f }	},
	{	{ -0.518960f, -0.009676f, 0.041481f },	{ 0.000000f, 0.000000f, 0.000000f },	{ -0.012800f, -0.999900f, -0.011600f }	},
	{	{ -0.038782f, -0.008745f, -0.568433f },	{ 0.000000f, 0.000000f, 0.000000f },	{ -0.012800f, -0.999900f, -0.011600f }	},
	{	{ -0.209692f, 0.316202f, 0.044237f },	{ 0.000000f, 0.000000f, 0.000000f },	{ -0.114400f, 0.967900f, -0.224000f }	},
	{	{ 0.071776f, 0.401672f, 0.269787f },	{ 0.000000f, 0.000000f, 0.000000f },	{ -0.114400f, 0.967900f, -0.224000f }	},
	{	{ 0.246654f, 0.285791f, -0.320249f },	{ 0.000000f, 0.000000f, 0.000000f },	{ -0.114400f, 0.967900f, -0.224000f }	},
	{	{ 0.246654f, 0.285791f, -0.320249f },	{ 0.000000f, 0.000000f, 0.000000f },	{ 0.400000f, 0.914500f, -0.061100f }	},
	{	{ 0.071776f, 0.401672f, 0.269787f },	{ 0.000000f, 0.000000f, 0.000000f },	{ 0.400000f, 0.914500f, -0.061100f }	},
	{	{ 0.348443f, 0.277310f, 0.219598f },	{ 0.000000f, 0.000000f, 0.000000f },	{ 0.400000f, 0.914500f, -0.061100f }	},
	{	{ 0.565671f, 0.273168f, 0.033805f },	{ 0.000000f, 0.000000f, 0.000000f },	{ 0.923600f, -0.059100f, -0.378700f }	},
	{	{ 0.533281f, -0.019018f, 0.000422f },	{ 0.000000f, 0.000000f, 0.000000f },	{ 0.923600f, -0.059100f, -0.378700f }	},
	{	{ 0.381297f, -0.016421f, -0.370617f },	{ 0.000000f, 0.000000f, 0.000000f },	{ 0.923600f, -0.059100f, -0.378700f }	},
	{	{ 0.341627f, -0.016385f, 0.033250f },	{ 0.000000f, 0.000000f, 0.000000f },	{ -0.014000f, -0.999900f, -0.001300f }	},
	{	{ 0.381297f, -0.016421f, -0.370617f },	{ 0.000000f, 0.000000f, 0.000000f },	{ -0.014000f, -0.999900f, -0.001300f }	},
	{	{ 0.533281f, -0.019018f, 0.000422f },	{ 0.000000f, 0.000000f, 0.000000f },	{ -0.014000f, -0.999900f, -0.001300f }	},
	{	{ 0.341627f, -0.016385f, 0.033250f },	{ 0.000000f, 0.000000f, 0.000000f },	{ 0.576400f, -0.447300f, 0.683900f }	},
	{	{ 0.565671f, 0.273168f, 0.033805f },	{ 0.000000f, 0.000000f, 0.000000f },	{ 0.576400f, -0.447300f, 0.683900f }	},
	{	{ 0.348443f, 0.277310f, 0.219598f },	{ 0.000000f, 0.000000f, 0.000000f },	{ 0.576400f, -0.447300f, 0.683900f }	},
	{	{ 0.035015f, 0.206382f, -0.424925f },	{ 0.000000f, 0.000000f, 0.000000f },	{ -0.636500f, 0.608100f, -0.474400f }	},
	{	{ -0.518960f, -0.009676f, 0.041481f },	{ 0.000000f, 0.000000f, 0.000000f },	{ -0.636500f, 0.608100f, -0.474400f }	},
	{	{ -0.209692f, 0.316202f, 0.044237f },	{ 0.000000f, 0.000000f, 0.000000f },	{ -0.636500f, 0.608100f, -0.474400f }	},
	{	{ -0.038782f, -0.008745f, -0.568433f },	{ 0.000000f, 0.000000f, 0.000000f },	{ 0.394200f, 0.412600f, -0.821200f }	},
	{	{ 0.035015f, 0.206382f, -0.424925f },	{ 0.000000f, 0.000000f, 0.000000f },	{ 0.394200f, 0.412600f, -0.821200f }	},
	{	{ 0.381297f, -0.016421f, -0.370617f },	{ 0.000000f, 0.000000f, 0.000000f },	{ 0.394200f, 0.412600f, -0.821200f }	},
	{	{ 0.565671f, 0.273168f, 0.033805f },	{ 0.000000f, 0.000000f, 0.000000f },	{ 0.686000f, 0.406200f, -0.603600f }	},
	{	{ 0.381297f, -0.016421f, -0.370617f },	{ 0.000000f, 0.000000f, 0.000000f },	{ 0.686000f, 0.406200f, -0.603600f }	},
	{	{ 0.246654f, 0.285791f, -0.320249f },	{ 0.000000f, 0.000000f, 0.000000f },	{ 0.686000f, 0.406200f, -0.603600f }	},
	{	{ 0.533281f, -0.019018f, 0.000422f },	{ 0.000000f, 0.000000f, 0.000000f },	{ 0.165700f, -0.130100f, 0.977600f }	},
	{	{ 0.565671f, 0.273168f, 0.033805f },	{ 0.000000f, 0.000000f, 0.000000f },	{ 0.165700f, -0.130100f, 0.977600f }	},
	{	{ 0.341627f, -0.016385f, 0.033250f },	{ 0.000000f, 0.000000f, 0.000000f },	{ 0.165700f, -0.130100f, 0.977600f }	},
};
#define _Rock2_vertices_
#endif
#ifndef _Rock2_indices_
// Index Data follows: Sequential by mesh, Every Three Indices == One Triangle.
const unsigned int Rock2_indices[60] =
{
	 0, 1, 2,
	 3, 4, 5,
	 6, 7, 8,
	 9, 10, 11,
	 12, 13, 14,
	 15, 16, 17,
	 18, 19, 20,
	 21, 22, 23,
	 24, 25, 26,
	 27, 28, 29,
	 30, 31, 32,
	 33, 34, 35,
	 36, 37, 38,
	 39, 40, 41,
	 42, 43, 44,
	 45, 46, 47,
	 48, 49, 50,
	 51, 52, 53,
	 54, 55, 56,
	 57, 58, 59,
};
#define _Rock2_indices_
#endif
// Part of an OBJ_MATERIAL, the struct is 16 byte aligned so it is GPU register friendly.
#ifndef __OBJ_ATTRIBUTES__
typedef struct _OBJ_ATTRIBUTES_
{
	OBJ_VEC3    Kd; // diffuse reflectivity
	float	    d; // dissolve (transparency) 
	OBJ_VEC3    Ks; // specular reflectivity
	float       Ns; // specular exponent
	OBJ_VEC3    Ka; // ambient reflectivity
	float       sharpness; // local reflection map sharpness
	OBJ_VEC3    Tf; // transmission filter
	float       Ni; // optical density (index of refraction)
	OBJ_VEC3    Ke; // emissive reflectivity
	unsigned    illum; // illumination model
}OBJ_ATTRIBUTES;
#define __OBJ_ATTRIBUTES__
#endif
// This structure also has been made GPU register friendly so it can be transfered directly if desired.
// Note: Total struct size will vary depenedening on CPU processor architecture (string pointers).
#ifndef __OBJ_MATERIAL__
typedef struct _OBJ_MATERIAL_
{
	// The following items are typically used in a pixel/fragment shader, they are packed for GPU registers.
	OBJ_ATTRIBUTES attrib; // Surface shading characteristics & illumination model
	// All items below this line are not needed on the GPU and are generally only used during load time.
	const char* name; // the name of this material
	// If the model's materials contain any specific texture data it will be located below.
	const char* map_Kd; // diffuse texture
	const char* map_Ks; // specular texture
	const char* map_Ka; // ambient texture
	const char* map_Ke; // emissive texture
	const char* map_Ns; // specular exponent texture
	const char* map_d; // transparency texture
	const char* disp; // roughness map (displacement)
	const char* decal; // decal texture (lerps texture & material colors)
	const char* bump; // normal/bumpmap texture
	void* padding[2]; // 16 byte alignment on 32bit or 64bit
}OBJ_MATERIAL;
#define __OBJ_MATERIAL__
#endif
#ifndef _Rock2_materials_
// Material Data follows: pulled from a .mtl file of the same name if present.
const OBJ_MATERIAL Rock2_materials[1] =
{
	{
		{{ 0.287072f, 0.287072f, 0.287072f },
		1.000000f,
		{ 0.500000f, 0.500000f, 0.500000f },
		96.078430f,
		{ 1.000000f, 1.000000f, 1.000000f },
		60.000000f,
		{ 1.000000f, 1.000000f, 1.000000f },
		1.000000f,
		{ 0.000000f, 0.000000f, 0.000000f },
		2},
		"Rock.001",
		"",
		"",
		"",
		"",
		"",
		"",
		"",
		"",
		"",
	},
};
#define _Rock2_materials_
#endif
/************************************************/
/*  This section contains the model's structure */
/************************************************/
#ifndef _Rock2_batches_
// Use this conveinence array to batch render all geometry by like material.
// Each entry corresponds to the same entry in the materials array above.
// The two numbers provided are the IndexCount and the IndexOffset into the indices array.
// If you need more fine grained control(ex: for transformations) use the OBJ_MESH data below.
const unsigned int Rock2_batches[1][2] =
{
	{ 60, 0 },
};
#define _Rock2_batches_
#endif
#ifndef __OBJ_MESH__
typedef struct _OBJ_MESH_
{
	const char* name;
	unsigned    indexCount;
	unsigned    indexOffset;
	unsigned    materialIndex;
}OBJ_MESH;
#define __OBJ_MESH__
#endif
#ifndef _Rock2_meshes_
// Mesh Data follows: Meshes are .obj groups sorted & split by material.
// Meshes are provided in sequential order, sorted by material first and name second.
const OBJ_MESH Rock2_meshes[1] =
{
	{
		"default",
		60,
		0,
		0,
	},
};
#define _Rock2_meshes_
#endif
