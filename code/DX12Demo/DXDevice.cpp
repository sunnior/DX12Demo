#include "DXDevice.h"
#include <sstream>
#include <winerror.h>
#include "D3D12RaytracingPrototypeHelpers.hpp"
#include "DXDevice_Helper.h"

#define ENABLE_DEBUG_LAYER
#define USE_NON_NULL_LOCAL_ROOT_SIG

static const wchar_t* s_hitGroupName = L"MyHitGroup";
static const wchar_t* s_raygenShaderName = L"MyRaygenShader";
static const wchar_t* s_closestHitShaderName = L"MyClosestHitShader";
static const wchar_t* s_missShaderName = L"MyMissShader";

using Microsoft::WRL::ComPtr;
using namespace DirectX;

DXDevice::DXDevice(WinParam winParam)
	: m_aspectRatio{ winParam.width / static_cast<float>(winParam.height) }
	, m_viewport{ 0.0f, 0.0f, static_cast<FLOAT>(winParam.width), static_cast<FLOAT>(winParam.height), D3D12_MIN_DEPTH, D3D12_MAX_DEPTH }
	, m_scissorRect{ 0, 0, winParam.width, winParam.height }
	, m_backbufferFormat{ DXGI_FORMAT_R8G8B8A8_UNORM }
	, m_winHeight{ static_cast<FLOAT>(winParam.height) }
	, m_winWidth{ static_cast<FLOAT>(winParam.width) }
{
	_EnableRaytracing();

	_CreateDXGIAdapter();

	_CreateDeviceResources();
	_CreateWindow(winParam);

	_CreateRaytracingDevice();
	
	_CreateRootSignatures();
	_CreateRaytracingPSO();

	_CreateRaytracingDescriptorHeaps();


	_CreateShaderResources();
	_CreateShaderTables();
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
	swapChainDesc.Format = m_backbufferFormat;
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
	if (m_topLevelAccelerationStructure == nullptr && m_instanceDescs.size() > 0)
	{
		_CreateTopLevelAS();
	}

	// Update the back buffer index.
	m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();
	m_fences[m_frameIndex].Wait(m_fenceEvent);

	ABORT_IF_FAILED_HR(m_commandAllocator[m_frameIndex]->Reset());
	ABORT_IF_FAILED_HR(m_commandList->Reset(m_commandAllocator[m_frameIndex].Get(), m_dummyPipelineState.Get()));

	m_commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_renderTargets[m_frameIndex].Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));
}

