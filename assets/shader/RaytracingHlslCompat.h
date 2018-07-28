#ifndef __RAYTRACINGHLSLCOMPAT_HEADER_H__
#define __RAYTRACINGHLSLCOMPAT_HEADER_H__

#ifdef LANGUAGE_CPP
#include <DirectXMath.h>
struct SceneConstantBuffer
{
	DirectX::XMFLOAT4X4 projectionToWorld;
	DirectX::XMFLOAT4 cameraPosition;
};
#else
struct SceneConstantBuffer
{
	row_major float4x4 projectionToWorld;
	float4 cameraPosition;
};
#endif


#endif