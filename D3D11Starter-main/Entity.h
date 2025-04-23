#pragma once
#include <memory>
#include "Transform.h"
#include "Mesh.h"
#include "d3d11.h"
#include "Materials.h"

class Entity
{
private:
	Transform transform;
	std::shared_ptr<Mesh> mesh;
	std::shared_ptr<Materials> mat;

public:
	Entity(std::shared_ptr<Mesh> mesh,std::shared_ptr<Materials> material);

	std::shared_ptr<Mesh> GetMesh();
	Transform* GetTransform();
	std::shared_ptr<Materials> GetMaterial();
	void SetMaterial(std::shared_ptr<Materials> material);

	void Draw(D3D11_MAPPED_SUBRESOURCE cbuffer);
};

