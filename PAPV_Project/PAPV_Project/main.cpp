#include <iostream>
#include <ctime>
#include <Windows.h>
#include <assert.h>
#include <vector>
#include <fbxsdk.h>

#include <d3d11.h>
#pragma comment(lib, "d3d11.lib")

#include <DirectXMath.h>

using namespace std;
using namespace DirectX;

#include "GroundShader_VS.csh"
#include "GroundShader_PS.csh"

#include "DDSTextureLoader.h"

struct FBXVertex
{
	float pos[3];
};

class WIN_APP
{
	HRESULT							hr;
	HINSTANCE						application;
	WNDPROC							appWndProc;
	HWND							window;

	ID3D11Device *device;
	ID3D11DeviceContext *deviceContext;
	ID3D11RenderTargetView *RTV;
	IDXGISwapChain *swapChain;
	D3D11_VIEWPORT viewport;
	

	ID3D11InputLayout *groundInputlayout;

	ID3D11Texture2D* depthStencil = NULL;
	D3D11_TEXTURE2D_DESC depthDesc;

	D3D11_DEPTH_STENCIL_DESC stencilDesc;
	ID3D11DepthStencilState *stencilState;
	ID3D11DepthStencilView* stencilView;
	D3D11_DEPTH_STENCIL_VIEW_DESC stencilViewdesc;

	XMMATRIX Camera;

	struct World
	{
		XMMATRIX WorldMatrix;
	};

	struct View
	{
		XMMATRIX ViewMatrix;
		XMMATRIX ProjectionMatrix;
	};

#pragma region FBX
	//struct FBXVertex
	//{
	//	float pos[3];
	//};

	FbxManager* m_fbxManager = nullptr;
#pragma endregion

	
	struct Light
	{
		XMFLOAT3 direction;
		float pointRadius;
		XMFLOAT4 color;

		XMFLOAT4 pointPosition;
		XMFLOAT4 pointColor;

		XMFLOAT4 spotColor;
		XMFLOAT4 spotPosition;
		XMFLOAT4 spotDirection;
		float spotRadius;

		XMFLOAT3 padding;

		XMFLOAT4 camPosition;
	};
	View view;
	World groundWorld;
	Light light;

public:

	struct SIMPLE_VERTEX
	{
		XMVECTOR pos;
		XMVECTOR color;
		XMFLOAT2 uv;
		XMFLOAT3 normal;
	};

	SIMPLE_VERTEX ground[4];

	POINT point;

	ID3D11Buffer * viewConstant;
	D3D11_BUFFER_DESC viewConstdesc;

	ID3D11ShaderResourceView *groundshaderView;

	ID3D11SamplerState *groundSample;
	D3D11_SAMPLER_DESC groundsampleDesc;

	ID3D11VertexShader *skyboxVertShader;
	ID3D11PixelShader *skyboxPixShader;
	ID3D11InputLayout *skyboxInputLayout;
	ID3D11Buffer *skyboxBuffer;
	D3D11_BUFFER_DESC skyboxDesc;

	ID3D11Buffer *groundBuffer;
	D3D11_BUFFER_DESC groundBufferdesc;

	ID3D11Buffer *groundConstant;
	D3D11_BUFFER_DESC groundConstantdesc;

	ID3D11Buffer *groundIndex;
	ID3D11VertexShader *groundVertshader;
	ID3D11PixelShader *groundPixshader;
	D3D11_BUFFER_DESC groundIndexdesc;

	ID3D11Buffer *lightBuffer;
	D3D11_BUFFER_DESC lightBufferdesc;

	WIN_APP(HINSTANCE hinst, WNDPROC proc);
	bool Run();
	bool ShutDown();
};

