#ifndef __DXDEVICE_HEADER_H__
#define __DXDEVICE_HEADER_H__

#include <vector>
#include <Windows.h>
#include <d3d12.h>
#include <wrl.h>
#include <dxgi1_6.h>
#include "d3dx12.h"
#include "d3d12_1.h"
#include "D3D12RaytracingFallback.h"
#include <DirectXMath.h>
#define LANGUAGE_CPP
#include "RaytracingHlslCompat.h"

class DXDevice
{
public:
	struct WinParam {
		HINSTANCE hInstance;
		int nCmdShow;
		int width;
		int height;
	};

private:
	struct Fence {
		//due to SetEventOnCompletion, Status_Idle must be bigger than Status_Busy.
		enum Status {
			Status_Busy = 0,
			Status_Idle = 1,
		};
		void Init(Microsoft::WRL::ComPtr<ID3D12Device>& device);

		void Kick(Microsoft::WRL::ComPtr<ID3D12CommandQueue>& commandQueue);
		void Wait(Microsoft::WRL::Wrappers::Event& fenceEvent);

		Microsoft::WRL::ComPtr<ID3D12Fence> m_fence;
	};

public:
	DXDevice(WinParam winParam);
	~DXDevice();

	DXDevice(const DXDevice&) = delete;


	void Begin();
	void Run();
	void End();

public:
	void AddInstance(const D3D12_RAYTRACING_FALLBACK_INSTANCE_DESC& instanceDesc);

	ID3D12Resource* CreateUploadBuffer(const void *pData, UINT64 datasize, const wchar_t* resourceName = L"NameEmpty");

	void CreateBottomLevelAS(
		D3D12_GPU_VIRTUAL_ADDRESS vertexAddress, UINT vertexCount, UINT64 vertexStride,
		D3D12_GPU_VIRTUAL_ADDRESS indexAddress, UINT indexCount,
		D3D12_RAYTRACING_FALLBACK_INSTANCE_DESC& ppInstanceDesc, const DirectX::XMFLOAT4X3& transform,
		ID3D12Resource** bottomLevelAccelerationStructure);

	void SetCamera(DirectX::XMFLOAT4X4 projectionToWorld, DirectX::XMFLOAT4 cameraPosition);

private:
	void _CreateDXGIAdapter();
	void _CreateDeviceResources();
	void _CreateWindow(WinParam winParam);

private:
	static const int s_backbufferCount = 3;

private:
	float m_aspectRatio;
	float m_winWidth;
	float m_winHeight;
	D3D12_VIEWPORT m_viewport;
	D3D12_RECT m_scissorRect;
	DXGI_FORMAT m_backbufferFormat;

private:
	Microsoft::WRL::ComPtr<IDXGIFactory4> m_factory;
	Microsoft::WRL::ComPtr<IDXGIAdapter1> m_adapter;
	Microsoft::WRL::ComPtr<IDXGISwapChain3> m_swapChain;

private:
	Microsoft::WRL::ComPtr<ID3D12Device> m_device;
	Microsoft::WRL::ComPtr<ID3D12CommandQueue> m_commandQueue;
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> m_commandAllocator[s_backbufferCount];
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> m_commandList;
	Fence m_fences[s_backbufferCount];

	Microsoft::WRL::ComPtr<ID3D12PipelineState> m_dummyPipelineState;


	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_rtvHeap;
	Microsoft::WRL::ComPtr<ID3D12Resource> m_renderTargets[s_backbufferCount];

	Microsoft::WRL::Wrappers::Event m_fenceEvent;

	UINT m_frameIndex;

private:
	void _EnableRaytracing();
	void _CreateRaytracingDevice();
	void _CreateRaytracingDescriptorHeaps();

	void _CreateTopLevelAS();

	void _CreateRootSignatures();
	void _CreateRaytracingPSO();
	void _CreateShaderResources();
	void _CreateShaderTables();

private:
	enum class GlobalRootSignatureParams {
		OutputViewSlot = 0,
		AccelerationStructureSlot,
		Count
	};

	enum class LocalRootSignatureParams {
		ViewportConstantSlot = 0,
		Count
	};

	enum class RaytracingDescriptorHeapSlot {
		OutputTexture,
		TopLevelWrapperPointer,
		BottomLevelWrapperPointer_Begin = 2,
		BottomLevelWrapperPointer_End = 5,
		Count
	};

	struct Viewport
	{
		float Left;
		float Top;
		float Right;
		float Bottom;
	};

	struct RayGenConstantBuffer
	{
		//Viewport rayGenViewport;
		DirectX::XMFLOAT4 missColor;
	};

private:
	Microsoft::WRL::ComPtr<ID3D12RaytracingFallbackDevice> m_raytracingDevice;
	Microsoft::WRL::ComPtr<ID3D12RaytracingFallbackCommandList> m_raytracingCommandList;

	Microsoft::WRL::ComPtr<ID3D12Resource> m_raytracingOutput;

	Microsoft::WRL::ComPtr<ID3D12Resource> m_topLevelAccelerationStructure;

	Microsoft::WRL::ComPtr<ID3D12RootSignature> m_raytracingGlobalRootSignature;
	Microsoft::WRL::ComPtr<ID3D12RootSignature> m_raytracingLocalRootSignature;
	Microsoft::WRL::ComPtr<ID3D12RootSignature> m_raytracingLocalRootSignatureEmpty;

	Microsoft::WRL::ComPtr<ID3D12RaytracingFallbackStateObject> m_stateObject;

	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_raytracingDescriptorHeap;
	UINT m_descriptorSize;

	D3D12_GPU_DESCRIPTOR_HANDLE m_raytracingOutputResourceUAVGpuDescriptor;

	WRAPPED_GPU_POINTER m_topLevelAccelerationStructurePointer;

	std::vector<D3D12_RAYTRACING_FALLBACK_INSTANCE_DESC> m_instanceDescs;
	Microsoft::WRL::ComPtr<ID3D12Resource> m_instanceDescResource;
	int m_bottomHeapIndex{ 0 };

	Microsoft::WRL::ComPtr<ID3D12Resource> m_missShaderTable;
	Microsoft::WRL::ComPtr<ID3D12Resource> m_hitGroupShaderTable;
	Microsoft::WRL::ComPtr<ID3D12Resource> m_rayGenShaderTable;

private:
	SceneConstantBuffer m_sceneCB;

};

#endif // _DEBUG

