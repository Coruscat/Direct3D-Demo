#include "Game.h"
#include "Graphics.h"
#include "Vertex.h"
#include "Input.h"
#include "PathHelpers.h"
#include "Window.h"
#include "ImGui/imgui.h"
#include "ImGui/imgui_impl_dx11.h"
#include "ImGui/imgui_impl_win32.h"
#include <DirectXMath.h>
#include "Mesh.h"
#include <memory>
#include <vector>
#include "Transform.h"
#include "Entity.h"
#include "Camera.h"
#include "Materials.h"
#include "Lights.h"
#include "Sky.h"
#include "WICTextureLoader.h"

// Needed for a helper function to load pre-compiled shader files
#pragma comment(lib, "d3dcompiler.lib")
#include <d3dcompiler.h>

// For the DirectX Math library
using namespace DirectX;

//Entities
std::vector<Entity> entities;

//Materials
std::vector<std::shared_ptr<Materials>> mats;

// Camera
std::vector<std::shared_ptr<Camera>> cams;
std::shared_ptr<Camera> currentCam;

//Shader Offset and Tint
static float tint[4] = { 1.0f,1.0f,1.0f,1.0f };
// Bg Color 
static float color[4];
static XMFLOAT3 ambientColor = XMFLOAT3(0.03f, 0.03f, 0.03f);
bool blur = false;




//Lights
std::vector<Light> lights;
Light directionalLight1;
Light directionalLight2;
Light directionalLight3;

Light pointLight1;
Light pointLight2;

std::shared_ptr<Sky> skyBox;

// Shadow Map Size
UINT shadowMapResolution = 2048;

