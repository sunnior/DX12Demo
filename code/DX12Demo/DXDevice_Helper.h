#ifndef __DXDEVICE_HELPER_HEADER_H__
#define __DXDEVICE_HELPER_HEADER_H__

#include <Windows.h>
#include <d3d12.h>
#include <wrl.h>
#include <dxgi1_6.h>
#include "d3dx12.h"
#include "d3d12_1.h"
#include "D3D12RaytracingFallback.h"

static void abortIfFailHr(HRESULT hr)
{
	if (hr < 0) {
		abort();
	}
}
#define ABORT_IF_FAILED_HR(hr) abortIfFailHr(hr)
#define ABORT_IF_FAILED(exp)      if(!(exp)) abort()

void _CreateUploadBuffer(ID3D12Device* pDevice, ID3D12Resource **ppResource, const void *pData, UINT64 datasize, const wchar_t* resourceName = nullptr);
void _CreateUAVBuffer(ID3D12Device* pDevice, ID3D12Resource **ppResource, UINT64 bufferSize, D3D12_RESOURCE_STATES initialResourceState = D3D12_RESOURCE_STATE_COMMON, const wchar_t* resourceName = nullptr);

WRAPPED_GPU_POINTER _CreateWrappedPointer(ID3D12Device* pDevice, ID3D12RaytracingFallbackDevice* pRTDevice, ID3D12DescriptorHeap* pHeap, ID3D12Resource* pResource, UINT bufferNumElements);

#endif // _DEBUG