HRESULT LoadFBX(FbxManager* fbxman, const char* fileName, vector<FBXVertex>* outVertexVector)
{
	if (fbxman == nullptr)
	{
		fbxman = FbxManager::Create();

		FbxIOSettings* m_IOSettings = FbxIOSettings::Create(fbxman, IOSROOT);
		fbxman->SetIOSettings(m_IOSettings);
	}

	FbxImporter* m_import = FbxImporter::Create(fbxman, "");
	FbxScene* m_scene = FbxScene::Create(fbxman, "");

	//Add an FBX to the project
	bool success = m_import->Initialize(fileName, -1, fbxman->GetIOSettings());
	if (!success)
	{
		return E_FAIL;
	}

	success = m_import->Import(m_scene);
	if (!success)
	{
		m_import->Destroy();
	}

	FbxNode* m_rootNode = m_scene->GetRootNode();
	if (m_rootNode)
	{
		for (unsigned int i = 0; i < m_rootNode->GetChildCount(); i++)
		{
			FbxNode* m_childNode = m_rootNode->GetChild(i);
			if (m_childNode->GetNodeAttribute() == NULL)
			{
				continue;
			}

			FbxNodeAttribute::EType attributeType = m_childNode->GetNodeAttribute()->GetAttributeType();
			if (attributeType != FbxNodeAttribute::eMesh)
			{
				continue;
			}

			FbxMesh* m_mesh = (FbxMesh*)m_childNode->GetNodeAttribute();

			FbxVector4* m_verts = m_mesh->GetControlPoints();

			for (int y = 0; y < m_mesh->GetPolygonCount(); y++)
			{
				int numVerts = m_mesh->GetPolygonSize(y);
				assert(numVerts == 3);

				for (int z = 0; z < numVerts; z++)
				{
					int controlPointIndex = m_mesh->GetPolygonVertex(y, z);

					FBXVertex vert;
					vert.pos[0] = (float)m_verts[controlPointIndex].mData[0];
					vert.pos[1] = (float)m_verts[controlPointIndex].mData[1];
					vert.pos[2] = (float)m_verts[controlPointIndex].mData[2];
					outVertexVector->push_back(vert);
				}
			}
		}
	}
	return S_OK;
}