// --------------------------------------------------------
// Called once per program, after the window and graphics API
// are initialized but before the game loop begins
// --------------------------------------------------------
void Game::Initialize()
{
	// Initialize ImGui & platform/renderer backends
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGui_ImplWin32_Init(Window::Handle());
	ImGui_ImplDX11_Init(Graphics::Device.Get(), Graphics::Context.Get());

	// Style
	ImGui::StyleColorsDark();


	// Lights
	directionalLight1 = {};
	directionalLight1.Type = LIGHT_TYPE_DIRECTIONAL;
	directionalLight1.Direction = XMFLOAT3(0.0f, -1.0f, 0.0f);
	directionalLight1.Color = XMFLOAT3(1.0f, 0.0f, 0.0f);
	directionalLight1.Intensity = 1.0f;
	lights.push_back(directionalLight1);

	directionalLight2 = {};
	directionalLight2.Type = LIGHT_TYPE_DIRECTIONAL;
	directionalLight2.Direction = XMFLOAT3(0.0f, 0.0f, -1.0f);
	directionalLight2.Color = XMFLOAT3(0.0f, 0.0f, 1.0f);
	directionalLight2.Intensity = 1.0f;
	lights.push_back(directionalLight2);

	directionalLight3 = {};
	directionalLight3.Type = LIGHT_TYPE_DIRECTIONAL;
	directionalLight3.Direction = XMFLOAT3(0.0f, 1.0f, 0.0f);
	directionalLight3.Color = XMFLOAT3(0.0f, .4f, 0.0f);
	directionalLight3.Intensity = 1.0f;
	lights.push_back(directionalLight3);

	pointLight1 = {};
	pointLight1.Type = LIGHT_TYPE_POINT;
	pointLight1.Range = 5.0f;
	pointLight1.Position = XMFLOAT3(-4.0f, 0.0f, 2.0f);
	pointLight1.Direction = XMFLOAT3(0.0f, 0.0f, 1.0f);
	pointLight1.Color = XMFLOAT3(1.0f, 1.0f, 1.0f);
	pointLight1.Intensity = 1.0f;
	lights.push_back(pointLight1);

	pointLight2 = {};
	pointLight2.Type = LIGHT_TYPE_POINT;
	pointLight2.Range = 5.0f;
	pointLight2.Position = XMFLOAT3(-4.0f, -4.0f, 2.0f);
	pointLight2.Direction = XMFLOAT3(0.0f, 0.0f, 1.0f);
	pointLight2.Color = XMFLOAT3(1.0f, 1.0f, 1.0f);
	pointLight2.Intensity = 1.0f;
	lights.push_back(pointLight2);

	// Camera
	std::shared_ptr<Camera> camera1 = std::make_shared<Camera>((float)Window::Width() / Window::Height(), XMFLOAT3(0.0f, 0.0f, -10.0f));
	std::shared_ptr<Camera> camera2 = std::make_shared<Camera>((float)Window::Width() / Window::Height(), XMFLOAT3(0.0f, 10.0f, -10.0f));
	std::shared_ptr<Camera> camera3 = std::make_shared<Camera>((float)Window::Width() / Window::Height(), XMFLOAT3(-10.0f, 0.0f, -5.0f));
	camera2->SetFOV(XM_PIDIV2);
	camera3->SetFOV(XM_PIDIV4);
	cams.push_back(camera1);
	cams.push_back(camera2);
	cams.push_back(camera3);
	currentCam = camera1;

	// Helper methods for loading shaders, creating some basic
	// geometry to draw and some simple camera matrices.
	//  - You'll be expanding and/or replacing these later
	LoadShaders();

	// Sample State
	D3D11_SAMPLER_DESC samplerDesc = {};
	samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.MaxAnisotropy = 16;
	samplerDesc.Filter = D3D11_FILTER_ANISOTROPIC;
	samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;

	Graphics::Device->CreateSamplerState(&samplerDesc, &sampleS);

	// Post Process Sampler
	D3D11_SAMPLER_DESC postSampDesc = {};
	postSampDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
	postSampDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
	postSampDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
	postSampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
	postSampDesc.MaxLOD = D3D11_FLOAT32_MAX;

	Graphics::Device->CreateSamplerState(&postSampDesc, postSampler.GetAddressOf());

	// Post Process Resources
	PostSetup();
	
	shadowDSV.Reset();
	shadowSRV.Reset();
	shadowSampler.Reset();
	shadowRasterizer.Reset();

	// Shadow Map Desc
	D3D11_TEXTURE2D_DESC shadowDesc = {};
	shadowDesc.Width = shadowMapResolution;
	shadowDesc.Height = shadowMapResolution;
	shadowDesc.ArraySize = 1;
	shadowDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
	shadowDesc.CPUAccessFlags = 0;
	shadowDesc.Format = DXGI_FORMAT_R32_TYPELESS;
	shadowDesc.MipLevels = 1;
	shadowDesc.MiscFlags = 0;
	shadowDesc.SampleDesc.Count = 1;
	shadowDesc.SampleDesc.Quality = 0;
	shadowDesc.Usage = D3D11_USAGE_DEFAULT;
	Microsoft::WRL::ComPtr<ID3D11Texture2D> shadowTexture;
	Graphics::Device->CreateTexture2D(&shadowDesc, 0, shadowTexture.GetAddressOf());

	// Shadow Stencil and View
	D3D11_DEPTH_STENCIL_VIEW_DESC shadowDSDesc = {};
	shadowDSDesc.Format = DXGI_FORMAT_D32_FLOAT;
	shadowDSDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	shadowDSDesc.Texture2D.MipSlice = 0;
	Graphics::Device->CreateDepthStencilView(
		shadowTexture.Get(),
		&shadowDSDesc,
		shadowDSV.GetAddressOf()
	);

	// Create SRV for shadow map
	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format = DXGI_FORMAT_R32_FLOAT;
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = 1;
	srvDesc.Texture2D.MostDetailedMip = 0;
	Graphics::Device->CreateShaderResourceView(
		shadowTexture.Get(),
		&srvDesc,
		shadowSRV.GetAddressOf()
	);

	// Shadow Sampler
	D3D11_SAMPLER_DESC shadowSampDesc = {};
	shadowSampDesc.Filter = D3D11_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR;
	shadowSampDesc.ComparisonFunc = D3D11_COMPARISON_LESS;
	shadowSampDesc.AddressU = D3D11_TEXTURE_ADDRESS_BORDER;
	shadowSampDesc.AddressV = D3D11_TEXTURE_ADDRESS_BORDER;
	shadowSampDesc.AddressW = D3D11_TEXTURE_ADDRESS_BORDER;
	shadowSampDesc.BorderColor[0] = 1.0f;
	shadowSampDesc.BorderColor[1] = 1.0f;
	shadowSampDesc.BorderColor[2] = 1.0f;
	shadowSampDesc.BorderColor[3] = 1.0f;
	Graphics::Device->CreateSamplerState(&shadowSampDesc, &shadowSampler);

	// Shadow Rasterizer
	D3D11_RASTERIZER_DESC shadowRastDesc = {};
	shadowRastDesc.FillMode = D3D11_FILL_SOLID;
	shadowRastDesc.CullMode = D3D11_CULL_BACK;
	shadowRastDesc.DepthClipEnable = true;
	shadowRastDesc.DepthBias = 1000; // Multiplied by (smallest possible positive value storable in the depth buffer)
	shadowRastDesc.SlopeScaledDepthBias = 1.0f;
	Graphics::Device->CreateRasterizerState(&shadowRastDesc, &shadowRasterizer);


	// Matrices for shadow rendering
	XMMATRIX lightView = XMMatrixLookAtLH(
		XMVectorSet(0, 20, 1, 0),
		XMVectorSet(0, 0, 0, 0),
		XMVectorSet(0, 1, 0, 0)
	);
	XMStoreFloat4x4(&lightViewMatrix, lightView);

	float lightProjectionSize = 50.0f; // Fix if shadows cut off
	XMMATRIX lightProjection = XMMatrixOrthographicLH(
		lightProjectionSize,
		lightProjectionSize,
		0.10f,
		100.0f
	);
	XMStoreFloat4x4(&lightProjectionMatrix, lightProjection);

	// MATERIALS 
	// 
	// 
	// 
	// Material Tints
	float noTint[4] = { 0.0f,0.0f,0.0f,0.0f };
	float red[4] = { .5f,0.0f,0.0f,0.0f };
	float blue[4] = { 0.0f,0.0f,.5f,0.0f };
	float white[4] = { 1.0f, 1.0f, 1.0f, 0.0f };

	// Material UV Scales
	float standardSize[2] = { 1.0f,1.0f };
	float smallSize[2] = { 2.0f,2.0f };
	float smallestSize[2] = { 3.0f,3.0f };

	// Material Pointers
	std::shared_ptr<Materials> mat1 = std::make_shared<Materials>(white, .2f, vertexShader, pixelShader, 0, standardSize);
	std::shared_ptr<Materials> mat2 = std::make_shared<Materials>(white, .2f, vertexShader, pixelShader, 0, smallSize);
	std::shared_ptr<Materials> mat3 = std::make_shared<Materials>(white, .2f, vertexShader, pixelShader, 0, smallestSize);
	std::shared_ptr<Materials> normal = std::make_shared<Materials>(noTint, 0.0f, vertexShader, normalPS, 0, standardSize);
	std::shared_ptr<Materials> uv = std::make_shared<Materials>(noTint, 0.0f, vertexShader, uvPS, 0, standardSize);
	std::shared_ptr<Materials> position = std::make_shared<Materials>(noTint, 0.0f, vertexShader, customPS, 0, standardSize);

	// Texture 1 
	CreateWICTextureFromFile(Graphics::Device.Get(), Graphics::Context.Get(), FixPath(L"../../Assets/PBR/cobblestone_albedo.png").c_str(), nullptr, &cobbleAlbedoSRV);
	CreateWICTextureFromFile(Graphics::Device.Get(), Graphics::Context.Get(), FixPath(L"../../Assets/PBR/cobblestone_metal.png").c_str(), nullptr, &cobbleMetalSRV);
	CreateWICTextureFromFile(Graphics::Device.Get(), Graphics::Context.Get(), FixPath(L"../../Assets/PBR/cobblestone_normals.png").c_str(), nullptr, &cobbleNormalSRV);
	CreateWICTextureFromFile(Graphics::Device.Get(), Graphics::Context.Get(), FixPath(L"../../Assets/PBR/cobblestone_roughness.png").c_str(), nullptr, &cobbleRoughSRV);

	mat1->AddTextureSRV("Albedo", cobbleAlbedoSRV);
	mat1->AddTextureSRV("NormalMap", cobbleNormalSRV);
	mat1->AddTextureSRV("RoughnessMap", cobbleRoughSRV);
	mat1->AddTextureSRV("MetallnessMap", cobbleMetalSRV);
	mat1->AddSampler("BasicSampler", sampleS);


	// Texture 2
	CreateWICTextureFromFile(Graphics::Device.Get(), Graphics::Context.Get(), FixPath(L"../../Assets/PBR/scratched_albedo.png").c_str(), nullptr, &scratchedAlbedoSRV);
	CreateWICTextureFromFile(Graphics::Device.Get(), Graphics::Context.Get(), FixPath(L"../../Assets/PBR/scratched_metal.png").c_str(), nullptr, &scratchedMetalSRV);
	CreateWICTextureFromFile(Graphics::Device.Get(), Graphics::Context.Get(), FixPath(L"../../Assets/PBR/scratched_normals.png").c_str(), nullptr, &scratchedNormalSRV);
	CreateWICTextureFromFile(Graphics::Device.Get(), Graphics::Context.Get(), FixPath(L"../../Assets/PBR/scratched_roughness.png").c_str(), nullptr, &scratchedRoughSRV);

	mat2->AddTextureSRV("Albedo", scratchedAlbedoSRV);
	mat2->AddTextureSRV("NormalMap", scratchedNormalSRV);
	mat2->AddTextureSRV("RoughnessMap", scratchedRoughSRV);
	mat2->AddTextureSRV("MetallnessMap", scratchedMetalSRV);
	mat2->AddSampler("BasicSampler", sampleS);


	// Texture 3
	CreateWICTextureFromFile(Graphics::Device.Get(), Graphics::Context.Get(), FixPath(L"../../Assets/PBR/wood_albedo.png").c_str(), nullptr, &woodAlbedoSRV);
	CreateWICTextureFromFile(Graphics::Device.Get(), Graphics::Context.Get(), FixPath(L"../../Assets/PBR/wood_metal.png").c_str(), nullptr, &woodMetalSRV);
	CreateWICTextureFromFile(Graphics::Device.Get(), Graphics::Context.Get(), FixPath(L"../../Assets/PBR/wood_normals.png").c_str(), nullptr, &woodNormalSRV);
	CreateWICTextureFromFile(Graphics::Device.Get(), Graphics::Context.Get(), FixPath(L"../../Assets/PBR/wood_roughness.png").c_str(), nullptr, &woodRoughSRV);

	mat3->AddTextureSRV("Albedo", woodAlbedoSRV);
	mat3->AddTextureSRV("NormalMap", woodNormalSRV);
	mat3->AddTextureSRV("RoughnessMap", woodRoughSRV);
	mat3->AddTextureSRV("MetallnessMap", woodMetalSRV);
	mat3->AddSampler("BasicSampler", sampleS);

	mats.push_back(mat1);
	mats.push_back(mat2);
	mats.push_back(mat3);
	mats.push_back(normal);
	mats.push_back(uv);
	mats.push_back(position);

	// Sky
	std::shared_ptr<Mesh> cube = std::make_shared<Mesh>((FixPath(L"../../Assets/Models/cube.obj").c_str()));
	std::shared_ptr<Sky> sky = std::make_shared<Sky>(cube, sampleS, skyPS, skyVS,
		FixPath(L"../../Assets/Skies/Clouds Pink/right.png").c_str(),
		FixPath(L"../../Assets/Skies/Clouds Pink/left.png").c_str(),
		FixPath(L"../../Assets/Skies/Clouds Pink/up.png").c_str(),
		FixPath(L"../../Assets/Skies/Clouds Pink/down.png").c_str(),
		FixPath(L"../../Assets/Skies/Clouds Pink/front.png").c_str(),
		FixPath(L"../../Assets/Skies/Clouds Pink/back.png").c_str());
	skyBox = sky;


	CreateGeometry();

	// Set initial graphics API state
	//  - These settings persist until we change them
	//  - Some of these, like the primitive topology & input layout, probably won't change
	//  - Others, like setting shaders, will need to be moved elsewhere later
	{
		// Tell the input assembler (IA) stage of the pipeline what kind of
		// geometric primitives (points, lines or triangles) we want to draw.  
		// Essentially: "What kind of shape should the GPU draw with our vertices?"
		Graphics::Context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		// Ensure the pipeline knows how to interpret all the numbers stored in
		// the vertex buffer. For this course, all of your vertices will probably
		// have the same layout, so we can just set this once at startup.

		// Set the active vertex and pixel shaders
		//  - Once you start applying different shaders to different objects,
		//    these calls will need to happen multiple times per frame

	}
}


