#include "Facade.h"
#include "Export.h"
using namespace DllExport;

namespace FBXE
{
	std::vector<FBXData> Facade::LoadFBX(std::vector<FBXData> outVerts, const char * file)
	{
		Export* exporter = new Export();
		exporter->Initialize();
		exporter->LoadScene(file);
		FbxScene* scene = exporter->getScene();
		FbxNode* node = scene->GetRootNode();
		int track = scene->GetNodeCount();
		exporter->ProcessControlPoints(node);
		exporter->ProcessMesh(scene->GetRootNode());

		for (unsigned int i = 0; i < exporter->getVertices().size(); i++)
		{
			FBXData fbx;
			vector<PNMVertex> tempFbx = exporter->getVertices();

			fbx.verts.x = tempFbx[i].mPosition.x;
			fbx.verts.y = tempFbx[i].mPosition.y;
			fbx.verts.z = tempFbx[i].mPosition.z;
			fbx.norms.x = tempFbx[i].mNormal.x;
			fbx.norms.y = tempFbx[i].mNormal.y;
			fbx.norms.z = tempFbx[i].mNormal.z;
			fbx.uvs.u = tempFbx[i].mUV.x;
			fbx.uvs.v = tempFbx[i].mUV.y;
			outVerts.push_back(fbx);
		}
		return outVerts;
	}
}
