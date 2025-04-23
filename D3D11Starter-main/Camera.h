#pragma once
#include "Transform.h"
#include "Input.h"

class Camera
{
private:
	Transform transform;
	DirectX::XMFLOAT4X4 viewMat;
	DirectX::XMFLOAT4X4 projMat;
	float fov = DirectX::XM_PI/3;
	float nearPlane = 0.1f;
	float farPlane = 100;
	float moveSpeed = 10;
	float mouseSpeed = .01f;
	bool orthoView = false;

public:
	Camera(float aspect, DirectX::XMFLOAT3 initialPos);

	// Getters
	DirectX::XMFLOAT4X4 GetViewMatrix();
	DirectX::XMFLOAT4X4 GetProjectionMatrix();
	float GetMouseSpeed();
	float GetFOV();
	Transform* GetTransform();


	// Setters
	void SetMouseSpeed(float x);
	void SetFOV(float x);

	// Methods
	void UpdateProjectionMatrix(float aspectRatio);
	void UpdateViewMatrix();

	void Update(float deltaTime);
};

