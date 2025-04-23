#pragma once

#include <d3d11.h>
#include <wrl/client.h>
#include <memory>
#include "SimpleShader.h"


class Game
{
public:
	// Basic OOP setup
	Game() = default;
	~Game();
	Game(const Game&) = delete; // Remove copy constructor
	Game& operator=(const Game&) = delete; // Remove copy-assignment operator

	// Primary functions
	void Initialize();
	void Update(float deltaTime, float totalTime);
	void Draw(float deltaTime, float totalTime);
	void OnResize();
	void BuildUI();
private:

	// Initialization helper methods - feel free to customize, combine, remove, etc.
	void LoadShaders();
	void CreateGeometry();
	void PostSetup();

	// Note the usage of ComPtr below
	//  - This is a smart pointer for objects that abide by the
	//     Component Object Model, which DirectX objects do
	//  - More info here: https://github.com/Microsoft/DirectXTK/wiki/ComPtr

	// Buffers to hold actual geometry data
	Microsoft::WRL::ComPtr<ID3D11Buffer> vertexBuffer;
	Microsoft::WRL::ComPtr<ID3D11Buffer> indexBuffer;

	//Post Processing Resources
	Microsoft::WRL::ComPtr<ID3D11Texture2D> postTexture;
	Microsoft::WRL::ComPtr<ID3D11RenderTargetView> postRenderTarget;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> postSRV;
	Microsoft::WRL::ComPtr<ID3D11SamplerState> postSampler;

	// Shaders and shader-related constructs
	std::shared_ptr<SimplePixelShader> pixelShader;
	std::shared_ptr<SimpleVertexShader> vertexShader;
	std::shared_ptr<SimplePixelShader> skyPS;
	std::shared_ptr<SimpleVertexShader> skyVS;
	std::shared_ptr<SimpleVertexShader> shadowVS;
	std::shared_ptr<SimplePixelShader> shadowPS;
	std::shared_ptr<SimplePixelShader> normalPS;
	std::shared_ptr<SimplePixelShader> uvPS;
	std::shared_ptr<SimplePixelShader> customPS;
	std::shared_ptr<SimplePixelShader> postPS;
	std::shared_ptr<SimpleVertexShader> postVS;
	Microsoft::WRL::ComPtr<ID3D11InputLayout> inputLayout;

	// Shadow Mapping
	Microsoft::WRL::ComPtr<ID3D11DepthStencilView> shadowDSV;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> shadowSRV;
	Microsoft::WRL::ComPtr<ID3D11RasterizerState> shadowRasterizer;
	Microsoft::WRL::ComPtr<ID3D11SamplerState> shadowSampler;
	DirectX::XMFLOAT4X4 lightViewMatrix;
	DirectX::XMFLOAT4X4 lightProjectionMatrix;

	// Texture 1 SRVs
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> cobbleAlbedoSRV;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> cobbleNormalSRV;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> cobbleRoughSRV;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> cobbleMetalSRV;
	// Texture 2 SRVs
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> scratchedAlbedoSRV;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> scratchedNormalSRV;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> scratchedRoughSRV;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> scratchedMetalSRV;
	// Texture 3 SRVs
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> woodAlbedoSRV;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> woodNormalSRV;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> woodRoughSRV;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> woodMetalSRV;

	Microsoft::WRL::ComPtr<ID3D11SamplerState> sampleS;
};

