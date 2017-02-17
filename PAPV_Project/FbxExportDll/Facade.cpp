#include "Facade.h"
#include "Export.h"


#include <ios>
#include <iostream>
#include <fstream>
using namespace DllExport;

namespace FBXE
{
	std::vector<FBXData> Facade::LoadFBX(std::vector<FBXData> outVerts, const char* file)
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
	void Facade::FbxToBinary(const char* inFile, const char* outputFileName)
	{
		//Fbx inital load
		Export* exporter = new Export();
		exporter->Initialize();
		exporter->LoadScene(inFile);
		FbxScene* scene = exporter->getScene();
		FbxNode* node = scene->GetRootNode();
		int track = scene->GetNodeCount();
		exporter->ProcessControlPoints(node);
		exporter->ProcessMesh(scene->GetRootNode());

		//Turning FBX into vector
		vector<FBXData> temp;
		temp = LoadFBX(temp, inFile);
		int len = temp.size();

		//Writing out to binary file
		ofstream file(outputFileName, ios::in | ios::binary);
		file.open(outputFileName, ios::binary);
		while (file.is_open())
		{
			for (unsigned int i = 0; i < len; i++)
			{
				string combine;
				char a = temp[i].verts.x;
				combine.push_back(a);
				combine.push_back(',');

				char b = temp[i].verts.y;
				combine.push_back(b);
				combine.push_back(',');

				char c = temp[i].verts.z;
				combine.push_back(c);
				combine.push_back(',');

				char d = temp[i].norms.x;
				combine.push_back(d);
				combine.push_back(',');

				char e = temp[i].norms.y;
				combine.push_back(e);
				combine.push_back(',');

				char f = temp[i].norms.z;
				combine.push_back(f);
				combine.push_back(',');

				char g = temp[i].uvs.u;
				combine.push_back(g);
				combine.push_back(',');

				char h = temp[i].uvs.v;
				combine.push_back(h);
				combine.push_back('/');

				file.write(combine.c_str(), combine.size());
			}
			file.close();
		}
	}
	std::vector<FBXData> Facade::BinaryToVerts(std::vector<FBXData> outVerts, const char * file)
	{
		vector<FBXData> temp;
		int count = 0;

		ifstream infile(file, ios::out | ios::binary);
		infile.open(file, ios::binary);

		infile.seekg(0, infile.end);
		int len = infile.tellg();
		infile.seekg(0, infile.beg);
		char* buff = new char[len];

		infile.read(buff, len);
		while (infile.is_open())
		{
			FBXData test;
			for (unsigned int i = 0; i < len; i++)
			{

				if (count == 0 && (buff[i] == ',' || buff[i] == '/'))
				{
					test.verts.x = (float)buff[i];
				}
				if (count == 1 && (buff[i] == ',' || buff[i] == '/'))
				{
					test.verts.y = (float)buff[i];
				}
				if (count == 2 && (buff[i] == ',' || buff[i] == '/'))
				{
					test.verts.z = (float)buff[i];
				}
				if (count == 3 && (buff[i] == ',' || buff[i] == '/'))
				{
					test.norms.x = (float)buff[i];
				}
				if (count == 4 && (buff[i] == ',' || buff[i] == '/'))
				{
					test.norms.y = (float)buff[i];
				}
				if (count == 5 && (buff[i] == ',' || buff[i] == '/'))
				{
					test.norms.z = (float)buff[i];
				}
				if (count == 6 && (buff[i] == ',' || buff[i] == '/'))
				{
					test.uvs.u = (float)buff[i];
				}
				if (count == 7 && (buff[i] == ',' || buff[i] == '/'))
				{
					test.uvs.v = (float)buff[i];
					temp.push_back(test);
				}
				count++;
				if (count > 7)
				{
					count = 0;
				}
			}
			infile.close();
		}
		return temp;
	}
}
