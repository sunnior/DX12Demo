#ifndef __RAYTRACINGHLSLCOMPAT_HEADER_H__
#define __RAYTRACINGHLSLCOMPAT_HEADER_H__

#ifdef LANGUAGE_HLSL
typedef float4 DirectX::XMFLOAT4;
typedef float4x4 DirectX::XMFLOAT4x4;
#endif

struct SceneConstantBuffer
{
	DirectX::XMFLOAT4X4 projectionToWorld;
	DirectX::XMFLOAT4 cameraPosition;
};

#endif