void DXDevice::Run()
{
	uint8_t* mappedData;
	// We don't unmap this until the app closes. Keeping buffer mapped for the lifetime of the resource is okay.
	CD3DX12_RANGE readRange(0, 0);        // We do not intend to read from this resource on the CPU.
	ABORT_IF_FAILED_HR(m_rayGenShaderTable->Map(0, &readRange, reinterpret_cast<void**>(&mappedData)));

	// Get shader identifiers

	void* rayGenShaderIdentifier = m_stateObject->GetShaderIdentifier(s_raygenShaderName);
	UINT shaderIdentifierSize = m_raytracingDevice->GetShaderIdentifierSize();
	struct RootArguments {
		SceneConstantBuffer cb;
	} rootArguments;
	rootArguments.cb.cameraPosition = m_sceneCB.cameraPosition;
	rootArguments.cb.projectionToWorld = m_sceneCB.projectionToWorld;
	UINT rootArgumentsSize = sizeof(rootArguments);

	// Shader record = {{ Shader ID }, { RootArguments }}
	UINT shaderRecordSize = shaderIdentifierSize + rootArgumentsSize;

	memcpy(mappedData, rayGenShaderIdentifier, shaderIdentifierSize);
	memcpy(mappedData + shaderIdentifierSize, &rootArguments, rootArgumentsSize);


/*
	UINT rtvDescSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_rtvHeap->GetCPUDescriptorHandleForHeapStart(), m_frameIndex, rtvDescSize);

	const float clearColor[] = { 0.0f, 0.2f, 0.4f, 1.0f };
	m_commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);*/

	m_commandList->SetComputeRootSignature(m_raytracingGlobalRootSignature.Get());

	m_raytracingCommandList->SetDescriptorHeaps(1, m_raytracingDescriptorHeap.GetAddressOf());
	m_commandList->SetComputeRootDescriptorTable(static_cast<UINT>(GlobalRootSignatureParams::OutputViewSlot), m_raytracingOutputResourceUAVGpuDescriptor);
	m_raytracingCommandList->SetTopLevelAccelerationStructure(static_cast<UINT>(GlobalRootSignatureParams::AccelerationStructureSlot), m_topLevelAccelerationStructurePointer);

	D3D12_FALLBACK_DISPATCH_RAYS_DESC dispatchDesc{};
	dispatchDesc.HitGroupTable.StartAddress = m_hitGroupShaderTable->GetGPUVirtualAddress();
	dispatchDesc.HitGroupTable.SizeInBytes = m_hitGroupShaderTable->GetDesc().Width;
	dispatchDesc.HitGroupTable.StrideInBytes = dispatchDesc.HitGroupTable.SizeInBytes;
	dispatchDesc.MissShaderTable.StartAddress = m_missShaderTable->GetGPUVirtualAddress();
	dispatchDesc.MissShaderTable.SizeInBytes = m_missShaderTable->GetDesc().Width;
	dispatchDesc.MissShaderTable.StrideInBytes = dispatchDesc.MissShaderTable.SizeInBytes; //TODO check this
	dispatchDesc.RayGenerationShaderRecord.StartAddress = m_rayGenShaderTable->GetGPUVirtualAddress();
	dispatchDesc.RayGenerationShaderRecord.SizeInBytes = m_rayGenShaderTable->GetDesc().Width;
	dispatchDesc.Width = static_cast<UINT>(m_winWidth);
	dispatchDesc.Height = static_cast<UINT>(m_winHeight);
	m_raytracingCommandList->DispatchRays(m_stateObject.Get(), &dispatchDesc);



	D3D12_RESOURCE_BARRIER preCopyBarriers[2];
	preCopyBarriers[0] = CD3DX12_RESOURCE_BARRIER::Transition(m_renderTargets[m_frameIndex].Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_COPY_DEST);
	preCopyBarriers[1] = CD3DX12_RESOURCE_BARRIER::Transition(m_raytracingOutput.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_SOURCE);
	m_commandList->ResourceBarrier(ARRAYSIZE(preCopyBarriers), preCopyBarriers);

	m_commandList->CopyResource(m_renderTargets[m_frameIndex].Get(), m_raytracingOutput.Get());

	D3D12_RESOURCE_BARRIER postCopyBarriers[2];
	postCopyBarriers[0] = CD3DX12_RESOURCE_BARRIER::Transition(m_renderTargets[m_frameIndex].Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_RENDER_TARGET);
	postCopyBarriers[1] = CD3DX12_RESOURCE_BARRIER::Transition(m_raytracingOutput.Get(), D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

	m_commandList->ResourceBarrier(ARRAYSIZE(postCopyBarriers), postCopyBarriers);
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

void DXDevice::AddInstance(const D3D12_RAYTRACING_FALLBACK_INSTANCE_DESC& instanceDesc)
{
	m_instanceDescs.push_back(instanceDesc);
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
		ABORT_IF_FAILED_HR(m_fence->SetEventOnCompletion(Status_Idle, fenceEvent.Get()));
		WaitForSingleObjectEx(fenceEvent.Get(), INFINITE, FALSE);
	}
}

void DXDevice::_EnableRaytracing()
{
	//UUID experimentalFeaturesSMandDXR[] = { D3D12ExperimentalShaderModels, D3D12RaytracingPrototype };
	UUID experimentalFeaturesSM[] = { D3D12ExperimentalShaderModels };

	// D3D12CreateDevice needs to pass after enabling experimental features to successfully declare support.
	//if (FAILED(D3D12EnableExperimentalFeatures(ARRAYSIZE(experimentalFeaturesSMandDXR), experimentalFeaturesSMandDXR, nullptr, nullptr))
	ABORT_IF_FAILED_HR(D3D12EnableExperimentalFeatures(ARRAYSIZE(experimentalFeaturesSM), experimentalFeaturesSM, nullptr, nullptr));
}

void DXDevice::_CreateRaytracingDevice()
{
	ABORT_IF_FAILED_HR(D3D12CreateRaytracingFallbackDevice(m_device.Get(), CreateRaytracingFallbackDeviceFlags::None, 0, IID_PPV_ARGS(&m_raytracingDevice)));
	m_raytracingDevice->QueryRaytracingCommandList(m_commandList.Get(), IID_PPV_ARGS(&m_raytracingCommandList));

}

void DXDevice::_CreateRaytracingDescriptorHeaps()
{
	D3D12_DESCRIPTOR_HEAP_DESC descriptorHeapDesc = {};
	// Allocate heap for plenty of descriptors
	descriptorHeapDesc.NumDescriptors = static_cast<int>(RaytracingDescriptorHeapSlot::Count);
	descriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	descriptorHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	descriptorHeapDesc.NodeMask = 0;
	m_device->CreateDescriptorHeap(&descriptorHeapDesc, IID_PPV_ARGS(&m_raytracingDescriptorHeap));

	m_descriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
}

void DXDevice::CreateBottomLevelAS(
	D3D12_GPU_VIRTUAL_ADDRESS vertexAddress, UINT vertexCount, UINT64 vertexStride, 
	D3D12_GPU_VIRTUAL_ADDRESS indexAddress, UINT indexCount,
	D3D12_RAYTRACING_FALLBACK_INSTANCE_DESC& InstanceDesc, const XMFLOAT4X3& transform,
	ID3D12Resource** bottomLevelAccelerationStructure)
{
	D3D12_RAYTRACING_GEOMETRY_DESC geometryDesc = {};
	geometryDesc.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
	geometryDesc.Triangles.IndexBuffer = indexAddress;
	geometryDesc.Triangles.IndexCount = indexCount;
	geometryDesc.Triangles.IndexFormat = DXGI_FORMAT_R16_UINT;
	geometryDesc.Triangles.Transform = 0;
	geometryDesc.Triangles.VertexFormat = DXGI_FORMAT_R32G32B32_FLOAT;
	geometryDesc.Triangles.VertexCount = vertexCount;
	geometryDesc.Triangles.VertexBuffer.StartAddress = vertexAddress; 
	geometryDesc.Triangles.VertexBuffer.StrideInBytes = vertexStride;
	//geometryDesc.Flags = D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE;

	// Get required sizes for an acceleration structure.
	D3D12_GET_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO_DESC prebuildInfoDesc{};
	prebuildInfoDesc.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
	prebuildInfoDesc.NumDescs = 1;
	prebuildInfoDesc.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
	prebuildInfoDesc.pGeometryDescs = &geometryDesc;

	D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO bottomLevelPrebuildInfo{};
	m_raytracingDevice->GetRaytracingAccelerationStructurePrebuildInfo(&prebuildInfoDesc, &bottomLevelPrebuildInfo);

	ABORT_IF_FAILED(bottomLevelPrebuildInfo.ResultDataMaxSizeInBytes > 0);

	D3D12_RESOURCE_STATES initialResourceState = m_raytracingDevice->GetAccelerationStructureResourceState();
	_CreateUAVBuffer(m_device.Get(), bottomLevelAccelerationStructure, bottomLevelPrebuildInfo.ResultDataMaxSizeInBytes, initialResourceState, L"BottomLevelAccelerationStructure");

	ComPtr<ID3D12Resource> scratchResource;
	_CreateUAVBuffer(m_device.Get(), &scratchResource, bottomLevelPrebuildInfo.ScratchDataSizeInBytes, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, L"ScratchResource");

	// Create an instance desc for the bottom level acceleration structure.

	memcpy(InstanceDesc.Transform, &transform, sizeof(transform));
	InstanceDesc.InstanceMask = 1;
	UINT numBufferElements = static_cast<UINT>(bottomLevelPrebuildInfo.ResultDataMaxSizeInBytes) / sizeof(UINT32);
	InstanceDesc.AccelerationStructure = _CreateWrappedPointer(m_device.Get(), m_raytracingDevice.Get(), m_raytracingDescriptorHeap.Get(), static_cast<UINT>(RaytracingDescriptorHeapSlot::BottomLevelWrapperPointer_Begin) + m_bottomHeapIndex, m_descriptorSize, *bottomLevelAccelerationStructure, numBufferElements);
	//InstanceDesc = CreateUploadBuffer(&instanceDesc, sizeof(instanceDesc), L"InstanceDescs");
	++m_bottomHeapIndex;

	// Bottom Level Acceleration Structure desc
	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC bottomLevelBuildDesc{};
	{
		bottomLevelBuildDesc.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
		bottomLevelBuildDesc.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;
		bottomLevelBuildDesc.ScratchAccelerationStructureData = { scratchResource->GetGPUVirtualAddress(), scratchResource->GetDesc().Width };
		bottomLevelBuildDesc.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
		bottomLevelBuildDesc.DestAccelerationStructureData = { (*bottomLevelAccelerationStructure)->GetGPUVirtualAddress(), bottomLevelPrebuildInfo.ResultDataMaxSizeInBytes };
		bottomLevelBuildDesc.NumDescs = 1;
		bottomLevelBuildDesc.pGeometryDescs = &geometryDesc;
	}

	ABORT_IF_FAILED_HR(m_commandAllocator[0]->Reset());
	ABORT_IF_FAILED_HR(m_commandList->Reset(m_commandAllocator[0].Get(), m_dummyPipelineState.Get()));
	m_raytracingCommandList->BuildRaytracingAccelerationStructure(&bottomLevelBuildDesc);
	m_commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::UAV(*bottomLevelAccelerationStructure));
	m_commandList->Close();
	ID3D12CommandList *commandLists[] = { m_commandList.Get() };
	m_commandQueue->ExecuteCommandLists(ARRAYSIZE(commandLists), commandLists);

	m_fences[0].Kick(m_commandQueue);
	m_fences[0].Wait(m_fenceEvent);
}

