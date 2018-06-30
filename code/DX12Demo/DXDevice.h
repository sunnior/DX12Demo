#ifndef __DXDEVICE_HEADER_H__
#define __DXDEVICE_HEADER_H__

#include <Windows.h>
#include <d3d12.h>
#include <wrl.h>
#include <dxgi1_6.h>

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
		enum Status {
			Status_Idle,
			Status_Busy,
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
	D3D12_VIEWPORT m_viewport;
	D3D12_RECT m_scissorRect;

private:
	Microsoft::WRL::ComPtr<IDXGIFactory2> m_factory;
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

};

#endif // _DEBUG

