#pragma once
#include <d3d11.h>
#include "DirectXMath.h"
using namespace DirectX;

#define LIGHT_TYPE_DIRECTIONAL 0
#define LIGHT_TYPE_POINT 1
#define LIGHT_TYPE_SPOT 2

struct Light
{
	int Type;
	XMFLOAT3 Direction;
	float Range;
	XMFLOAT3 Position;
	float Intensity;
	XMFLOAT3 Color;
	float SpotInnerAngle;
	float SpotOuterAngle;
	XMFLOAT2 Padding;
};

