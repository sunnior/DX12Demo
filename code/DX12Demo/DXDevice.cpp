#include "DXDevice.h"
#include "d3dx12.h"
#include <sstream>

static void abortIfFailHr(HRESULT hr)
{
	if (hr < 0) {
		abort();
	}
}

#define ENABLE_DEBUG_LAYER
#define ABORT_IF_FAILED_HR(hr) abortIfFailHr(hr)
#define ABORT_IF_FAILED(exp)      if(!(exp)) abort()

using Microsoft::WRL::ComPtr;

DXDevice::DXDevice(WinParam winParam)
	: m_aspectRatio(winParam.width / static_cast<float>(winParam.height))
	, m_viewport{ 0.0f, 0.0f, static_cast<FLOAT>(winParam.width), static_cast<FLOAT>(winParam.height), D3D12_MIN_DEPTH, D3D12_MAX_DEPTH }
	, m_scissorRect{ 0, 0, winParam.width, winParam.height }
{
	_CreateDXGIAdapter();
	_CreateDeviceResources();
	_CreateWindow(winParam);
}

DXDevice::~DXDevice()
{

}

void DXDevice::_CreateDXGIAdapter()
{
	UINT dxgifactoryFlags = 0;

#if defined(ENABLE_DEBUG_LAYER)
	ComPtr<ID3D12Debug> debugController;

	ABORT_IF_FAILED_HR(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)));
	debugController->EnableDebugLayer();
	dxgifactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
#endif

	ABORT_IF_FAILED_HR(CreateDXGIFactory2(dxgifactoryFlags, IID_PPV_ARGS(&m_factory)));

	ComPtr<IDXGIAdapter1> adapter;

	for (UINT i = 0; m_factory->EnumAdapters1(i, &adapter) != DXGI_ERROR_NOT_FOUND; ++i)
	{
		DXGI_ADAPTER_DESC1 desc;
		adapter->GetDesc1(&desc);
		if (!(desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE))
		{
			break;
		}
	}

	m_adapter = adapter.Detach();
}

void DXDevice::_CreateDeviceResources()
{
	//UUID experimentalFeatures[] = { D3D12ExperimentalShaderModels };
	//ABORT_IF_FAILED_HR(D3D12EnableExperimentalFeatures(_countof(experimentalFeatures), experimentalFeatures, nullptr, nullptr));
	
	ABORT_IF_FAILED_HR(D3D12CreateDevice(m_adapter.Get(), D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&m_device)));

	D3D12_COMMAND_QUEUE_DESC queueDesc = {};
	queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

	ABORT_IF_FAILED_HR(m_device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_commandQueue)));

	for (UINT i = 0; i < s_backbufferCount; ++i)
	{
		ABORT_IF_FAILED_HR(m_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_commandAllocator[i])));
		m_fences[i].Init(m_device);
	}

	ABORT_IF_FAILED_HR(m_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_commandAllocator[0].Get(), nullptr, IID_PPV_ARGS(&m_commandList)));
	m_commandList->SetName(L"MainCommandList");

	ABORT_IF_FAILED_HR(m_commandList->Close());

	m_fenceEvent.Attach(CreateEvent(nullptr, FALSE, FALSE, nullptr));
	ABORT_IF_FAILED(m_fenceEvent.IsValid());
}

LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	}

	return DefWindowProc(hWnd, message, wParam, lParam);
}