// --------------------------------------------------------
// Clean up memory or objects created by this class
// 
// Note: Using smart pointers means there probably won't
//       be much to manually clean up here!
// --------------------------------------------------------
Game::~Game()
{
	ImGui_ImplDX11_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();
}


// --------------------------------------------------------
// Loads shaders from compiled shader object (.cso) files
// and also created the Input Layout that describes our 
// vertex data to the rendering pipeline. 
// - Input Layout creation is done here because it must 
//    be verified against vertex shader byte code
// - We'll have that byte code already loaded below
// --------------------------------------------------------
void Game::LoadShaders()
{
	vertexShader = std::make_shared<SimpleVertexShader>(Graphics::Device,
		Graphics::Context, FixPath(L"VertexShader.cso").c_str());
	pixelShader = std::make_shared<SimplePixelShader>(Graphics::Device,
		Graphics::Context, FixPath(L"PixelShader.cso").c_str());
	customPS = std::make_shared<SimplePixelShader>(Graphics::Device,
		Graphics::Context, FixPath(L"customPS.cso").c_str());
	normalPS = std::make_shared<SimplePixelShader>(Graphics::Device,
		Graphics::Context, FixPath(L"normalPS.cso").c_str());
	uvPS = std::make_shared<SimplePixelShader>(Graphics::Device,
		Graphics::Context, FixPath(L"uvPS.cso").c_str());
	skyPS = std::make_shared<SimplePixelShader>(Graphics::Device,
		Graphics::Context, FixPath(L"SkyPixelShader.cso").c_str());
	skyVS = std::make_shared<SimpleVertexShader>(Graphics::Device,
		Graphics::Context, FixPath(L"SkyVertexShader.cso").c_str());
	shadowVS = std::make_shared<SimpleVertexShader>(Graphics::Device,
		Graphics::Context, FixPath(L"ShadowMapVertexShader.cso").c_str());
	shadowPS = std::make_shared<SimplePixelShader>(Graphics::Device,
		Graphics::Context, FixPath(L"ShadowMapPixelShader.cso").c_str());
	postPS = std::make_shared<SimplePixelShader>(Graphics::Device,
		Graphics::Context, FixPath(L"PostPS.cso").c_str());
	postVS = std::make_shared<SimpleVertexShader>(Graphics::Device,
		Graphics::Context, FixPath(L"PostVS.cso").c_str());
}


