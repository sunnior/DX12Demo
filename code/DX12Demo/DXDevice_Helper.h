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

WRAPPED_GPU_POINTER _CreateWrappedPointer(ID3D12Device* pDevice, ID3D12RaytracingFallbackDevice* pRTDevice, ID3D12DescriptorHeap* pHeap, UINT index, UINT descriptorSize, ID3D12Resource* pResource, UINT bufferNumElements);
void _SerializeAndCreateRaytracingRootSignature(ID3D12RaytracingFallbackDevice* pRaytracingDevice, D3D12_ROOT_SIGNATURE_DESC& desc, ID3D12RootSignature **ppRootSig);

void _LoadBinaryResource(LPCSTR name, const void*& data, size_t& size);
void _PrintStateObjectDesc(const D3D12_STATE_OBJECT_DESC* desc);

class ShaderRecord
{
public:
	ShaderRecord(void* pShaderIdentifier, UINT shaderIdentifierSize) :
		shaderIdentifier(pShaderIdentifier, shaderIdentifierSize)
	{
	}

	ShaderRecord(void* pShaderIdentifier, UINT shaderIdentifierSize, void* pLocalRootArguments, UINT localRootArgumentsSize) :
		shaderIdentifier(pShaderIdentifier, shaderIdentifierSize),
		localRootArguments(pLocalRootArguments, localRootArgumentsSize)
	{
	}
	UINT Size() { return shaderIdentifier.size + localRootArguments.size; }

	inline UINT Align(UINT size, UINT alignment)
	{
		return (size + (alignment - 1)) & ~(alignment - 1);
	}

	void AllocateAsUploadBuffer(ID3D12Device* pDevice, ID3D12Resource **ppResource, const wchar_t* resourceName = nullptr)
	{
		const auto uploadHeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
		UINT size = Align(Size(), D3D12_RAYTRACING_SHADER_RECORD_BYTE_ALIGNMENT);
		auto bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(size);
		ABORT_IF_FAILED_HR(pDevice->CreateCommittedResource(
			&uploadHeapProperties,
			D3D12_HEAP_FLAG_NONE,
			&bufferDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(ppResource)));
		if (resourceName)
		{
			(*ppResource)->SetName(resourceName);
		}
		UINT8 *pMappedData;
		(*ppResource)->Map(0, nullptr, reinterpret_cast<void**>(&pMappedData));
		memcpy(pMappedData, shaderIdentifier.ptr, shaderIdentifier.size);
		memcpy(pMappedData + shaderIdentifier.size, localRootArguments.ptr, localRootArguments.size);
		(*ppResource)->Unmap(0, nullptr);
	}

	struct PointerWithSize {
		void *ptr;
		UINT size;

		PointerWithSize() : ptr(nullptr), size(0) {}
		PointerWithSize(void* _ptr, UINT _size) : ptr(_ptr), size(_size) {};
	};
	PointerWithSize shaderIdentifier;
	PointerWithSize localRootArguments;

};


#endif // _DEBUG