void DXDevice::SetCamera(DirectX::XMFLOAT4X4 projectionToWorld, DirectX::XMFLOAT4 cameraPosition)
{
	m_sceneCB.cameraPosition = cameraPosition;
	m_sceneCB.projectionToWorld = projectionToWorld;
}

void DXDevice::_CreateTopLevelAS()
{


	m_instanceDescResource = CreateUploadBuffer(m_instanceDescs.data(), m_instanceDescs.size() * sizeof(m_instanceDescs[0]), L"InstanceDescs");

	// Get required sizes for an acceleration structure.
	D3D12_GET_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO_DESC prebuildInfoDesc{};
	prebuildInfoDesc.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
	prebuildInfoDesc.NumDescs = m_instanceDescs.size();
	prebuildInfoDesc.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
	prebuildInfoDesc.pGeometryDescs = nullptr;

	D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO topLevelPrebuildInfo{};

	m_raytracingDevice->GetRaytracingAccelerationStructurePrebuildInfo(&prebuildInfoDesc, &topLevelPrebuildInfo);
	
	ABORT_IF_FAILED(topLevelPrebuildInfo.ResultDataMaxSizeInBytes > 0);

	ComPtr<ID3D12Resource> scratchResource;
	_CreateUAVBuffer(m_device.Get(), &scratchResource, topLevelPrebuildInfo.ScratchDataSizeInBytes, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, L"ScratchResource");
	D3D12_RESOURCE_STATES initialResourceState = m_raytracingDevice->GetAccelerationStructureResourceState();

	_CreateUAVBuffer(m_device.Get(), &m_topLevelAccelerationStructure, topLevelPrebuildInfo.ResultDataMaxSizeInBytes, initialResourceState, L"TopLevelAccelerationStructure");

	UINT numBufferElements = static_cast<UINT>(topLevelPrebuildInfo.ResultDataMaxSizeInBytes) / sizeof(UINT32);
	m_topLevelAccelerationStructurePointer = _CreateWrappedPointer(m_device.Get(), m_raytracingDevice.Get(), m_raytracingDescriptorHeap.Get(), static_cast<UINT>(RaytracingDescriptorHeapSlot::TopLevelWrapperPointer), m_descriptorSize, m_topLevelAccelerationStructure.Get(), numBufferElements);

	// Top Level Acceleration Structure desc
	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC topLevelBuildDesc{};
	{
		topLevelBuildDesc.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
		topLevelBuildDesc.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;

		topLevelBuildDesc.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
		topLevelBuildDesc.DestAccelerationStructureData = { m_topLevelAccelerationStructure->GetGPUVirtualAddress(), topLevelPrebuildInfo.ResultDataMaxSizeInBytes };
		topLevelBuildDesc.NumDescs = m_instanceDescs.size();
		topLevelBuildDesc.pGeometryDescs = nullptr;
		topLevelBuildDesc.InstanceDescs = m_instanceDescResource->GetGPUVirtualAddress();
		topLevelBuildDesc.ScratchAccelerationStructureData = { scratchResource->GetGPUVirtualAddress(), scratchResource->GetDesc().Width };
	}

	ABORT_IF_FAILED_HR(m_commandAllocator[0]->Reset());
	ABORT_IF_FAILED_HR(m_commandList->Reset(m_commandAllocator[0].Get(), m_dummyPipelineState.Get()));
	ID3D12DescriptorHeap *pDescriptorHeaps[] = { m_raytracingDescriptorHeap.Get() };
	m_raytracingCommandList->SetDescriptorHeaps(ARRAYSIZE(pDescriptorHeaps), pDescriptorHeaps);
	m_raytracingCommandList->BuildRaytracingAccelerationStructure(&topLevelBuildDesc);
	m_commandList->Close();
	ID3D12CommandList *commandLists[] = { m_commandList.Get() };
	m_commandQueue->ExecuteCommandLists(ARRAYSIZE(commandLists), commandLists);

	m_fences[0].Kick(m_commandQueue);
	m_fences[0].Wait(m_fenceEvent);
}