// --------------------------------------------------------
// Creates the geometry we're going to draw
// --------------------------------------------------------
void Game::CreateGeometry()
{
	// Old meshes
	{
		// Create some temporary variables to represent colors
		// - Not necessary, just makes things more readable
		XMFLOAT4 red = XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f);
		XMFLOAT4 green = XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f);
		XMFLOAT4 blue = XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f);
		XMFLOAT4 yellow = XMFLOAT4(1.0f, 1.0f, 0.0f, 1.0f);

		// Triangle 
		Vertex triVs[] =
		{
			{ XMFLOAT3(+0.0f, +0.1f, +0.0f),XMFLOAT3(0,0,-1),XMFLOAT3(0,0,0)},
			{ XMFLOAT3(+0.1f, -0.1f, +0.0f),XMFLOAT3(0,0,-1),XMFLOAT3(0,0,0)},
			{ XMFLOAT3(-0.1f, -0.1f, +0.0f),XMFLOAT3(0,0,-1),XMFLOAT3(0,0,0)},
		};
		int triIs[] = { 0,1,2 };
		std::shared_ptr<Mesh> triangle = std::make_shared<Mesh>(3, 3, triVs, triIs);
		entities.push_back(Entity(triangle, mats[0]));

		//// triforce
		//vertex trifvs[] =
		//{
		//	// top triangle
		//	{ xmfloat3(0.0f - .6f, +0.1f + 0.6f, 0.0f), yellow }, // top
		//	{ xmfloat3(+0.1f - .6f, -0.1f + 0.6f, 0.0f), yellow }, // btm r
		//	{ xmfloat3(-0.1f - .6f, -0.1f + 0.6f, 0.0f), yellow }, // btm l

		//	// bottom-left triangle
		//	{ xmfloat3(-0.1f - .6f, -0.1f + 0.6f, 0.0f), red },
		//	{ xmfloat3(0.0f - .6f, -0.3f + 0.6f, 0.0f), red },
		//	{ xmfloat3(-0.2f - .6f, -0.3f + 0.6f, 0.0f), red },

		//	// bottom-right triangle
		//	{ xmfloat3(+0.1f - .6f, -0.1f + 0.6f, 0.0f), blue },
		//	{ xmfloat3(+0.2f - .6f, -0.3f + 0.6f, 0.0f), blue },
		//	{ xmfloat3(0.0f - .6f, -0.3f + 0.6f, 0.0f), blue },
		//};
		//int trifis[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8 };
		//std::shared_ptr<mesh> triforce = std::make_shared<mesh>(9, 9, trifvs, trifis);

		//// 3 of these bad boys
		//for (int i = 0; i < 3; i++)
		//{
		//	entities.push_back(entity(triforce,mats[1]));
		//}
		//entities[2].gettransform()->moveabsolute(.3f, -.6f,0);
		//entities[3].gettransform()->moveabsolute(.1f, -.3f, 0);
		//// sqaure
		//vertex sqvs[] =
		//{
		//	// bottom left
		//	{ xmfloat3(+0.0f + .6f, +0.1f + .6f, +0.0f), red },
		//	{ xmfloat3(+0.1f + .6f, -0.1f + .6f, +0.0f), red },
		//	{ xmfloat3(+0.0f + .6f, -0.1f + .6f, +0.0f), red },

		//	// top right
		//	{ xmfloat3(+0.1f + .6f, +0.1f + .6f, +0.0f), red },
		//	{ xmfloat3(+0.1f + .6f, -0.1f + .6f, +0.0f), red },
		//	{ xmfloat3(-0.0f + .6f, +0.1f + .6f, +0.0f), red },
		//};
		//int sqis[] = { 0,1,2,3,4,5 };
		//std::shared_ptr<mesh> square = std::make_shared<mesh>(6, 6, sqvs, sqis);
		//entities.push_back(entity(square,mats[2]));
	}

	// Floor
	std::shared_ptr<Mesh> cube = std::make_shared<Mesh>((FixPath(L"../../Assets/Models/cube.obj").c_str()));
	entities.push_back(Entity(cube, mats[2]));
	entities[1].GetTransform()->SetPosition(2, -4, 0);
	entities[1].GetTransform()->SetScale(10, 0, 10);

	// Bottom Row
	std::shared_ptr<Mesh> sphere = std::make_shared<Mesh>((FixPath(L"../../Assets/Models/sphere.obj").c_str()));
	entities.push_back(Entity(sphere, mats[0]));
	entities[2].GetTransform()->SetPosition(4, -4, 0);

	std::shared_ptr<Mesh> helix = std::make_shared<Mesh>((FixPath(L"../../Assets/Models/helix.obj").c_str()));
	entities.push_back(Entity(helix, mats[0]));
	entities[3].GetTransform()->SetPosition(8, -4, 0);

	std::shared_ptr<Mesh> torus = std::make_shared<Mesh>((FixPath(L"../../Assets/Models/torus.obj").c_str()));
	entities.push_back(Entity(torus, mats[0]));
	entities[4].GetTransform()->SetPosition(0, -4, 0);

	std::shared_ptr<Mesh> cylinder = std::make_shared<Mesh>((FixPath(L"../../Assets/Models/cylinder.obj").c_str()));
	entities.push_back(Entity(cylinder, mats[0]));
	entities[5].GetTransform()->SetPosition(-4, -4, 0);

	// Middle
	entities.push_back(Entity(sphere, mats[1]));
	entities[6].GetTransform()->SetPosition(4, 0, 0);

	entities.push_back(Entity(helix, mats[1]));
	entities[7].GetTransform()->SetPosition(8, 0, 0);

	entities.push_back(Entity(torus, mats[1]));
	entities[8].GetTransform()->SetPosition(0, 0, 0);

	entities.push_back(Entity(cylinder, mats[1]));
	entities[9].GetTransform()->SetPosition(-4, 0, 0);

	// Top
	entities.push_back(Entity(sphere, mats[2]));
	entities[10].GetTransform()->SetPosition(4, 4, 0);

	entities.push_back(Entity(helix, mats[2]));
	entities[11].GetTransform()->SetPosition(8, 4, 0);

	entities.push_back(Entity(torus, mats[2]));
	entities[12].GetTransform()->SetPosition(0, 4, 0);

	entities.push_back(Entity(cylinder, mats[2]));
	entities[13].GetTransform()->SetPosition(-4, 4, 0);

}

