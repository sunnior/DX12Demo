#ifndef __CAMERA_H__
#define __CAMERA_H__

#include <DirectXMath.h>

class Camera
{
public:
	Camera(float aspectRatio);
	~Camera();

	DirectX::XMFLOAT4 GetPosition() { return m_pos; }
	DirectX::XMFLOAT4X4 GetProjectionToWorld() { return m_projectionToWorld; }

	void Update(float dt);

private:
	void _UpdateMatrix();
private:
	DirectX::XMFLOAT4 m_pos;
	const DirectX::XMFLOAT4 m_upward;
	DirectX::XMFLOAT4 m_forward;

	DirectX::XMFLOAT4X4 m_projectionToWorld;
	DirectX::XMFLOAT4X4 m_projectMatrix;

	float m_aspectRatio{ 0.0f };

};

#endif