#pragma once
#include "DirectXMath.h"
#include "wrl/client.h"
#include "d3d11.h"
#include "Mesh.h"
#include "SimpleShader.h"
#include "WICTextureLoader.h"
#include "PathHelpers.h"
#include "Camera.h"
#include <memory>

using Microsoft::WRL::ComPtr;

class Sky
{
private: 
	ComPtr<ID3D11SamplerState> samplerState;
	ComPtr<ID3D11ShaderResourceView> srv;
	ComPtr<ID3D11DepthStencilState> depthStencilState;
	ComPtr<ID3D11RasterizerState> rasterizerState;
	std::shared_ptr<Mesh> mesh;
	std::shared_ptr<SimplePixelShader> ps;
	std::shared_ptr<SimpleVertexShader> vs;

public:
	Sky(std::shared_ptr<Mesh> box,ComPtr<ID3D11SamplerState> sampler, std::shared_ptr<SimplePixelShader> pixelShader, std::shared_ptr<SimpleVertexShader> vertexShader,
		const wchar_t* right,
		const wchar_t* left,
		const wchar_t* up,
		const wchar_t* down,
		const wchar_t* front,
		const wchar_t* back)
	{
		samplerState = sampler;
		mesh = box;
		ps = pixelShader;
		vs = vertexShader;

		D3D11_RASTERIZER_DESC rDesc = {};
		rDesc.FillMode = D3D11_FILL_SOLID;
		rDesc.CullMode = D3D11_CULL_FRONT;
		Graphics::Device->CreateRasterizerState(&rDesc,&rasterizerState);

		D3D11_DEPTH_STENCIL_DESC dsDesc = {};
		dsDesc.DepthEnable = true;
		dsDesc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;
		Graphics::Device->CreateDepthStencilState(&dsDesc, &depthStencilState);

		srv = CreateCubemap(right, left, up, down, front, back);
	};

	~Sky();

	void Draw(std::shared_ptr<Camera> cam);
// --------------------------------------------------------
// Author: Chris Cascioli
// Purpose: Creates a cube map on the GPU from 6 individual textures
// 
// - You are allowed to directly copy/paste this into your code base
//   for assignments, given that you clearly cite that this is not
//   code of your own design.
//
// - Note: This code assumes you’re putting the function in Sky.cpp, 
//   you’ve included WICTextureLoader.h and you have an ID3D11Device 
//   ComPtr called “device”.  Make any adjustments necessary for
//   your own implementation.
// --------------------------------------------------------

// Helper for creating a cubemap from 6 individual textures
Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> CreateCubemap(
	const wchar_t* right,
	const wchar_t* left,
	const wchar_t* up,
	const wchar_t* down,
	const wchar_t* front,
	const wchar_t* back);
};

