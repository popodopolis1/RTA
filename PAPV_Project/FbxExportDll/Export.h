#pragma once
#include "fbxsdk.h"
#include <fbxsdk.h>
#include <Windows.h>
#include <vector>
#include <DirectXMath.h>
#include <string>
#include <unordered_map>
#include <d3d11.h>

#ifdef FBXEXPORTDLL_EXPORTS
#define DLLEXPORT __declspec(dllexport)
#else
#define DLLEXPORT __declspec(dllimport)
#endif
using namespace DirectX;
using namespace std;


namespace DllExport
{
	struct BlendingIndexWeightPair
	{
		unsigned int mBlendingIndex;
		double mBlendingWeight;

		BlendingIndexWeightPair() :
			mBlendingIndex(0),
			mBlendingWeight(0)
		{}
	};

	struct CtrlPoint
	{
		XMFLOAT3 mPosition;
		vector<BlendingIndexWeightPair> mBlendingInfo;

		CtrlPoint()
		{
			mBlendingInfo.reserve(4);
		}
	};

	struct Keyframe
	{
		FbxLongLong mFrameNum;
		FbxAMatrix mGlobalTransform;
		Keyframe* mNext;

		Keyframe() :
			mNext(nullptr)
		{}
	};

	struct Joint
	{
		string mName;
		int mParentIndex;
		FbxAMatrix mGlobalBindposeInverse;
		Keyframe* mAnimation;
		FbxNode* mNode;

		Joint() :
			mNode(nullptr),
			mAnimation(nullptr)
		{
			mGlobalBindposeInverse.SetIdentity();
			mParentIndex = -1;
		}

		~Joint()
		{
			while (mAnimation)
			{
				Keyframe* temp = mAnimation->mNext;
				delete mAnimation;
				mAnimation = temp;
			}
		}
	};

	struct Skeleton
	{
		vector<Joint> mJoints;
	};

	struct Triangle
	{
		vector<unsigned int> mIndices;
		string mMaterialName;
		unsigned int mMaterialIndex;

		bool operator<(const Triangle& rhs)
		{
			return mMaterialIndex < rhs.mMaterialIndex;
		}
	};

	class DLLEXPORT Material
	{
	public:
		string mName;
		XMFLOAT3 mAmbient;
		XMFLOAT3 mDiffuse;
		XMFLOAT3 mEmissive;
		double mTransparencyFactor;
		string mDiffuseMapName;
		string mEmissiveMapName;
		string mGlossMapName;
		string mNormalMapName;
		string mSpecularMapName;
	};

	struct PNMVertex
	{
		XMFLOAT3 mPosition;
		XMFLOAT3 mNormal;
		XMFLOAT2 mUV;
	};

	class DLLEXPORT Export
	{
	public:
		Export();
		bool Initialize();
		bool LoadScene(const char* inFileName);
		FbxScene* getScene() { return mScene; }
		vector<PNMVertex> getVertices() { return mVerts; }
		void ProcessSkeletonHierarchy(FbxNode* inRootNode);
		void ProcessSkeletonHierarchyRecursively(FbxNode* inNode, int inDepth, int myIndex, int inParentIndex);
		void ProcessControlPoints(FbxNode* inNode);
		void ProcessMesh(FbxNode* inNode);
		FbxAMatrix GetGeometryTransforms(FbxNode* inNode);
		void ProcessJointAndAnimations(FbxNode* inNode);
		unsigned int FindJointIndexUsingName(const string& inJointName);
		void ReadUV(FbxMesh* inMesh, int inCtrlPointIndex, int inTextureUVIndex, int inUVLayer, XMFLOAT2& outUV);
		void ReadNormal(FbxMesh* inMesh, int inCtrlPointIndex, int inVertexCounter, XMFLOAT3& outNormal);
		void ReadBinormal(FbxMesh* inMesh, int inCtrlPointIndex, int inVertexCounter, XMFLOAT3& outBinormal);
		void ReadTangent(FbxMesh* inMesh, int inCtrlPointIndex, int inVertexCounter, XMFLOAT3& outTangent);
		Skeleton getSkelton() { return mSkeleton; }
	private:

		FbxManager* mFBXMan;
		FbxScene* mScene;
		string inputFilePath;
		string outputFilePath;
		bool hasAnimation;
		unordered_map<unsigned int, CtrlPoint*> mControlPoints;
		vector<XMFLOAT3> m_ControlVectors;
		unsigned int mTriCount;
		vector<Triangle> mTris;
		vector<PNMVertex> mVerts;
		Skeleton mSkeleton;
		unordered_map<unsigned int, Material*> mMaterialLookUp;
		FbxLongLong mAnimationLength;
		string mAnimationName;
		LARGE_INTEGER mCPUFreq;
	};
}



