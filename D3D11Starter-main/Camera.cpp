#include "Camera.h"

using namespace DirectX;
Camera::Camera(float aspect, DirectX::XMFLOAT3 initialPos)
{
	transform.SetPosition(initialPos);

	UpdateViewMatrix();
	UpdateProjectionMatrix(aspect);
}

DirectX::XMFLOAT4X4 Camera::GetViewMatrix()
{
	return viewMat;
}

DirectX::XMFLOAT4X4 Camera::GetProjectionMatrix()
{
	return projMat;
}

float Camera::GetMouseSpeed()
{
	return mouseSpeed;
}

float Camera::GetFOV()
{
	return fov;
}

Transform* Camera::GetTransform()
{
	return &transform;
}

void Camera::SetMouseSpeed(float x)
{
	mouseSpeed = x;
}

void Camera::SetFOV(float x)
{
	fov = x;
}

void Camera::UpdateProjectionMatrix(float aspectRatio)
{
	XMStoreFloat4x4(&projMat, XMMatrixPerspectiveFovLH(fov, aspectRatio, nearPlane, farPlane));
}

void Camera::UpdateViewMatrix()
{
	XMFLOAT3 position = transform.GetPosition();
	XMVECTOR direction = XMVectorSet(transform.GetForward().x, transform.GetForward().y, transform.GetForward().z, 0);
	XMVECTOR up = XMVectorSet(0, 1, 0, 0);
	XMVECTOR eye = XMVectorSet(position.x, position.y, position.z, 0);
	XMStoreFloat4x4(&viewMat, XMMatrixLookToLH(eye, direction, up));
}

void Camera::Update(float deltaTime)
{
	if (Input::KeyDown('W')) { transform.MoveRelative(XMFLOAT3(0, 0, moveSpeed * deltaTime)); }
	if (Input::KeyDown('S')) { transform.MoveRelative(XMFLOAT3(0, 0, -moveSpeed * deltaTime)); }
	if (Input::KeyDown('A')) { transform.MoveRelative(XMFLOAT3(-moveSpeed * deltaTime, 0, 0)); }
	if (Input::KeyDown('D')) { transform.MoveRelative(XMFLOAT3(moveSpeed * deltaTime, 0, 0)); }
	if (Input::KeyDown(VK_SPACE)) { transform.MoveRelative(XMFLOAT3(0, moveSpeed * deltaTime, 0)); }
	if (Input::KeyDown(VK_SHIFT)) { transform.MoveRelative(XMFLOAT3(0, -moveSpeed * deltaTime, 0)); }

	if (Input::MouseLeftDown())
	{
		int cursorMovementX = Input::GetMouseXDelta();
		int cursorMovementY = Input::GetMouseYDelta();

		float rotationX = cursorMovementX * mouseSpeed;
		float rotationY = cursorMovementY * mouseSpeed;

	// Update the current rotation with the new values
	float currentRotationY = 0.0f;

    currentRotationY += rotationY;

    // Clamp the vertical rotation
	if (currentRotationY > XM_PIDIV2)
	{
		currentRotationY = XM_PIDIV2;
	}
	else if (currentRotationY < -XM_PIDIV2)
	{
		currentRotationY = -XM_PIDIV2;
	}

    // Update the transform with the new clamped rotations
    transform.Rotate(currentRotationY, rotationX, 0);
	}

	UpdateViewMatrix();
}
