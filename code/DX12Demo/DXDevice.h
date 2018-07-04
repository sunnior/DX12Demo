#ifndef __DXDEVICE_HEADER_H__
#define __DXDEVICE_HEADER_H__

#include <Windows.h>
#include <d3d12.h>
#include <wrl.h>
#include <dxgi1_6.h>
#include "d3dx12.h"
#include "d3d12_1.h"
#include "D3D12RaytracingFallback.h"
#include <DirectXMath.h>
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

	void Begin();
	void Run();
	void End();

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

	void _CreateAccelerationStructures();
	void _CreateGeometry();
	void _CreateBottomLevelAS(ID3D12Resource** ppInstanceDescs);
	void _CreateTopLevelAS(ID3D12Resource* pInstanceDescs);

	void _CreateRootSignatures();
	void _CreateRaytracingPSO();
	void _CreateShaderResources();
	void _CreateShaderTables();

	void _InitMatrix();
	void _UpdateMatrix();
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
		BottomLevelWrapperPointer,
		TopLevelWrapperPointer,
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
		Viewport rayGenViewport;
		Viewport rayGenStencil;
	};

private:
	Microsoft::WRL::ComPtr<ID3D12RaytracingFallbackDevice> m_raytracingDevice;
	Microsoft::WRL::ComPtr<ID3D12RaytracingFallbackCommandList> m_raytracingCommandList;

	Microsoft::WRL::ComPtr<ID3D12Resource> m_raytracingOutput;

	Microsoft::WRL::ComPtr<ID3D12Resource> m_vertexBuffer;
	Microsoft::WRL::ComPtr<ID3D12Resource> m_indexBuffer;

	Microsoft::WRL::ComPtr<ID3D12Resource> m_bottomLevelAccelerationStructure;
	Microsoft::WRL::ComPtr<ID3D12Resource> m_topLevelAccelerationStructure;

	Microsoft::WRL::ComPtr<ID3D12RootSignature> m_raytracingGlobalRootSignature;
	Microsoft::WRL::ComPtr<ID3D12RootSignature> m_raytracingLocalRootSignature;

	Microsoft::WRL::ComPtr<ID3D12RaytracingFallbackStateObject> m_stateObject;

	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_raytracingDescriptorHeap;
	UINT m_descriptorSize;

	D3D12_GPU_DESCRIPTOR_HANDLE m_raytracingOutputResourceUAVGpuDescriptor;

	WRAPPED_GPU_POINTER m_topLevelAccelerationStructurePointer;

	RayGenConstantBuffer m_rayGenCB;

	Microsoft::WRL::ComPtr<ID3D12Resource> m_missShaderTable;
	Microsoft::WRL::ComPtr<ID3D12Resource> m_hitGroupShaderTable;
	Microsoft::WRL::ComPtr<ID3D12Resource> m_rayGenShaderTable;

private:
	typedef UINT16 Index_t;
	typedef DirectX::XMFLOAT3 Vertex_t[2];
	DirectX::XMFLOAT4 m_eye;
	DirectX::XMFLOAT4 m_at;
	DirectX::XMFLOAT4 m_up;

	SceneConstantBuffer m_sceneCB;

};

#endif // _DEBUG