void DXDevice::_CreateRootSignatures()
{
	// Global Root Signature
	{
		CD3DX12_DESCRIPTOR_RANGE UAVDescriptor;
		UAVDescriptor.Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0);
		CD3DX12_ROOT_PARAMETER rootParameters[static_cast<int>(GlobalRootSignatureParams::Count)];
		rootParameters[static_cast<int>(GlobalRootSignatureParams::OutputViewSlot)].InitAsDescriptorTable(1, &UAVDescriptor);
		rootParameters[static_cast<int>(GlobalRootSignatureParams::AccelerationStructureSlot)].InitAsShaderResourceView(0);
		CD3DX12_ROOT_SIGNATURE_DESC globalRootSignatureDesc(ARRAYSIZE(rootParameters), rootParameters);
		_SerializeAndCreateRaytracingRootSignature(m_raytracingDevice.Get(), globalRootSignatureDesc, &m_raytracingGlobalRootSignature);
	}

	// Local Root Signature
	{
		CD3DX12_ROOT_PARAMETER rootParameters[static_cast<int>(LocalRootSignatureParams::Count)];
		UINT num32BitValues = (sizeof(m_sceneCB) - 1) / sizeof(UINT32) + 1;
		rootParameters[static_cast<int>(LocalRootSignatureParams::ViewportConstantSlot)].InitAsConstants(num32BitValues, 0);
		CD3DX12_ROOT_SIGNATURE_DESC localRootSignatureDesc(ARRAYSIZE(rootParameters), rootParameters);
		localRootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE;
		_SerializeAndCreateRaytracingRootSignature(m_raytracingDevice.Get(), localRootSignatureDesc, &m_raytracingLocalRootSignature);
	}

