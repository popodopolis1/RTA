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

struct JointVertex
{
	float x;
	float y;
	float z;
	float w;
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
		void FbxToBinary(const char* file, const char* outputFileName);
		std::vector<FBXData> BinaryToVerts(std::vector<FBXData> outVerts, const char* file);
		//std::vector<XMMATRIX> GetJoints(std::vector<XMMATRIX> outJoints, const char* file);
		std::vector<JointVertex> GetJoints(std::vector<JointVertex> outJoints, const char* file);
		std::vector<JointVertex> GetKeyframes(std::vector<JointVertex> outFrames, const char* file);
	};
}

