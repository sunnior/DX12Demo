#ifndef __CAMERA_H__
#define __CAMERA_H__

#include <DirectXMath.h>

class Camera
{
public:
	Camera(float aspectRatio);
	~Camera();

	DirectX::XMFLOAT4 GetPosition() { return m_eye; }
	DirectX::XMFLOAT4X4 GetProjectionToWorld() { return m_projectionToWorld; }

	void Update();

private:
	DirectX::XMFLOAT4 m_eye;
	DirectX::XMFLOAT4 m_up;

	DirectX::XMFLOAT4X4 m_projectionToWorld;
	DirectX::XMFLOAT4X4 m_projectMatrix;

	float m_aspectRatio{ 0.0f };

};

#endif