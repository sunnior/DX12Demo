#include "Camera.h"
#include "Engine.h"

using namespace DirectX;

Camera::Camera(float aspectRatio)
	: m_aspectRatio(aspectRatio)
	, m_upward(0.0f, 1.0f, 0.0f, 0.0f)
	, m_forward(0.0f, 0.0f, -1.0f, 0.0f)\
	, m_pos(0.0f, 1.0f, 5.0f, 1.0f)
{
	//XMVECTOR eye = { 0.0f, 1.0f, 5.0f, 1.0f };
	//XMVECTOR at = { 0.0f, 0.0f, 0.0f, 1.0f };

	//XMVECTOR right = { 1.0f, 0.0f, 0.0f, 0.0f };
	//XMVECTOR direction = { 0.0f, 0.0f, -1.0f, 0.0f };//XMVector4Normalize(at - eye);
	//XMVECTOR at = eye + direction;
	//XMVECTOR up = XMLoadFloat4(&m_upward);

	//XMVECTOR up = XMVector3Normalize(XMVector3Cross(direction, right));

	// Rotate camera around Y axis.
	//XMMATRIX rotate = XMMatrixRotationY(XMConvertToRadians(45.0f));
	//eye = XMVector3Transform(eye, rotate);
	//up = XMVector3Transform(up, rotate);

	//XMStoreFloat4(&m_forward, direction);
	//XMStoreFloat4(&m_pos, eye);
	//XMStoreFloat4(&m_upward, up);
	_UpdateMatrix();
}

Camera::~Camera()
{

}

void Camera::Update(float dt)
{
	bool isWPressed = Engine::GetInstance()->GetKeyboard().IsPressed(Keyboard::KeyCode::W);
	bool isSPressed = Engine::GetInstance()->GetKeyboard().IsPressed(Keyboard::KeyCode::S);

	if (isWPressed || isSPressed)
	{
		float speed = 1.0f;
		if (isSPressed)
		{
			speed *= -1;
		}
		XMVECTOR pos = XMLoadFloat4(&m_pos);
		XMVECTOR forward = XMLoadFloat4(&m_forward);
		pos += forward * speed * dt;
		XMStoreFloat4(&m_pos, pos);
	}

	bool isQPressed = Engine::GetInstance()->GetKeyboard().IsPressed(Keyboard::KeyCode::Q);
	bool isEPressed = Engine::GetInstance()->GetKeyboard().IsPressed(Keyboard::KeyCode::E);
	
	if (isQPressed || isEPressed)
	{
		float speed = 1.0f;
		if (isEPressed)
		{
			speed *= -1;
		}
		XMVECTOR pos = XMLoadFloat4(&m_pos);
		XMVECTOR upward = XMLoadFloat4(&m_upward);
		pos += upward * speed * dt;
		XMStoreFloat4(&m_pos, pos);
	}

	bool isAPressed = Engine::GetInstance()->GetKeyboard().IsPressed(Keyboard::KeyCode::A);
	bool isDPressed = Engine::GetInstance()->GetKeyboard().IsPressed(Keyboard::KeyCode::D);

	_UpdateMatrix();
}

void Camera::_UpdateMatrix()
{
	float fovAngleY = 45.0f;

	XMVECTOR eye = XMLoadFloat4(&m_pos);
	XMVECTOR at = eye + XMLoadFloat4(&m_forward);
	XMVECTOR up = XMLoadFloat4(&m_upward);

	XMMATRIX view = XMMatrixLookAtLH(eye, at, up);
	XMMATRIX proj = XMMatrixPerspectiveFovLH(XMConvertToRadians(fovAngleY), m_aspectRatio, 1.0f, 125.0f);
	XMMATRIX viewProj = view * proj;

	XMMATRIX projectionToWorld = XMMatrixInverse(nullptr, viewProj);

	XMStoreFloat4x4(&m_projectMatrix, proj);
	XMStoreFloat4x4(&m_projectionToWorld, projectionToWorld);
}
