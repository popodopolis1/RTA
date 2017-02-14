#include "Export.h"
#include <fstream>
#include <sstream>
#include <iomanip>

namespace DllExport
{
	Export::Export()
	{
		mFBXMan = nullptr;
		mScene = nullptr;
		mTriCount = 0;
		hasAnimation = true;
		QueryPerformanceFrequency(&mCPUFreq);
	}

	bool Export::Initialize()
	{
		mFBXMan = FbxManager::Create();
		if (!mFBXMan)
		{
			return false;
		}

		FbxIOSettings* IOSettings = FbxIOSettings::Create(mFBXMan, IOSROOT);
		mFBXMan->SetIOSettings(IOSettings);

		mScene = FbxScene::Create(mFBXMan, "myScene");
		return true;
	}

	bool Export::LoadScene(const char * inFileName)
	{
		LARGE_INTEGER start;
		LARGE_INTEGER end;
		inputFilePath = inFileName;

		QueryPerformanceCounter(&start);
		FbxImporter* import = FbxImporter::Create(mFBXMan, "importer");

		if (!import)
		{
			return false;
		}
		if (!import->Initialize(inFileName, -1, mFBXMan->GetIOSettings()))
		{
			return false;
		}
		if (!import->Import(mScene))
		{
			return false;
		}

		import->Destroy();
		QueryPerformanceCounter(&end);
		return true;
	}

	void Export::ProcessSkeletonHierarchy(FbxNode * inRootNode)
	{
		for (int childIndex = 0; childIndex < inRootNode->GetChildCount(); ++childIndex)
		{
			FbxNode* node = inRootNode->GetChild(childIndex);
			ProcessSkeletonHierarchyRecursively(node, 0, 0, -1);
		}
	}

	void Export::ProcessSkeletonHierarchyRecursively(FbxNode * inNode, int inDepth, int myIndex, int inParentIndex)
	{
		if (inNode->GetNodeAttribute() && inNode->GetNodeAttribute()->GetAttributeType() && inNode->GetNodeAttribute()->GetAttributeType() == FbxNodeAttribute::eSkeleton)
		{
			Joint joint;
			joint.mParentIndex = inParentIndex;
			joint.mName = inNode->GetName();
			mSkeleton.mJoints.push_back(joint);
		}
		for (unsigned int i = 0; i < inNode->GetChildCount(); i++)
		{
			ProcessSkeletonHierarchyRecursively(inNode->GetChild(i), inDepth + 1, mSkeleton.mJoints.size(), myIndex);
		}
	}

	void Export::ProcessControlPoints(FbxNode * inNode)
	{
		FbxMesh* mesh = inNode->GetMesh();
		if (mesh == nullptr)
		{
			int track = 0;
			track = inNode->GetChildCount(false);
			mesh = inNode->GetChild(0)->GetMesh();
		}

		unsigned int pointCount = 0;
		pointCount = mesh->GetControlPointsCount();
		for (unsigned int i = 0; i < pointCount; ++i)
		{
			CtrlPoint* ctrlPoint = new CtrlPoint();
			XMFLOAT3 pos;
			pos.x = static_cast<float>(mesh->GetControlPointAt(i).mData[0]);
			pos.y = static_cast<float>(mesh->GetControlPointAt(i).mData[1]);
			pos.z = static_cast<float>(mesh->GetControlPointAt(i).mData[2]);
			ctrlPoint->mPosition = pos;
			m_ControlVectors.push_back(pos);
			mControlPoints[i] = ctrlPoint;
		}
	}

	void Export::ProcessMesh(FbxNode * inNode)
	{
		FbxMesh* mesh = inNode->GetMesh();
		if (mesh == nullptr)
		{
			int track = 0;
			track = inNode->GetChildCount(false);
			mesh = inNode->GetChild(0)->GetMesh();
		}
		mTriCount = mesh->GetPolygonCount();
		int vertCount = 0;

		mTris.reserve(mTriCount);
		for (unsigned int x = 0; x < mTriCount; ++x)
		{
			XMFLOAT3 norm[3];
			XMFLOAT3 tan[3];
			XMFLOAT3 binorm[3];
			XMFLOAT2 uv[3][2];

			Triangle tri;
			mTris.push_back(tri);
			for (unsigned int y = 0; y < 3; ++y)
			{
				int ctrlPointIndex = mesh->GetPolygonVertex(x, y);
				CtrlPoint* ctrlPoint = mControlPoints[ctrlPointIndex];

				ReadNormal(mesh, ctrlPointIndex, vertCount, norm[y]);
				for (int z = 0; z < 1; ++z)
				{
					ReadUV(mesh, ctrlPointIndex, mesh->GetTextureUVIndex(x, y), z, uv[y][z]);
				}
				PNMVertex vert;
				vert.mPosition = ctrlPoint->mPosition;
				vert.mNormal = norm[y];
				vert.mUV = uv[y][0];
				mVerts.push_back(vert);
				mTris.back().mIndices.push_back(vertCount);
				++vertCount;
			}
		}

		for (auto itr = mControlPoints.begin(); itr != mControlPoints.end(); ++itr)
		{
			delete itr->second;
		}
		mControlPoints.clear();
	}