void DXDevice::_CreateWindow(WinParam winParam)
{
	const char* title = "DX12Demo";
	WNDCLASSEX windowClass = { 0 };
	windowClass.cbSize = sizeof(WNDCLASSEX);
	windowClass.style = CS_HREDRAW | CS_VREDRAW;
	windowClass.lpfnWndProc = WindowProc;
	windowClass.hInstance = winParam.hInstance;
	windowClass.hCursor = LoadCursor(NULL, IDC_ARROW);
	windowClass.lpszClassName = title;
	RegisterClassEx(&windowClass);

	RECT windowRect = { 0, 0, winParam.width, winParam.height };
	AdjustWindowRect(&windowRect, WS_OVERLAPPEDWINDOW, FALSE);

	HWND hwnd = CreateWindow(
		windowClass.lpszClassName,
		title,
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		windowRect.right - windowRect.left,
		windowRect.bottom - windowRect.top,
		nullptr,		// We have no parent window.
		nullptr,		// We aren't using menus.
		winParam.hInstance,
		nullptr);

	ShowWindow(hwnd, winParam.nCmdShow);

	DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
	swapChainDesc.BufferCount = s_backbufferCount;
	swapChainDesc.Width = winParam.width;
	swapChainDesc.Height = winParam.height;
	swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	swapChainDesc.SampleDesc.Count = 1;

	ComPtr<IDXGISwapChain1> swapChain1;
	ABORT_IF_FAILED_HR(m_factory->CreateSwapChainForHwnd(m_commandQueue.Get(), hwnd, &swapChainDesc, nullptr, nullptr, &swapChain1));

	ABORT_IF_FAILED_HR(m_factory->MakeWindowAssociation(hwnd, DXGI_MWA_NO_ALT_ENTER));
	swapChain1.As(&m_swapChain);

	D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
	rtvHeapDesc.NumDescriptors = s_backbufferCount;
	rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	ABORT_IF_FAILED_HR(m_device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&m_rtvHeap)));

	UINT rtvDescSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();

	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = m_rtvHeap->GetCPUDescriptorHandleForHeapStart();
	for (UINT i = 0; i < s_backbufferCount; ++i)
	{
		ABORT_IF_FAILED_HR(m_swapChain->GetBuffer(i, IID_PPV_ARGS(&m_renderTargets[i])));
		m_device->CreateRenderTargetView(m_renderTargets[i].Get(), nullptr, rtvHandle);
		std::wostringstream wss;
		wss << L"RenderTarget_" << i;
		m_renderTargets[i]->SetName(wss.str().c_str());
		rtvHandle.ptr += rtvDescSize;
	}
}

void DXDevice::Begin()
{
	// Update the back buffer index.
	m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();
	m_fences[m_frameIndex].Wait(m_fenceEvent);

	ABORT_IF_FAILED_HR(m_commandAllocator[m_frameIndex]->Reset());
	ABORT_IF_FAILED_HR(m_commandList->Reset(m_commandAllocator[m_frameIndex].Get(), m_dummyPipelineState.Get()));

	m_commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_renderTargets[m_frameIndex].Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));
}

void DXDevice::Run()
{
	UINT rtvDescSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_rtvHeap->GetCPUDescriptorHandleForHeapStart(), m_frameIndex, rtvDescSize);

	const float clearColor[] = { 0.0f, 0.2f, 0.4f, 1.0f };
	m_commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);

}

void DXDevice::End()
{
	// Transition the render target to the state that allows it to be presented to the display.
	D3D12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(m_renderTargets[m_frameIndex].Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
	m_commandList->ResourceBarrier(1, &barrier);

	ABORT_IF_FAILED_HR(m_commandList->Close());
	ID3D12CommandList *commandLists[] = { m_commandList.Get() };
	m_commandQueue->ExecuteCommandLists(ARRAYSIZE(commandLists), commandLists);
	m_fences[m_frameIndex].Kick(m_commandQueue);

	ABORT_IF_FAILED_HR(m_swapChain->Present(1, 0));
}

void DXDevice::Fence::Init(Microsoft::WRL::ComPtr<ID3D12Device>& device)
{
	ABORT_IF_FAILED_HR(device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence)));
	m_fence->Signal(Status_Idle);
}

void DXDevice::Fence::Kick(Microsoft::WRL::ComPtr<ID3D12CommandQueue>& commandQueue)
{
	m_fence->Signal(Status_Busy);
	ABORT_IF_FAILED_HR(commandQueue->Signal(m_fence.Get(), Status_Idle));
}

void DXDevice::Fence::Wait(Microsoft::WRL::Wrappers::Event& fenceEvent)
{
	if (m_fence->GetCompletedValue() != Status_Idle)
	{
		ABORT_IF_FAILED(m_fence->SetEventOnCompletion(Status_Idle, fenceEvent.Get()));
		WaitForSingleObjectEx(fenceEvent.Get(), INFINITE, FALSE);
	}
}
