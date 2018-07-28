#include "Camera.h"
#include "Engine.h"

using namespace DirectX;

Camera::Camera(float aspectRatio)
	: m_aspectRatio(aspectRatio)
{
	XMVECTOR eye = { 0.0f, 0.0f, 5.0f, 1.0f };
	//XMVECTOR at = { 0.0f, 0.0f, 0.0f, 1.0f };

	XMVECTOR right = { 1.0f, 0.0f, 0.0f, 0.0f };
	XMVECTOR direction = { 0.0f, 0.0f, -1.0f, 0.0f };//XMVector4Normalize(at - eye);
	XMVECTOR at = eye + direction;

	XMVECTOR up = XMVector3Normalize(XMVector3Cross(direction, right));

	// Rotate camera around Y axis.
	//XMMATRIX rotate = XMMatrixRotationY(XMConvertToRadians(45.0f));
	//eye = XMVector3Transform(eye, rotate);
	//up = XMVector3Transform(up, rotate);

	XMStoreFloat4(&m_eye, eye);
	XMStoreFloat4(&m_up, up);

	float fovAngleY = 45.0f;

	XMMATRIX view = XMMatrixLookAtLH(eye, at, up);
	XMMATRIX proj = XMMatrixPerspectiveFovLH(XMConvertToRadians(fovAngleY), m_aspectRatio, 1.0f, 125.0f);
	XMMATRIX viewProj = view * proj;

	XMMATRIX projectionToWorld = XMMatrixInverse(nullptr, viewProj);

	XMStoreFloat4x4(&m_projectMatrix, proj);
	XMStoreFloat4x4(&m_projectionToWorld, projectionToWorld);
}

Camera::~Camera()
{

}

void Camera::Update()
{
	bool isWPressed = Engine::GetInstance()->GetKeyboard().IsPressed(Keyboard::KeyCode::W);
	if (isWPressed)
	{
		int a = 0;
	}
}
