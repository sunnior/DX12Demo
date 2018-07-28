#include "RaytracingHlslCompat.h"

RaytracingAccelerationStructure Scene : register(t0, space0);
RWTexture2D<float4> RenderTarget : register(u0);

struct Viewport
{
    float2 topLeft;
    float2 bottomRight;
};

ConstantBuffer<SceneConstantBuffer> g_sceneCB: register(b0);

typedef BuiltInTriangleIntersectionAttributes MyAttributes;
struct RayPayload
{
	float4 color;
};

inline void GenerateCameraRay(uint2 index, out float3 origin, out float3 direction)
{
	float2 xy = index + 0.5f; // center in the middle of the pixel.
	float2 screenPos = xy / DispatchRaysDimensions() * 2.0 - 1.0;

	// Invert Y for DirectX-style coordinates.
	screenPos.y = -screenPos.y;

	// Unproject the pixel coordinate into a ray.
	float4 world = mul(float4(screenPos, 0, 1), g_sceneCB.projectionToWorld);

	world.xyz /= world.w;
	origin = g_sceneCB.cameraPosition.xyz;
	direction = normalize(world.xyz - origin);
}

[shader("raygeneration")]
void MyRaygenShader()
{
	float3 rayDir;
	float3 origin;

	GenerateCameraRay(DispatchRaysIndex(), origin, rayDir);

	// Cast rays
	RayDesc ray;
	ray.Origin = origin;
	ray.Direction = rayDir;
	// Set TMin to a non-zero small value to avoid aliasing issues due to floating - point errors.
	// TMin should be kept small to prevent missing geometry at close contact areas.
	ray.TMin = 0.001;
	ray.TMax = 10000.0;

	RayPayload payload = { float4(0, 0, 0, 1) };
	TraceRay(Scene, RAY_FLAG_CULL_BACK_FACING_TRIANGLES, ~0, 0, 1, 0, ray, payload);
	RenderTarget[DispatchRaysIndex()] = payload.color;

}

[shader("closesthit")]
void MyClosestHitShader(inout RayPayload payload : SV_RayPayload, in MyAttributes attr : SV_IntersectionAttributes)
{
    float3 barycentrics = float3(1.0 - attr.barycentrics.x - attr.barycentrics.y, attr.barycentrics.x, attr.barycentrics.y);
    payload.color = float4(barycentrics, 1);
}

[shader("miss")]
void MyMissShader(inout RayPayload payload : SV_RayPayload)
{
	payload.color = float4(0, 0, 0, 1);
}