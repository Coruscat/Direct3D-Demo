#include "Entity.h"

Entity::Entity(std::shared_ptr<Mesh> mesh, std::shared_ptr<Materials> material) 
{
	this->mesh = mesh;
	mat = material;
}

std::shared_ptr<Mesh> Entity::GetMesh()
{
	return mesh;
}

Transform* Entity::GetTransform()
{
	return &transform;
}

std::shared_ptr<Materials> Entity::GetMaterial()
{
	return mat;
}

void Entity::SetMaterial(std::shared_ptr<Materials> material)
{
	mat = material;
}

void Entity::Draw(D3D11_MAPPED_SUBRESOURCE cbuffer)
{

}