void Game::PostSetup()
{
	postSRV.Reset();
	postRenderTarget.Reset();

	D3D11_TEXTURE2D_DESC textDesc = {};
	textDesc.Width = (unsigned int)(Window::Width());
	textDesc.Height = (unsigned int)(Window::Height());
	textDesc.ArraySize = 1;
	textDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
	textDesc.CPUAccessFlags = 0;
	textDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	textDesc.MipLevels = 1;
	textDesc.MiscFlags = 0;
	textDesc.SampleDesc.Count = 1;
	textDesc.SampleDesc.Quality = 0;
	textDesc.Usage = D3D11_USAGE_DEFAULT;
	
	Graphics::Device->CreateTexture2D(&textDesc,0, postTexture.GetAddressOf());

	D3D11_RENDER_TARGET_VIEW_DESC rtDesc = {};
	rtDesc.Format = textDesc.Format;
	rtDesc.Texture2D.MipSlice = 0;
	rtDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;

	Graphics::Device->CreateRenderTargetView(postTexture.Get(), &rtDesc, postRenderTarget.ReleaseAndGetAddressOf());

	Graphics::Device->CreateShaderResourceView(postTexture.Get(), 0, postSRV.ReleaseAndGetAddressOf());

}


// --------------------------------------------------------
// Handle resizing to match the new window size
//  - Eventually, we'll want to update our 3D camera
// --------------------------------------------------------
void Game::OnResize()
{
	if (!cams.empty())
	{
		for (int i = 0; i < cams.size(); i++)
		{
			cams[i]->UpdateProjectionMatrix((float)Window::Width() / Window::Height());
		}
	}

	if (Graphics::Device)
	{
		PostSetup();
	}
}