WIN_APP::WIN_APP(HINSTANCE hinst, WNDPROC proc)
{
#pragma region Windows Init
	application = hinst;
	appWndProc = proc;

	WNDCLASSEX  wndClass;
	ZeroMemory(&wndClass, sizeof(wndClass));
	wndClass.cbSize = sizeof(WNDCLASSEX);
	wndClass.lpfnWndProc = appWndProc;
	wndClass.lpszClassName = L"RTA Project";
	wndClass.hInstance = application;
	wndClass.hCursor = LoadCursor(NULL, IDC_ARROW);
	wndClass.hbrBackground = (HBRUSH)(COLOR_WINDOWFRAME);
	RegisterClassEx(&wndClass);

	RECT window_size = { 0, 0, 1280, 720 };
	AdjustWindowRect(&window_size, WS_OVERLAPPEDWINDOW, false);

	window = CreateWindow(L"RTA Project", L"RTA Project", WS_OVERLAPPEDWINDOW & ~(WS_THICKFRAME | WS_MAXIMIZEBOX),
		CW_USEDEFAULT, CW_USEDEFAULT, window_size.right - window_size.left, window_size.bottom - window_size.top,
		NULL, NULL, application, this);

	ShowWindow(window, SW_SHOW);
#pragma endregion

#pragma region Device and SwapChain Init
	DXGI_SWAP_CHAIN_DESC desc;
	ZeroMemory(&desc, sizeof(DXGI_SWAP_CHAIN_DESC));
	desc.BufferCount = 1;
	desc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	desc.BufferDesc.Width = 1280;
	desc.BufferDesc.Height = 720;
	desc.Flags = D3D11_CREATE_DEVICE_DEBUG;
	desc.OutputWindow = window;
	desc.SampleDesc.Count = 1;
	desc.Windowed = TRUE;
	D3D_FEATURE_LEVEL feature[] = { D3D_FEATURE_LEVEL_11_0 };
	D3D11CreateDeviceAndSwapChain(
		NULL,
		D3D_DRIVER_TYPE_HARDWARE,
		NULL,
		D3D11_CREATE_DEVICE_DEBUG,
		feature,
		1,
		D3D11_SDK_VERSION,
		&desc,
		&swapChain,
		&device,
		NULL,
		&deviceContext);
#pragma endregion

	ID3D11Resource *p_RT;
	swapChain->GetBuffer(0, __uuidof(p_RT), reinterpret_cast<void**>(&p_RT));

	device->CreateRenderTargetView(p_RT, NULL, &RTV);
	p_RT->Release();

	viewport.TopLeftX = 0;
	viewport.TopLeftY = 0;
	viewport.Width = 1280;
	viewport.Height = 720;
	viewport.MinDepth = 0;
	viewport.MaxDepth = 1;

#pragma region Ground and Light Init
	ground[0].pos.m128_f32[0] = -15;
	ground[0].pos.m128_f32[1] = -5;
	ground[0].pos.m128_f32[2] = -15;
	ground[0].pos.m128_f32[3] = 1;
	ground[0].uv.x = 0;
	ground[0].uv.y = 1;
	ground[0].normal.x = 0;
	ground[0].normal.y = 1;
	ground[0].normal.z = 0;

	ground[1].pos.m128_f32[0] = -15;
	ground[1].pos.m128_f32[1] = -5;
	ground[1].pos.m128_f32[2] = 15;
	ground[1].pos.m128_f32[3] = 1;
	ground[1].uv.x = 0;
	ground[1].uv.y = 0;
	ground[1].normal.x = 0;
	ground[1].normal.y = 1;
	ground[1].normal.z = 0;

	ground[2].pos.m128_f32[0] = 15;
	ground[2].pos.m128_f32[1] = -5;
	ground[2].pos.m128_f32[2] = -15;
	ground[2].pos.m128_f32[3] = 1;
	ground[2].uv.x = 1;
	ground[2].uv.y = 1;
	ground[2].normal.x = 0;
	ground[2].normal.y = 1;
	ground[2].normal.z = 0;

	ground[3].pos.m128_f32[0] = 15;
	ground[3].pos.m128_f32[1] = -5;
	ground[3].pos.m128_f32[2] = 15;
	ground[3].pos.m128_f32[3] = 1;
	ground[3].uv.x = 1;
	ground[3].uv.y = 0;
	ground[3].normal.x = 0;
	ground[3].normal.y = 1;
	ground[3].normal.z = 0;

	UINT groundIndexes[12]
	{
		0, 1, 2,
		3, 2, 1,
		2, 1, 0,
		1, 2, 3
	};

	light.color = { 1,1,1,1 };
	light.direction = { -1.0f, -0.75f, 0.0f };

	light.pointPosition = { 0, -3, 0, 1 };
	light.pointColor = { 0.0f, 1.0f, 1.0f, 1.0f };
	light.pointRadius = 10;

	light.spotColor = { 1.0f, 1.0f, 0.0f, 1.0f };
	light.spotDirection = { 0, 0, 1, 1 };
	light.spotPosition = { -5, 0, -10, 1 };
	light.spotRadius = 0.93f;

	CreateDDSTextureFromFile(device, L"moon.dds", nullptr, &groundshaderView, 0);

	groundBufferdesc.Usage = D3D11_USAGE_IMMUTABLE;
	groundBufferdesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	groundBufferdesc.CPUAccessFlags = NULL;
	groundBufferdesc.ByteWidth = sizeof(SIMPLE_VERTEX) * 4;
	groundBufferdesc.MiscFlags = 0;
	groundBufferdesc.StructureByteStride = 0;

	lightBufferdesc.Usage = D3D11_USAGE_DYNAMIC;
	lightBufferdesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	lightBufferdesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	lightBufferdesc.ByteWidth = sizeof(Light);
	lightBufferdesc.MiscFlags = 0;
	lightBufferdesc.StructureByteStride = 0;

	viewConstdesc.Usage = D3D11_USAGE_DYNAMIC;
	viewConstdesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	viewConstdesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	viewConstdesc.ByteWidth = sizeof(View);
	viewConstdesc.MiscFlags = 0;
	viewConstdesc.StructureByteStride = 0;

	groundConstantdesc.Usage = D3D11_USAGE_DYNAMIC;
	groundConstantdesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	groundConstantdesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	groundConstantdesc.ByteWidth = sizeof(World);
	groundConstantdesc.MiscFlags = 0;
	groundConstantdesc.StructureByteStride = 0;

	groundIndexdesc.Usage = D3D11_USAGE_IMMUTABLE;
	groundIndexdesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	groundIndexdesc.CPUAccessFlags = NULL;
	groundIndexdesc.ByteWidth = sizeof(groundIndexes);
	groundIndexdesc.MiscFlags = 0;
	groundIndexdesc.StructureByteStride = 0;

	depthDesc.Width = 1280;
	depthDesc.Height = 720;
	depthDesc.MipLevels = 1;
	depthDesc.ArraySize = 1;
	depthDesc.Format = DXGI_FORMAT_D32_FLOAT;
	depthDesc.SampleDesc.Count = 1;
	depthDesc.SampleDesc.Quality = 0;
	depthDesc.Usage = D3D11_USAGE_DEFAULT;
	depthDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
	depthDesc.CPUAccessFlags = 0;
	depthDesc.MiscFlags = 0;
	device->CreateTexture2D(&depthDesc, NULL, &depthStencil);

	stencilDesc.DepthEnable = true;
	stencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
	stencilDesc.DepthFunc = D3D11_COMPARISON_LESS;

	stencilDesc.StencilEnable = true;
	stencilDesc.StencilReadMask = 0xFF;
	stencilDesc.StencilWriteMask = 0xFF;

	stencilDesc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	stencilDesc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_INCR;
	stencilDesc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
	stencilDesc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;

	stencilDesc.BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	stencilDesc.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_DECR;
	stencilDesc.BackFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
	stencilDesc.BackFace.StencilFunc = D3D11_COMPARISON_ALWAYS;

	device->CreateDepthStencilState(&stencilDesc, &stencilState);

	stencilViewdesc.Format = DXGI_FORMAT_D32_FLOAT;
	stencilViewdesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	stencilViewdesc.Texture2D.MipSlice = 0;

	groundsampleDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	groundsampleDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
	groundsampleDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
	groundsampleDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
	groundsampleDesc.MinLOD = (-FLT_MAX);
	groundsampleDesc.MaxLOD = (FLT_MAX);
	groundsampleDesc.MipLODBias = 0.0f;
	groundsampleDesc.MaxAnisotropy = 1;
	groundsampleDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
	groundsampleDesc.BorderColor[0] = 1;
	groundsampleDesc.BorderColor[1] = 1;
	groundsampleDesc.BorderColor[2] = 1;
	groundsampleDesc.BorderColor[3] = 1;
#pragma endregion

	device->CreateDepthStencilView(depthStencil, &stencilViewdesc, &stencilView);

	view.ViewMatrix = XMMatrixInverse(0, XMMatrixTranslation(0, 5, -10));
	groundWorld.WorldMatrix = XMMatrixIdentity();

	XMMATRIX projection;

	projection = XMMatrixPerspectiveFovLH(XMConvertToRadians(65.0f), (1280.0f/720.0f), 0.1f, 100.0f);

	view.ProjectionMatrix = projection;


#pragma region Buffer Creation
	D3D11_SUBRESOURCE_DATA InitData;
	InitData.pSysMem = ground;
	InitData.SysMemPitch = 0;
	InitData.SysMemSlicePitch = 0;

	device->CreateBuffer(&groundBufferdesc, &InitData, &groundBuffer);

	InitData.pSysMem = &view;
	device->CreateBuffer(&viewConstdesc, &InitData, &viewConstant);

	InitData.pSysMem = &groundWorld;
	device->CreateBuffer(&groundConstantdesc, &InitData, &groundConstant);

	InitData.pSysMem = groundIndexes;
	device->CreateBuffer(&groundIndexdesc, &InitData, &groundIndex);

	InitData.pSysMem = &light;
	device->CreateBuffer(&lightBufferdesc, &InitData, &lightBuffer);
#pragma endregion

	device->CreateVertexShader(GroundShader_VS, sizeof(GroundShader_VS), nullptr, &groundVertshader);
	device->CreatePixelShader(GroundShader_PS, sizeof(GroundShader_PS), nullptr, &groundPixshader);

	D3D11_INPUT_ELEMENT_DESC groundLayout[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "UV", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 }

	};

	device->CreateSamplerState(&groundsampleDesc, &groundSample);


	device->CreateInputLayout(groundLayout, ARRAYSIZE(groundLayout), GroundShader_VS, sizeof(GroundShader_VS), &groundInputlayout);
	vector<FBXVertex>* test = new vector<FBXVertex>;

	LoadFBX(m_fbxManager, "bone.fbx", test);

}
bool WIN_APP::Run()
{
	view.ViewMatrix = XMMatrixInverse(0, view.ViewMatrix);

	//light.camPosition = { toShader.ViewMatrix.r[3].m128_f32[0], toShader.ViewMatrix.r[3].m128_f32[1], toShader.ViewMatrix.r[3].m128_f32[2], toShader.ViewMatrix.r[3].m128_f32[3] };

#pragma region Camera and Light Controls
	if (GetAsyncKeyState(VK_SPACE))
	{
		XMMATRIX up = XMMatrixTranslation(0, 0.1f * 2, 0);
		view.ViewMatrix = XMMatrixMultiply(up, view.ViewMatrix);
	}
	if (GetAsyncKeyState(VK_LCONTROL))
	{
		XMMATRIX down = XMMatrixTranslation(0, -0.1f * 2, 0);
		view.ViewMatrix = XMMatrixMultiply(down, view.ViewMatrix);
	}
	if (GetAsyncKeyState('A'))
	{
		XMMATRIX left = XMMatrixTranslation(-0.1f * 2, 0, 0);
		view.ViewMatrix = XMMatrixMultiply(left, view.ViewMatrix);
	}
	if (GetAsyncKeyState('D'))
	{
		XMMATRIX right = XMMatrixTranslation(0.1f * 2, 0, 0);
		view.ViewMatrix = XMMatrixMultiply(right, view.ViewMatrix);
	}

	if (GetAsyncKeyState('W'))
	{
		XMMATRIX forward = XMMatrixTranslation(0, 0, 0.1f * 2);
		view.ViewMatrix = XMMatrixMultiply(forward, view.ViewMatrix);
	}

	if (GetAsyncKeyState('S'))
	{
		XMMATRIX backward = XMMatrixTranslation(0, 0, -0.1f * 2);
		view.ViewMatrix = XMMatrixMultiply(backward, view.ViewMatrix);
	}

	if (GetAsyncKeyState(VK_UP))
	{
		light.pointPosition = { light.pointPosition.x, light.pointPosition.y, light.pointPosition.z + (float)(0.1f * 2), light.pointPosition.w };
	}
	if (GetAsyncKeyState(VK_DOWN))
	{
		light.pointPosition = { light.pointPosition.x, light.pointPosition.y, light.pointPosition.z - (float)(0.1f * 2), light.pointPosition.w };
	}
	if (GetAsyncKeyState(VK_LEFT))
	{
		light.pointPosition = { light.pointPosition.x - (float)(0.1f * 2), light.pointPosition.y, light.pointPosition.z, light.pointPosition.w };
	}
	if (GetAsyncKeyState(VK_RIGHT))
	{
		light.pointPosition = { light.pointPosition.x + (float)(0.1f * 2), light.pointPosition.y, light.pointPosition.z, light.pointPosition.w };
	}
	if (GetAsyncKeyState('I'))
	{
		light.pointPosition = { light.pointPosition.x, light.pointPosition.y + (float)(0.1f * 2), light.pointPosition.z, light.pointPosition.w };
	}
	if (GetAsyncKeyState('K'))
	{
		light.pointPosition = { light.pointPosition.x, light.pointPosition.y - (float)(0.1f * 2), light.pointPosition.z, light.pointPosition.w };
	}

	if (GetAsyncKeyState(VK_NUMPAD8))
	{
		light.direction = { light.direction.x, light.direction.y + (float)(0.1f), light.direction.z };
	}
	if (GetAsyncKeyState(VK_NUMPAD2))
	{
		light.direction = { light.direction.x, light.direction.y - (float)(0.1f), light.direction.z };
	}
	if (GetAsyncKeyState(VK_NUMPAD4))
	{
		light.direction = { light.direction.x - (float)(0.1f), light.direction.y, light.direction.z };
	}
	if (GetAsyncKeyState(VK_NUMPAD6))
	{
		light.direction = { light.direction.x + (float)(0.1f), light.direction.y, light.direction.z };
	}

	POINT newPos;
	GetCursorPos(&newPos);
	if (GetAsyncKeyState(VK_LBUTTON))
	{
		XMMATRIX Store = view.ViewMatrix;

		view.ViewMatrix.r[3].m128_f32[0] = 0;
		view.ViewMatrix.r[3].m128_f32[1] = 0;
		view.ViewMatrix.r[3].m128_f32[2] = 0;


		XMMATRIX XROT = -XMMatrixRotationX((newPos.y - point.y)  * 0.01f);
		XMMATRIX YROT = -XMMatrixRotationY((newPos.x - point.x)  * 0.01f);



		view.ViewMatrix = XMMatrixMultiply(view.ViewMatrix, YROT);
		view.ViewMatrix = XMMatrixMultiply(XROT, view.ViewMatrix);


		view.ViewMatrix.r[3].m128_f32[0] = Store.r[3].m128_f32[0];
		view.ViewMatrix.r[3].m128_f32[1] = Store.r[3].m128_f32[1];
		view.ViewMatrix.r[3].m128_f32[2] = Store.r[3].m128_f32[2];


	}

	view.ViewMatrix = XMMatrixInverse(0, view.ViewMatrix);

	point = newPos;
#pragma endregion

	//deviceContext->ClearDepthStencilView(stencilView, D3D11_CLEAR_DEPTH, 1, 0);


	D3D11_MAPPED_SUBRESOURCE viewMap;
	D3D11_MAPPED_SUBRESOURCE groundMap;
	D3D11_MAPPED_SUBRESOURCE lightMap;
	UINT stride = sizeof(SIMPLE_VERTEX);
	UINT offset = 0;

	deviceContext->Map(viewConstant, 0, D3D11_MAP_WRITE_DISCARD, NULL, &viewMap);
	memcpy_s(viewMap.pData, sizeof(View), &view, sizeof(View));
	deviceContext->Unmap(viewConstant, 0);

	deviceContext->Map(groundConstant, 0, D3D11_MAP_WRITE_DISCARD, NULL, &groundMap);
	memcpy_s(groundMap.pData, sizeof(World), &groundWorld, sizeof(World));
	deviceContext->Unmap(groundConstant, 0);

	deviceContext->Map(lightBuffer, 0, D3D11_MAP_WRITE_DISCARD, NULL, &lightMap);
	memcpy_s(lightMap.pData, sizeof(Light), &light, sizeof(Light));
	deviceContext->Unmap(lightBuffer, 0);

	deviceContext->OMSetRenderTargets(1, &RTV, stencilView);
	deviceContext->OMSetDepthStencilState(stencilState, 1);
	
	deviceContext->RSSetViewports(1, &viewport);
	
	float rgba[4] = { 0.0f, 0.0f, 1.0f, 0.0f };
	deviceContext->ClearRenderTargetView(RTV, rgba);

	deviceContext->PSSetConstantBuffers(0, 1, &lightBuffer);

	deviceContext->VSSetConstantBuffers(0, 1, &groundConstant);
	deviceContext->VSSetConstantBuffers(1, 1, &viewConstant);
	deviceContext->IASetVertexBuffers(0, 1, &groundBuffer, &stride, &offset);
	deviceContext->IASetIndexBuffer(groundIndex, DXGI_FORMAT_R32_UINT, 0);
	
	deviceContext->IASetInputLayout(groundInputlayout);
	deviceContext->VSSetShader(groundVertshader, NULL, 0);
	deviceContext->PSSetShader(groundPixshader, NULL, 0);
	deviceContext->PSSetShaderResources(0, 1, &groundshaderView);
	deviceContext->PSSetSamplers(0, 1, &groundSample);

	deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	deviceContext->DrawIndexed(12, 0, 0);

	swapChain->Present(0, 0);

	return true;
}
bool WIN_APP::ShutDown()
{
	UnregisterClass(L"RTA Project", application);
	return true;
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nCmdShow);
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wparam, LPARAM lparam);
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, LPTSTR, int)
{
	srand(unsigned int(time(0)));
	WIN_APP myApp(hInstance, (WNDPROC)WndProc);
	MSG msg; ZeroMemory(&msg, sizeof(msg));
	while (msg.message != WM_QUIT && myApp.Run())
	{
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}
	myApp.ShutDown();
	return 0;
}
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	if (GetAsyncKeyState(VK_ESCAPE))
		message = WM_DESTROY;
	switch (message)
	{
	case (WM_DESTROY) : { PostQuitMessage(0); }
						break;
	}
	return DefWindowProc(hWnd, message, wParam, lParam);
}