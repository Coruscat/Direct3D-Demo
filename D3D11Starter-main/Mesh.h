#pragma once
#include <d3d11.h>
#include <wrl/client.h>
#include "Graphics.h"
#include "Vertex.h"
#include <fstream>
#include <memory>
#include <vector>
#include "DirectXMath.h"

using Microsoft::WRL::ComPtr;

class Mesh
{
private:
    ComPtr<ID3D11Buffer> vertexBuffer;
    ComPtr<ID3D11Buffer> indexBuffer;
    int indices;
    int vertices;

public:
    Mesh(int i, int v, Vertex points[], int pointOrder[]);

	Mesh(const std::wstring name);

    // Destructor
    ~Mesh();

	// Getters
	const ComPtr<ID3D11Buffer> GetVertexBuffer()
	{
		return vertexBuffer;
	}

	const ComPtr<ID3D11Buffer> GetIndexBuffer()
	{
		return indexBuffer;
	}

	int GetIndexCount() const
	{
		return indices;
	}

	int GetVertexCount() const
	{
		return vertices;
	}

    // Methods
    void Draw();

	void CalculateTangents(Vertex* verts, int numVerts, unsigned int* indices, int numIndices);
};