#ifdef USE_NON_NULL_LOCAL_ROOT_SIG
	// Empty local root signature
	{
		CD3DX12_ROOT_SIGNATURE_DESC localRootSignatureDesc(D3D12_DEFAULT);
		localRootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE;
		_SerializeAndCreateRaytracingRootSignature(m_raytracingDevice.Get(), localRootSignatureDesc, &m_raytracingLocalRootSignatureEmpty);
	}
#endif
}

void DXDevice::_CreateRaytracingPSO()
{
	CD3D12_STATE_OBJECT_DESC dxrPipeline{ D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE };
	auto lib = dxrPipeline.CreateSubobject<CD3D12_DXIL_LIBRARY_SUBOBJECT>();
	D3D12_SHADER_BYTECODE libdxil{};
	
	_LoadBinaryResource("Raytracing", libdxil.pShaderBytecode, libdxil.BytecodeLength);

	lib->SetDXILLibrary(&libdxil);
	lib->DefineExport(s_raygenShaderName);
	lib->DefineExport(s_closestHitShaderName);
	lib->DefineExport(s_missShaderName);

	auto hitGroup = dxrPipeline.CreateSubobject<CD3D12_HIT_GROUP_SUBOBJECT>();
	hitGroup->SetClosestHitShaderImport(s_closestHitShaderName);
	hitGroup->SetHitGroupExport(s_hitGroupName);

	/*
	auto shaderConfig = dxrPipeline.CreateSubobject<CD3D12_RAYTRACING_SHADER_CONFIG_SUBOBJECT>();
	//payload and attributes' size in bytes
	UINT payloadSize = sizeof(XMFLOAT4); // float4 pixelColor
	UINT attrSize = sizeof(XMFLOAT2); // float2 barycentrics
	shaderConfig->Config(payloadSize, attrSize);

	auto shaderConfigAssociation = dxrPipeline.CreateSubobject<CD3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION_SUBOBJECT>();
	shaderConfigAssociation->SetSubobjectToAssociate(*shaderConfig);
	shaderConfigAssociation->AddExport(s_raygenShaderName);
	shaderConfigAssociation->AddExport(s_missShaderName);
	shaderConfigAssociation->AddExport(s_hitGroupName);*/

	auto localRootSignature = dxrPipeline.CreateSubobject<CD3D12_LOCAL_ROOT_SIGNATURE_SUBOBJECT>();
	localRootSignature->SetRootSignature(m_raytracingLocalRootSignature.Get());

	auto rootSignatureAssociation = dxrPipeline.CreateSubobject<CD3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION_SUBOBJECT>();
	rootSignatureAssociation->SetSubobjectToAssociate(*localRootSignature);
	rootSignatureAssociation->AddExport(s_raygenShaderName);
	//rootSignatureAssociation->AddExport(s_missShaderName);
	//rootSignatureAssociation->AddExport(s_hitGroupName);

#ifdef USE_NON_NULL_LOCAL_ROOT_SIG 
	// Empty local root signature to be used in a ray gen and a miss shader.
	{
		auto localRootSignature = dxrPipeline.CreateSubobject<CD3D12_LOCAL_ROOT_SIGNATURE_SUBOBJECT>();
		localRootSignature->SetRootSignature(m_raytracingLocalRootSignatureEmpty.Get());
		// Shader association
		auto rootSignatureAssociation = dxrPipeline.CreateSubobject<CD3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION_SUBOBJECT>();
		rootSignatureAssociation->SetSubobjectToAssociate(*localRootSignature);
		//rootSignatureAssociation->AddExport(s_raygenShaderName);
		rootSignatureAssociation->AddExport(s_missShaderName);
		rootSignatureAssociation->AddExport(s_hitGroupName);
	}
#endif

	auto globalRootSignature = dxrPipeline.CreateSubobject<CD3D12_ROOT_SIGNATURE_SUBOBJECT>();
	globalRootSignature->SetRootSignature(m_raytracingGlobalRootSignature.Get());

	auto pipelineConfig = dxrPipeline.CreateSubobject<CD3D12_RAYTRACING_PIPELINE_CONFIG_SUBOBJECT>();
	pipelineConfig->Config(2);

	//_PrintStateObjectDesc(dxrPipeline);

	ABORT_IF_FAILED_HR(m_raytracingDevice->CreateStateObject(dxrPipeline, IID_PPV_ARGS(&m_stateObject)));

}

