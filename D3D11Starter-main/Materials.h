#pragma once
#include <memory>
#include "SimpleShader.h"
#include <unordered_map>

class Materials
{
private:
	float color[4];
	float roughness;
	std::shared_ptr<SimpleVertexShader> vertex;
	std::shared_ptr<SimplePixelShader> pixel;
	std::unordered_map<std::string, Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>> textureSRVs;
	std::unordered_map<std::string, Microsoft::WRL::ComPtr<ID3D11SamplerState>> samplers;
	int specMap;
	float uvScale[2];

public:
	Materials(float tint[4], float rough, std::shared_ptr<SimpleVertexShader> vertexShader, std::shared_ptr<SimplePixelShader> pixelShader,int specTF,float texSize[2])
	{
		for (int i = 0; i < 4; i++)
		{
			color[i] = tint[i];
		}

		for (int i = 0; i < 2; i++)
		{
			uvScale[i] = texSize[i];
		}
		roughness = rough;
		vertex = vertexShader;
		pixel = pixelShader;

		if (rough > 1) rough = 1;
		if (rough < 0) rough = 0;

		specMap = specTF;

	}

	void PrepareMaterial()
	{
		for (auto& t : textureSRVs) { this->GetPixelShader()->SetShaderResourceView(t.first.c_str(), t.second); }
		for (auto& s : samplers) { this->GetPixelShader()->SetSamplerState(s.first.c_str(), s.second); }
		this->GetPixelShader()->SetInt("specMap", specMap);
		this->GetPixelShader()->SetFloat2("uvScale", uvScale);
	}

	std::shared_ptr<SimpleVertexShader> GetVertexShader()
	{
		return vertex;
	}

	void SetVertexShader(std::shared_ptr<SimpleVertexShader> vertexShader)
	{
		vertex = vertexShader;
	}

	void SetPixelShader(std::shared_ptr<SimplePixelShader> pixelShader)
	{
		pixel = pixelShader;
	}

	std::shared_ptr<SimplePixelShader> GetPixelShader()
	{
		return pixel;
	}

	float* GetColor()
	{
		return color;
	}

	void SetColor(float tint[4])
	{
		for (int i = 0; i < 4; i++)
		{
			color[i] = tint[i];
		}
	}

	float GetRoughness()
	{
		return roughness;
	}

	void SetRoughness(float rough)
	{
		if (rough > 1) rough = 1;
		if (rough < 0) rough = 0;
		roughness = rough;
	}

	void AddTextureSRV(std::string name, Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srv)
	{
		textureSRVs.insert({ name,srv });
	}

	void AddSampler(std::string name, Microsoft::WRL::ComPtr<ID3D11SamplerState> sampler)
	{
		samplers.insert({ name,sampler });
	}
};

