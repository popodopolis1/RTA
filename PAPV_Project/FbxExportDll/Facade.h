#pragma once

#include <vector>
#ifdef FBXEXPORTDLL_EXPORTS
#define DLLEXPORT1 __declspec(dllexport)
#else
#define DLLEXPORT1 __declspec(dllimport)
#endif

struct Vertex
{
	float x;
	float y;
	float z;
};

struct Normal
{
	float x;
	float y;
	float z;
};

struct UV
{
	float u;
	float v;
};

struct FBXData
{
	Vertex verts;
	Normal norms;
	UV uvs;
};

namespace FBXE
{
	class DLLEXPORT1 Facade
	{
	public:
		bool succeded = false;
		std::vector<FBXData> LoadFBX(std::vector<FBXData> outVerts, const char* file);
	};
}

