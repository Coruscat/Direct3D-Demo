#include "Transform.h"
using namespace DirectX;
Transform::Transform() :
	position(0, 0, 0),
	scale(1, 1, 1),
	pitchYawRoll(0, 0, 0)
{
	XMStoreFloat4x4(&worldMatrix, XMMatrixIdentity());
	XMStoreFloat4x4(&inverseWorldMatrix, XMMatrixIdentity());
}

// Getters
DirectX::XMFLOAT4X4 Transform::GetWorldMatrix()
{
	// Skip math if matrix isn't changed?
	// Create the three matrices
	XMMATRIX tr = XMMatrixTranslationFromVector(XMLoadFloat3(&position)); // like this
	XMMATRIX sc = XMMatrixScalingFromVector(XMLoadFloat3(&scale));
	XMMATRIX pyr = XMMatrixRotationRollPitchYawFromVector(XMLoadFloat3(&pitchYawRoll));

	// Combine and store
	XMStoreFloat4x4(&worldMatrix, XMMatrixMultiply(XMMatrixMultiply(sc, pyr), tr)); // Scale * Rotate * Translate

	return worldMatrix;
}

DirectX::XMFLOAT4X4 Transform::GetWorldInverseTransposeMatrix()
{
	// Create the three matrices
	XMMATRIX tr = XMMatrixTranslationFromVector(XMLoadFloat3(&position));
	XMMATRIX sc = XMMatrixScalingFromVector(XMLoadFloat3(&scale));
	XMMATRIX pyr = XMMatrixRotationRollPitchYawFromVector(XMLoadFloat3(&pitchYawRoll));

	// Combine and store
	XMStoreFloat4x4(&inverseWorldMatrix, XMMatrixInverse(
		0, XMMatrixTranspose(
			XMMatrixMultiply(XMMatrixMultiply(sc, pyr), tr)
		)));

	return inverseWorldMatrix;
}

DirectX::XMFLOAT3 Transform::GetPosition()
{
	return position;
}

DirectX::XMFLOAT3 Transform::GetPitchYawRoll()
{
	return pitchYawRoll;
}

DirectX::XMFLOAT3 Transform::GetScale()
{
	return scale;
}

DirectX::XMFLOAT3 Transform::GetRight()
{
	XMVECTOR right = XMVectorSet(1, 0, 0, 0);
	XMVECTOR curRot = XMQuaternionRotationRollPitchYaw(pitchYawRoll.x, pitchYawRoll.y, pitchYawRoll.z);

	XMFLOAT3 rotatedRight;
	XMStoreFloat3(&rotatedRight, XMVector3Rotate(right, curRot));

	return rotatedRight;
}

DirectX::XMFLOAT3 Transform::GetUp()
{
	XMVECTOR up = XMVectorSet(0, 1, 0, 0);
	XMVECTOR curRot = XMQuaternionRotationRollPitchYaw(pitchYawRoll.x, pitchYawRoll.y, pitchYawRoll.z);

	XMFLOAT3 rotatedUp;
	XMStoreFloat3(&rotatedUp, XMVector3Rotate(up, curRot));
	return rotatedUp;
}

DirectX::XMFLOAT3 Transform::GetForward()
{
	XMVECTOR forward = XMVectorSet(0, 0, 1, 0);
	XMVECTOR curRot = XMQuaternionRotationRollPitchYaw(pitchYawRoll.x, pitchYawRoll.y, pitchYawRoll.z);

	XMFLOAT3 rotatedForward;
	XMStoreFloat3(&rotatedForward, XMVector3Rotate(forward, curRot));
	return rotatedForward;
}

// Setters
void Transform::SetPosition(float x, float y, float z)
{
	position.x = x;
	position.y = y;
	position.z = z;
}

void Transform::SetPosition(DirectX::XMFLOAT3 pos)
{
	position = pos;
}

void Transform::SetRotation(float pitch, float yaw, float roll)
{
	pitchYawRoll.x = pitch;
	pitchYawRoll.y = yaw;
	pitchYawRoll.z = roll;
}

void Transform::SetRotation(DirectX::XMFLOAT3 rotation)
{
	pitchYawRoll = rotation;
}

void Transform::SetScale(float x, float y, float z)
{
	scale.x = x;
	scale.y = y;
	scale.z = z;
}

void Transform::SetScale(DirectX::XMFLOAT3 scale)
{
	this->scale = scale;
}

// Transformers
void Transform::MoveAbsolute(float x, float y, float z)
{
	position.x += x;
	position.y += y;
	position.z += z;
}

void Transform::MoveAbsolute(DirectX::XMFLOAT3 offset)
{
	position.x += offset.x;
	position.y += offset.y;
	position.z += offset.z;
}

void Transform::MoveRelative(float x, float y, float z)
{
	XMVECTOR offset = XMVectorSet(x, y, z, 0);
	XMVECTOR curRot = XMQuaternionRotationRollPitchYaw(pitchYawRoll.x, pitchYawRoll.y, pitchYawRoll.z);

	XMFLOAT3 rotatedOffset;
	XMStoreFloat3(&rotatedOffset, XMVector3Rotate(offset, curRot));

	position.x += rotatedOffset.x;
	position.y += rotatedOffset.y;
	position.z += rotatedOffset.z;
}

void Transform::MoveRelative(DirectX::XMFLOAT3 offset)
{
	XMVECTOR move = XMVectorSet(offset.x, offset.y, offset.z,0);
	XMVECTOR curRot = XMQuaternionRotationRollPitchYaw(pitchYawRoll.x, pitchYawRoll.y, pitchYawRoll.z);

	XMFLOAT3 rotatedOffset;
	XMStoreFloat3(&rotatedOffset, XMVector3Rotate(move, curRot));

	position.x += rotatedOffset.x;
	position.y += rotatedOffset.y;
	position.z += rotatedOffset.z;
}

void Transform::Rotate(float pitch, float yaw, float roll)
{
	pitchYawRoll.x += pitch;
	pitchYawRoll.y += yaw;
	pitchYawRoll.z += roll;
}

void Transform::Rotate(DirectX::XMFLOAT3 rotate)
{
	pitchYawRoll.x += rotate.x;
	pitchYawRoll.y += rotate.y;
	pitchYawRoll.z += rotate.z;
}

void Transform::Scale(float x, float y, float z)
{
	scale.x *= x;
	scale.y *= y;
	scale.z *= z;
}

void Transform::Scale(DirectX::XMFLOAT3 scale)
{
	scale.x *= scale.x;
	scale.y *= scale.y;
	scale.z *= scale.z;
}