void DXDevice::_CreateShaderResources()
{
	// Create the output resource. The dimensions and format should match the swap-chain.
	auto uavDesc = CD3DX12_RESOURCE_DESC::Tex2D(m_backbufferFormat, static_cast<UINT64>(m_winWidth), static_cast<UINT16>(m_winHeight), 1, 1, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);

	auto defaultHeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
	ABORT_IF_FAILED_HR(m_device->CreateCommittedResource(
		&defaultHeapProperties, D3D12_HEAP_FLAG_NONE, &uavDesc, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, nullptr, IID_PPV_ARGS(&m_raytracingOutput)));
	m_raytracingOutput->SetName(L"OutputBuffer");

	//TODO check use different heap.
	D3D12_CPU_DESCRIPTOR_HANDLE uavDescriptorHandle;
	uavDescriptorHandle.ptr = CD3DX12_CPU_DESCRIPTOR_HANDLE(m_raytracingDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), static_cast<INT>(RaytracingDescriptorHeapSlot::OutputTexture), m_descriptorSize).ptr;
	D3D12_UNORDERED_ACCESS_VIEW_DESC UAVDesc = {};
	UAVDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
	m_device->CreateUnorderedAccessView(m_raytracingOutput.Get(), nullptr, &UAVDesc, uavDescriptorHandle);
	m_raytracingOutputResourceUAVGpuDescriptor = m_raytracingDescriptorHeap->GetGPUDescriptorHandleForHeapStart();
}