// UI 
void ImGuiUpdate(float deltaTime)
{
	// Feed data to ImGui
	ImGuiIO& io = ImGui::GetIO();
	io.DeltaTime = deltaTime;
	io.DisplaySize.x = (float)Window::Width();
	io.DisplaySize.y = (float)Window::Height();

	// Reset Frame
	ImGui_ImplDX11_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	// New Input Capture
	Input::SetKeyboardCapture(io.WantCaptureKeyboard);
	Input::SetMouseCapture(io.WantCaptureMouse);

}

static float dir1col[4] = { 1,0,0,0 };
static float dir2col[4] = { 0,0,1,0 };
static float dir3col[4] = { 0,1,0,0 };
static float point1col[4] = { 1,1,1,0 };
static float point2col[4] = { 1,1,1,0 };
static int blurAmount = 0;
static int fogType = 0;
static float fogColor[4] = { 0.5f, 0.5f, 0.5f };
static float fogStart = 20.0f;
static float fogEnd = 30.0f;
static float fogDensity = 0.02;
void Game::BuildUI()
{
	ImGui::Begin("Jon's Useful Stats & Config");

	ImGui::SeparatorText("Display Info");
	ImGui::Text("Frame rate: %f fps", ImGui::GetIO().Framerate);
	ImGui::Text("Res: %dx%d", Window::Width(), Window::Height());

	ImGui::SeparatorText("Shadow Map Texture");
	ImGui::Image(shadowSRV.Get(), ImVec2(512, 512));

	ImGui::SeparatorText("Post Processes");
	ImGui::Checkbox("Enable Blur", &blur);
	ImGui::SliderInt("Blur Amount", &blurAmount, 0, 50);
	ImGui::RadioButton("No Fog", &fogType, 0);
	ImGui::RadioButton("Limited Fog", &fogType, 1);
	ImGui::RadioButton("Silent Hill Mode(Exponential to Far Clip)", &fogType, 2);
	ImGui::ColorEdit3("Fog Color", fogColor);
	ImGui::SliderFloat("Begin Fog Distance", &fogStart, 0.0f, 100.0f);
	ImGui::SliderFloat("End Fog Distance", &fogEnd, 0.0f, 100.0f);
	ImGui::SliderFloat("Fog Density", &fogDensity, 0.0f,1.0f);

	ImGui::SeparatorText("Lights");
	ImGui::Text("Light color");
	ImGui::ColorEdit4("Directional Light 1", dir1col);
	ImGui::ColorEdit4("Directional Light 2", dir2col);
	ImGui::ColorEdit4("Directional Light 3", dir3col);
	ImGui::ColorEdit4("Point Light 1", point1col);
	ImGui::ColorEdit4("Point Light 2", point2col);
	directionalLight1.Color = XMFLOAT3(dir1col);
	directionalLight2.Color = XMFLOAT3(dir2col);
	directionalLight3.Color = XMFLOAT3(dir3col);
	pointLight1.Color = XMFLOAT3(point1col);
	pointLight2.Color = XMFLOAT3(point2col);

	ImGui::SeparatorText("Camera:");
	static int selected = 0;
	ImGui::RadioButton("Cam 1", &selected, 0);
	ImGui::RadioButton("Cam 2", &selected, 1);
	ImGui::RadioButton("Cam 3", &selected, 2);
	currentCam = cams[selected];

	ImGui::Text("FOV: %d", cams[selected]->GetFOV());
	ImGui::Text("Position:");
	ImGui::Text("X: %f", cams[selected]->GetTransform()->GetPosition().x);
	ImGui::Text("Y: %f", cams[selected]->GetTransform()->GetPosition().y);
	ImGui::Text("Z: %f", cams[selected]->GetTransform()->GetPosition().z);

	ImGui::SeparatorText("Entity Info");

	for (int i = 0; i < entities.size(); i++)
	{
		ImGui::Text("Entity %d:", i);
		ImGui::Text("Mesh info:");
		ImGui::Text("Triangles: %d", entities[i].GetMesh()->GetVertexCount() / 3);
		ImGui::Text("Vertices: %d", entities[i].GetMesh()->GetVertexCount());
		ImGui::Text("Indices: %d", entities[i].GetMesh()->GetIndexCount());

		XMFLOAT3 pos = entities[i].GetTransform()->GetPosition();
		XMFLOAT3 rot = entities[i].GetTransform()->GetPitchYawRoll();
		XMFLOAT3 sca = entities[i].GetTransform()->GetScale();
		ImGui::PushID(i);
		if (ImGui::DragFloat3("Position", &pos.x, 0.01f)) entities[i].GetTransform()->SetPosition(pos);
		if (ImGui::DragFloat3("Rotation", &rot.x, 0.01f)) entities[i].GetTransform()->SetRotation(rot);
		if (ImGui::DragFloat3("Scale", &sca.x, 0.01f)) entities[i].GetTransform()->SetScale(sca);
		ImGui::PopID();
	}

	ImGui::SeparatorText("Fun Features");
	// Rewrite Bg Color
	ImGui::ColorEdit4("BG Color", color);
	ImGui::ColorEdit4("Vertex Tint", tint);

	// Window toggle
	static bool show = false;
	if (ImGui::Button("Toggle Demo Window"))
	{

		show = !show;
	}
	// Cool Tool Tip!
	ImGui::SetItemTooltip("Opens a Demo of ImGui the current GUI you are interfacing with");
	if (show)
	{
		ImGui::ShowDemoWindow();
	}

	ImGui::End();
}