	void Export::ReadUV(FbxMesh * inMesh, int inCtrlPointIndex, int inTextureUVIndex, int inUVLayer, XMFLOAT2 & outUV)
	{
		if (inUVLayer >= 2 || inMesh->GetElementUVCount() <= inUVLayer)
		{
			throw exception("Incorrect UV Layer");
		}

		FbxGeometryElementUV* uv = inMesh->GetElementUV(inUVLayer);
		switch(uv->GetMappingMode())
		{
		case FbxGeometryElement::eByControlPoint:
			switch (uv->GetReferenceMode())
			{
			case FbxGeometryElement::eDirect:
			{
				outUV.x = static_cast<float>(uv->GetDirectArray().GetAt(inCtrlPointIndex).mData[0]);
				outUV.y = static_cast<float>(uv->GetDirectArray().GetAt(inCtrlPointIndex).mData[1]);
			}
			break;

			case FbxGeometryElement::eIndexToDirect:
			{
				int index = uv->GetIndexArray().GetAt(inCtrlPointIndex);
				outUV.x = static_cast<float>(uv->GetDirectArray().GetAt(index).mData[0]);
				outUV.y = static_cast<float>(uv->GetDirectArray().GetAt(index).mData[1]);
			}
			break;

			default:
			{
				throw exception("Reference Used is Invalid");
			}
			break;

			}

			case FbxGeometryElement::eByPolygonVertex:
				switch (uv->GetReferenceMode())
				{
				case FbxGeometryElement::eDirect:
				case FbxGeometryElement::eIndexToDirect:
				{
					outUV.x = static_cast<float>(uv->GetDirectArray().GetAt(inTextureUVIndex).mData[0]);
					outUV.y = static_cast<float>(uv->GetDirectArray().GetAt(inTextureUVIndex).mData[1]);
				}
				break;

				default:
					throw std::exception("Reference Used is Invalid");
				}
				break;
		}
	}

	void Export::ReadNormal(FbxMesh * inMesh, int inCtrlPointIndex, int inVertexCounter, XMFLOAT3 & outNormal)
	{
		if (inMesh->GetElementNormalCount() < 1)
		{
			throw exception("Normal Number Invalid");
		}

		FbxGeometryElementNormal* normal = inMesh->GetElementNormal(0);
		switch (normal->GetMappingMode())
		{
		case FbxGeometryElement::eByControlPoint:
			switch (normal->GetReferenceMode())
			{
			case FbxGeometryElement::eDirect:
			{
				outNormal.x = static_cast<float>(normal->GetDirectArray().GetAt(inCtrlPointIndex).mData[0]);
				outNormal.y = static_cast<float>(normal->GetDirectArray().GetAt(inCtrlPointIndex).mData[1]);
				outNormal.z = static_cast<float>(normal->GetDirectArray().GetAt(inCtrlPointIndex).mData[2]);
			}
			break;

			case FbxGeometryElement::eIndexToDirect:
			{
				int index = normal->GetIndexArray().GetAt(inCtrlPointIndex);
				outNormal.x = static_cast<float>(normal->GetDirectArray().GetAt(index).mData[0]);
				outNormal.y = static_cast<float>(normal->GetDirectArray().GetAt(index).mData[1]);
				outNormal.z = static_cast<float>(normal->GetDirectArray().GetAt(index).mData[2]);
			}
			break;

			default:
			{
				throw exception("Reference Used is Invalid");
			}
			break;
			}

		case FbxGeometryElement::eByPolygonVertex:
			switch (normal->GetReferenceMode())
			{
			case FbxGeometryElement::eDirect:
			{
				outNormal.x = static_cast<float>(normal->GetDirectArray().GetAt(inVertexCounter).mData[0]);
				outNormal.y = static_cast<float>(normal->GetDirectArray().GetAt(inVertexCounter).mData[1]);
				outNormal.z = static_cast<float>(normal->GetDirectArray().GetAt(inVertexCounter).mData[2]);
			}
			break;

			case FbxGeometryElement::eIndexToDirect:
			{
				int index = normal->GetIndexArray().GetAt(inVertexCounter);
				outNormal.x = static_cast<float>(normal->GetDirectArray().GetAt(index).mData[0]);
				outNormal.y = static_cast<float>(normal->GetDirectArray().GetAt(index).mData[1]);
				outNormal.z = static_cast<float>(normal->GetDirectArray().GetAt(index).mData[2]);
			}
			break;

			default:
			{
				throw exception("Reference Used is Invalid");
			}
			break;
			}

		}
	}

