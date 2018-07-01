#include "DXDevice_Helper.h"

void _CreateUploadBuffer(ID3D12Device* pDevice, ID3D12Resource **ppResource, const void *pData, UINT64 datasize, const wchar_t* resourceName /*= nullptr*/)
{

	ABORT_IF_FAILED_HR(pDevice->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD), D3D12_HEAP_FLAG_NONE, &CD3DX12_RESOURCE_DESC::Buffer(datasize), D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(ppResource)));


	if (resourceName)
	{
		(*ppResource)->SetName(resourceName);
	}

	void* pMappedData = nullptr;
	(*ppResource)->Map(0, nullptr, &pMappedData);
	memcpy(pMappedData, pData, datasize);
	(*ppResource)->Unmap(0, nullptr);
}

void _CreateUAVBuffer(ID3D12Device* pDevice, ID3D12Resource **ppResource, UINT64 bufferSize, D3D12_RESOURCE_STATES initialResourceState /*= D3D12_RESOURCE_STATE_COMMON*/, const wchar_t* resourceName /*= nullptr*/)
{
	ABORT_IF_FAILED_HR(pDevice->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(bufferSize, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS),
		initialResourceState,
		nullptr,
		IID_PPV_ARGS(ppResource)));
	if (resourceName)
	{
		(*ppResource)->SetName(resourceName);
	}
}

WRAPPED_GPU_POINTER _CreateWrappedPointer(ID3D12Device* pDevice, ID3D12RaytracingFallbackDevice* pRTDevice, ID3D12DescriptorHeap* pHeap, ID3D12Resource* pResource, UINT bufferNumElements)
{
	//assert(m_raytracingType == RaytracingType::FallbackLayer);

	D3D12_UNORDERED_ACCESS_VIEW_DESC rawBufferUavDesc = {};
	rawBufferUavDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
	rawBufferUavDesc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_RAW;
	rawBufferUavDesc.Format = DXGI_FORMAT_R32_TYPELESS;
	rawBufferUavDesc.Buffer.NumElements = bufferNumElements;


	// Only compute fallback requires a valid descriptor index when creating a wrapped pointer.
	D3D12_CPU_DESCRIPTOR_HANDLE bottomLevelDescriptor = pHeap->GetCPUDescriptorHandleForHeapStart();

	pDevice->CreateUnorderedAccessView(pResource, nullptr, &rawBufferUavDesc, bottomLevelDescriptor);
	return pRTDevice->GetWrappedPointerSimple(0, pResource->GetGPUVirtualAddress());
}
