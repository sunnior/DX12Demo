RaytracingAccelerationStructure Scene : register(t0, space0);
RWTexture2D<float4> RenderTarget : register(u0);

struct Viewport
{
    float2 topLeft;
    float2 bottomRight;
};

struct RayGenConstantBuffer 
{
	float4 missColor;
};

ConstantBuffer<RayGenConstantBuffer> g_raygenBuffer: register(b0);

typedef BuiltInTriangleIntersectionAttributes MyAttributes;
struct RayPayload
{
	float4 color;
};

bool IsInside(float p, float2 range)
{
    return p.x > range.x && p.x < range.y;
}

bool IsInsideViewport(float2 p, Viewport viewport)
{
    return (p.x >= viewport.topLeft.x && p.x <= viewport.bottomRight.x)
        && (p.y >= viewport.topLeft.y && p.y <= viewport.bottomRight.y);
}

[shader("raygeneration")]
void MyRaygenShader()
{
	float2 lerpValues = (float2)DispatchRaysIndex() / DispatchRaysDimensions();

	// Orthographic projection since we're raytracing in screen space
	float3 rayDir = float3(0.0, 0.0, 1);
	float3 origin = float3(
		lerp(-1.0f, 1.0f, lerpValues.x),
		lerp(-1.0f, 1.0f, lerpValues.y),
		0.0f);


	// Cast rays
	RayDesc myRay = { origin,
		0.001f,
		rayDir,
		10000.0f };
	RayPayload payload = { float4(0, 0, 0, 1) };
	TraceRay(Scene, RAY_FLAG_CULL_BACK_FACING_TRIANGLES, ~0, 0, 1, 0, myRay, payload);
	RenderTarget[DispatchRaysIndex()] = payload.color;
	//RenderTarget[DispatchRaysIndex()] = g_raygenBuffer.missColor;

}

[shader("closesthit")]
void MyClosestHitShader(inout RayPayload payload : SV_RayPayload, in MyAttributes attr : SV_IntersectionAttributes)
{
    float3 barycentrics = float3(1.0 - attr.barycentrics.x - attr.barycentrics.y, attr.barycentrics.x, attr.barycentrics.y);
    payload.color = float4(barycentrics, 1);
	payload.color = g_raygenBuffer.missColor;
}

[shader("miss")]
void MyMissShader(inout RayPayload payload : SV_RayPayload)
{
	payload.color = float4(0, 0, 0, 1);
}