void DXDevice::_CreateShaderTables()
{
	// Get shader identifiers
	void* rayGenShaderIdentifier = m_stateObject->GetShaderIdentifier(s_raygenShaderName);
	void* missShaderIdentifier = m_stateObject->GetShaderIdentifier(s_missShaderName);
	void* hitGroupShaderIdentifier = m_stateObject->GetShaderIdentifier(s_hitGroupName);
	UINT shaderIdentifierSize = m_raytracingDevice->GetShaderIdentifierSize();

	// Initialize shader records
	//assert(LocalRootSignatureParams::ViewportConstantSlot == 0 && LocalRootSignatureParams::Count == 1);

	//m_rayGenCB.rayGenViewport = { -1.0f, -1.0f, 1.0f, 1.0f };
	//m_rayGenCB.missColor = { 1.0f, 0.0f, 0.0f, 1.0f };
	struct RootArguments {
		SceneConstantBuffer cb;
	} rootArguments;
	rootArguments.cb = m_sceneCB;
	UINT rootArgumentsSize = sizeof(rootArguments);

	// Shader record = {{ Shader ID }, { RootArguments }}
	UINT shaderRecordSize = shaderIdentifierSize + rootArgumentsSize;

	ShaderRecord rayGenShaderRecord(rayGenShaderIdentifier, shaderIdentifierSize, &rootArguments, rootArgumentsSize);
	rayGenShaderRecord.AllocateAsUploadBuffer(m_device.Get(), &m_rayGenShaderTable, L"RayGenShaderTable");

	ShaderRecord missShaderRecord(missShaderIdentifier, shaderIdentifierSize, nullptr, 0);
	missShaderRecord.AllocateAsUploadBuffer(m_device.Get(), &m_missShaderTable, L"MissShaderTable");

	ShaderRecord hitGroupShaderRecord(hitGroupShaderIdentifier, shaderIdentifierSize, nullptr, 0);
	hitGroupShaderRecord.AllocateAsUploadBuffer(m_device.Get(), &m_hitGroupShaderTable, L"HitGroupShaderTable");
}

ID3D12Resource* DXDevice::CreateUploadBuffer(const void *pData, UINT64 datasize, const wchar_t* resourceName /*= "NameEmpty"*/)
{
	ID3D12Resource* ppResource = nullptr;
	ABORT_IF_FAILED_HR(m_device.Get()->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD), D3D12_HEAP_FLAG_NONE, &CD3DX12_RESOURCE_DESC::Buffer(datasize), D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&ppResource)));
	
	ppResource->SetName(resourceName);

	void* pMappedData = nullptr;
	ppResource->Map(0, nullptr, &pMappedData);
	memcpy(pMappedData, pData, datasize);
	ppResource->Unmap(0, nullptr);
	return ppResource;
}