static float radius = 5.0f;
static float speed = 1.0f;
// --------------------------------------------------------
// Update your game here - user input, move objects, AI, etc.
// --------------------------------------------------------
void Game::Update(float deltaTime, float totalTime)
{
	ImGuiUpdate(deltaTime);
	BuildUI();

	static float angle = 0.0f;
	angle += speed * deltaTime;

	for (int i = 2; i < entities.size(); i++)
	{
		float offset = i * 0.1f;

		float dx = radius * cos(angle + offset) * deltaTime;
		float dz = radius * sin(angle + offset) * deltaTime;
		float dy = radius * sin(angle + offset) * deltaTime;

		entities[i].GetTransform()->MoveRelative(dx, dy, dz);
	}

	// Example input checking: Quit if the escape key is pressed
	if (Input::KeyDown(VK_ESCAPE))
		Window::Quit();
}




// --------------------------------------------------------
// Clear the screen, redraw everything, present to the user
// --------------------------------------------------------
void Game::Draw(float deltaTime, float totalTime)
{
	// Frame START
	// - These things should happen ONCE PER FRAME
	// - At the beginning of Game::Draw() before drawing *anything*
	{
		// Clear the back buffer (erase what's on screen) and depth buffer
		Graphics::Context->ClearRenderTargetView(Graphics::BackBufferRTV.Get(), color);
		Graphics::Context->ClearDepthStencilView(Graphics::DepthBufferDSV.Get(), D3D11_CLEAR_DEPTH, 1.0f, 0);
		Graphics::Context->ClearDepthStencilView(shadowDSV.Get(), D3D11_CLEAR_DEPTH, 1.0f, 0);
	}

	// Shadow Map Output Merger Shift
	ID3D11RenderTargetView* nullRTV{};
	Graphics::Context->OMSetRenderTargets(1, &nullRTV, shadowDSV.Get());



	// Switch Viewport
	D3D11_VIEWPORT viewport = {};
	viewport.Width = (float)shadowMapResolution;
	viewport.Height = (float)shadowMapResolution;
	viewport.MaxDepth = 1.0f;
	Graphics::Context->RSSetViewports(1, &viewport);



	// Render Shadows
	Graphics::Context->RSSetState(shadowRasterizer.Get());
	shadowVS->SetShader();
	shadowVS->SetMatrix4x4("view", lightViewMatrix);
	shadowVS->SetMatrix4x4("projection", lightProjectionMatrix);
	// Deactivate Pixel Shader
	Graphics::Context->PSSetShader(0, 0, 0);

	for (auto& e : entities)
	{
		shadowVS->SetMatrix4x4("world", e.GetTransform()->GetWorldMatrix());
		shadowVS->CopyAllBufferData();

		e.GetMesh()->Draw();
	}

	// Reset to Normal Rendering
	viewport.Width = (float)Window::Width();
	viewport.Height = (float)Window::Height();
	Graphics::Context->RSSetViewports(1, &viewport);
	Graphics::Context->OMSetRenderTargets(
		1,
		Graphics::BackBufferRTV.GetAddressOf(),
		Graphics::DepthBufferDSV.Get()
	);
	Graphics::Context->RSSetState(0);

	if (blur)
	{
		const float bg[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
		Graphics::Context->ClearRenderTargetView(postRenderTarget.Get(), bg);

		Graphics::Context->OMSetRenderTargets(1, postRenderTarget.GetAddressOf(), Graphics::DepthBufferDSV.Get());
	}


	// Entities Loop
	{
		currentCam->Update(deltaTime);

		for (int i = 0; i < entities.size(); i++)
		{
			entities[i].GetMaterial()->GetVertexShader()->SetShader();
			entities[i].GetMaterial()->GetPixelShader()->SetShader();
			std::shared_ptr<SimpleVertexShader> vs = entities[i].GetMaterial()->GetVertexShader();
			vs->SetMatrix4x4("world", entities[i].GetTransform()->GetWorldMatrix());
			vs->SetMatrix4x4("viewMat", currentCam->GetViewMatrix());
			vs->SetMatrix4x4("projMat", currentCam->GetProjectionMatrix());
			vs->SetMatrix4x4("worldInvTranspose", entities[i].GetTransform()->GetWorldInverseTransposeMatrix());
			vs->SetMatrix4x4("lightView", lightViewMatrix);
			vs->SetMatrix4x4("lightProj", lightProjectionMatrix);
			vs->CopyAllBufferData();

			std::shared_ptr<SimplePixelShader> ps = entities[i].GetMaterial()->GetPixelShader();
			entities[i].GetMaterial()->PrepareMaterial();
			ps->SetData("directionalLight1", &directionalLight1, sizeof(Light));
			ps->SetData("directionalLight2", &directionalLight2, sizeof(Light));
			ps->SetData("directionalLight3", &directionalLight3, sizeof(Light));
			ps->SetData("pointLight1", &pointLight1, sizeof(Light));
			ps->SetData("pointLight2", &pointLight2, sizeof(Light));
			ps->SetFloat("roughness", entities[i].GetMaterial()->GetRoughness());
			ps->SetFloat3("cameraPosition", currentCam->GetTransform()->GetPosition());
			ps->SetFloat4("colorTint", entities[i].GetMaterial()->GetColor());
			ps->SetFloat3("ambient", ambientColor);
			ps->SetSamplerState("ShadowSampler", shadowSampler);
			ps->SetShaderResourceView("ShadowMap", shadowSRV.Get());
			

			ps->SetFloat3("fogColor", fogColor);
			ps->SetFloat("fogDensity", fogDensity);
			ps->SetFloat("fogStart", fogStart);
			ps->SetFloat("fogEnd", fogEnd);
			ps->SetInt("fogType", fogType);
			ps->CopyAllBufferData();


			entities[i].GetMesh()->Draw();
		}

		skyBox->Draw(currentCam);
	}

	if (blur)
	{
		// reset render target
		Graphics::Context->OMSetRenderTargets(1, Graphics::BackBufferRTV.GetAddressOf(), 0);

		// Reset buffers and set black tri over everything
		UINT stride = sizeof(Vertex);
		UINT offset = 0;
		ID3D11Buffer* nothing = 0;
		Graphics::Context->IASetIndexBuffer(0, DXGI_FORMAT_R32_UINT, 0);
		Graphics::Context->IASetVertexBuffers(0, 1, &nothing, &stride, &offset);

		// bind shaders and setup srv and sampler
		postPS->SetShader();
		postVS->SetShader();

		postPS->SetShaderResourceView("PixelColors", postSRV);
		postPS->SetSamplerState("BasicSampler", postSampler);

		// cbuffer
		postPS->SetFloat("pixelWidth", 1.0f / Window::Width());
		postPS->SetFloat("pixelHeight", 1.0f / Window::Height());
		postPS->SetInt("blurDistance", blurAmount);
		postPS->CopyAllBufferData();

		Graphics::Context->Draw(3, 0);

		// Reset Resource View
		ID3D11ShaderResourceView* nullSRVs[16] = {};
		Graphics::Context->PSSetShaderResources(0, 16, nullSRVs);
	}


	// Frame END
	// - These should happen exactly ONCE PER FRAME
	// - At the very end of the frame (after drawing *everything*)
	{

		// Present at the end of the frame
		bool vsync = Graphics::VsyncState();
		ImGui::Render(); // Turns UI into renderable Tris
		ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData()); // Draw to screen

		Graphics::SwapChain->Present(
			vsync ? 1 : 0,
			vsync ? 0 : DXGI_PRESENT_ALLOW_TEARING);

		// Re-bind back buffer and depth buffer after presenting
		Graphics::Context->OMSetRenderTargets(
			1,
			Graphics::BackBufferRTV.GetAddressOf(),
			Graphics::DepthBufferDSV.Get());

		ID3D11ShaderResourceView* nullSRVs[128] = {};
		Graphics::Context->PSSetShaderResources(0, 128, nullSRVs);
	}


}