	void Export::ReadBinormal(FbxMesh * inMesh, int inCtrlPointIndex, int inVertexCounter, XMFLOAT3 & outBinormal)
	{
		if (inMesh->GetElementBinormalCount() < 1)
		{
			throw exception("Binormal Number Invalid");
		}

		FbxGeometryElementBinormal* binormal = inMesh->GetElementBinormal(0);
		switch (binormal->GetMappingMode())
		{
		case FbxGeometryElement::eByControlPoint:
			switch (binormal->GetReferenceMode())
			{
			case FbxGeometryElement::eDirect:
			{
				outBinormal.x = static_cast<float>(binormal->GetDirectArray().GetAt(inCtrlPointIndex).mData[0]);
				outBinormal.y = static_cast<float>(binormal->GetDirectArray().GetAt(inCtrlPointIndex).mData[1]);
				outBinormal.z = static_cast<float>(binormal->GetDirectArray().GetAt(inCtrlPointIndex).mData[2]);
			}
			break;

			case FbxGeometryElement::eIndexToDirect:
			{
				int index = binormal->GetIndexArray().GetAt(inCtrlPointIndex);
				outBinormal.x = static_cast<float>(binormal->GetDirectArray().GetAt(index).mData[0]);
				outBinormal.y = static_cast<float>(binormal->GetDirectArray().GetAt(index).mData[1]);
				outBinormal.z = static_cast<float>(binormal->GetDirectArray().GetAt(index).mData[2]);
			}
			break;

			default:
			{
				throw exception("Reference Used is Invalid");
			}
			break;
			}

		case FbxGeometryElement::eByPolygonVertex:
			switch (binormal->GetReferenceMode())
			{
			case FbxGeometryElement::eDirect:
			{
				outBinormal.x = static_cast<float>(binormal->GetDirectArray().GetAt(inVertexCounter).mData[0]);
				outBinormal.y = static_cast<float>(binormal->GetDirectArray().GetAt(inVertexCounter).mData[1]);
				outBinormal.z = static_cast<float>(binormal->GetDirectArray().GetAt(inVertexCounter).mData[2]);
			}
			break;

			case FbxGeometryElement::eIndexToDirect:
			{
				int index = binormal->GetIndexArray().GetAt(inVertexCounter);
				outBinormal.x = static_cast<float>(binormal->GetDirectArray().GetAt(index).mData[0]);
				outBinormal.y = static_cast<float>(binormal->GetDirectArray().GetAt(index).mData[1]);
				outBinormal.z = static_cast<float>(binormal->GetDirectArray().GetAt(index).mData[2]);
			}
			break;

			default:
			{
				throw exception("Reference Used is Invalid");
			}
			break;
			}

		}
	}

	void Export::ReadTangent(FbxMesh * inMesh, int inCtrlPointIndex, int inVertexCounter, XMFLOAT3 & outTangent)
	{
		if (inMesh->GetElementTangentCount() < 1)
		{
			throw exception("Tangent Number Invalid");
		}

		FbxGeometryElementTangent* tangent = inMesh->GetElementTangent(0);
		switch (tangent->GetMappingMode())
		{
		case FbxGeometryElement::eByControlPoint:
			switch (tangent->GetReferenceMode())
			{
			case FbxGeometryElement::eDirect:
			{
				outTangent.x = static_cast<float>(tangent->GetDirectArray().GetAt(inCtrlPointIndex).mData[0]);
				outTangent.y = static_cast<float>(tangent->GetDirectArray().GetAt(inCtrlPointIndex).mData[1]);
				outTangent.z = static_cast<float>(tangent->GetDirectArray().GetAt(inCtrlPointIndex).mData[2]);
			}
			break;

			case FbxGeometryElement::eIndexToDirect:
			{
				int index = tangent->GetIndexArray().GetAt(inCtrlPointIndex);
				outTangent.x = static_cast<float>(tangent->GetDirectArray().GetAt(index).mData[0]);
				outTangent.y = static_cast<float>(tangent->GetDirectArray().GetAt(index).mData[1]);
				outTangent.z = static_cast<float>(tangent->GetDirectArray().GetAt(index).mData[2]);
			}
			break;

			default:
			{
				throw exception("Reference Used is Invalid");
			}
			break;
			}

		case FbxGeometryElement::eByPolygonVertex:
			switch (tangent->GetReferenceMode())
			{
			case FbxGeometryElement::eDirect:
			{
				outTangent.x = static_cast<float>(tangent->GetDirectArray().GetAt(inVertexCounter).mData[0]);
				outTangent.y = static_cast<float>(tangent->GetDirectArray().GetAt(inVertexCounter).mData[1]);
				outTangent.z = static_cast<float>(tangent->GetDirectArray().GetAt(inVertexCounter).mData[2]);
			}
			break;

			case FbxGeometryElement::eIndexToDirect:
			{
				int index = tangent->GetIndexArray().GetAt(inVertexCounter);
				outTangent.x = static_cast<float>(tangent->GetDirectArray().GetAt(index).mData[0]);
				outTangent.y = static_cast<float>(tangent->GetDirectArray().GetAt(index).mData[1]);
				outTangent.z = static_cast<float>(tangent->GetDirectArray().GetAt(index).mData[2]);
			}
			break;

			default:
			{
				throw exception("Reference Used is Invalid");
			}
			break;
			}

		}
	}

}
