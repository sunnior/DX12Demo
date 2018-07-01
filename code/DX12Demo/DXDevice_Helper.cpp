#include "DXDevice_Helper.h"
#include <sstream>
#include <iomanip>

using Microsoft::WRL::ComPtr;

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

WRAPPED_GPU_POINTER _CreateWrappedPointer(ID3D12Device* pDevice, ID3D12RaytracingFallbackDevice* pRTDevice, ID3D12DescriptorHeap* pHeap, UINT index, UINT descriptorSize, ID3D12Resource* pResource, UINT bufferNumElements)
{
	//assert(m_raytracingType == RaytracingType::FallbackLayer);

	D3D12_UNORDERED_ACCESS_VIEW_DESC rawBufferUavDesc = {};
	rawBufferUavDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
	rawBufferUavDesc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_RAW;
	rawBufferUavDesc.Format = DXGI_FORMAT_R32_TYPELESS;
	rawBufferUavDesc.Buffer.NumElements = bufferNumElements;

	// Only compute fallback requires a valid descriptor index when creating a wrapped pointer.
	D3D12_CPU_DESCRIPTOR_HANDLE bottomLevelDescriptor;
	bottomLevelDescriptor.ptr = CD3DX12_CPU_DESCRIPTOR_HANDLE(pHeap->GetCPUDescriptorHandleForHeapStart(), index, descriptorSize).ptr;

	pDevice->CreateUnorderedAccessView(pResource, nullptr, &rawBufferUavDesc, bottomLevelDescriptor);
	return pRTDevice->GetWrappedPointerSimple(index, pResource->GetGPUVirtualAddress());
}

void _LoadBinaryResource(LPCSTR name, const void*& data, size_t& size)
{
	HRSRC resourceInfo = FindResource(NULL, name, RT_RCDATA);
	ABORT_IF_FAILED(resourceInfo);

	HGLOBAL resourceData = LoadResource(NULL, resourceInfo);
	ABORT_IF_FAILED(resourceData);

	data = LockResource(resourceData);
	size = SizeofResource(NULL, resourceInfo);
}

void _SerializeAndCreateRaytracingRootSignature(ID3D12RaytracingFallbackDevice* pRaytracingDevice, D3D12_ROOT_SIGNATURE_DESC& desc, ID3D12RootSignature **ppRootSig)
{
	ComPtr<ID3DBlob> blob;
	ComPtr<ID3DBlob> error;

	ABORT_IF_FAILED_HR(pRaytracingDevice->D3D12SerializeRootSignature(&desc, D3D_ROOT_SIGNATURE_VERSION_1, &blob, &error));
	ABORT_IF_FAILED(error.Get() == nullptr);
	ABORT_IF_FAILED_HR(pRaytracingDevice->CreateRootSignature(1, blob->GetBufferPointer(), blob->GetBufferSize(), IID_PPV_ARGS(ppRootSig)));
}

// Pretty-print a state object tree
void _PrintStateObjectDesc(const D3D12_STATE_OBJECT_DESC* desc)
{
	std::wstringstream wstr;
	wstr << L"D3D12 State Object 0x" << static_cast<const void*>(desc) << L": ";
	if (desc->Type == D3D12_STATE_OBJECT_TYPE_COLLECTION) wstr << L"Collection\n";
	if (desc->Type == D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE) wstr << L"Raytracing Pipeline\n";

	auto ExportTree = [](UINT depth, UINT numExports, const D3D12_EXPORT_DESC* exports)
	{
		std::wostringstream woss;
		for (UINT i = 0; i < numExports; i++)
		{
			if (depth > 0)
			{
				for (UINT j = 0; j < 2 * depth - 1; j++) woss << L" ";
				woss << (i == numExports - 1 ? L"\xC0" : L"\xC3");
			}
			woss << L"[" << i << L"]: ";
			if (exports[i].ExportToRename) woss << exports[i].ExportToRename << L" --> ";
			woss << exports[i].Name << L"\n";
		}
		return woss.str();
	};

	for (UINT i = 0; i < desc->NumSubobjects; i++)
	{
		wstr << L"[" << i << L"]: ";
		if (desc->pSubobjects[i].Type == D3D12_STATE_SUBOBJECT_TYPE_FLAGS)
		{
			wstr << L"Flags (not yet defined)\n";
		}
		if (desc->pSubobjects[i].Type == D3D12_STATE_SUBOBJECT_TYPE_ROOT_SIGNATURE)
		{
			wstr << L"Root Signature 0x" << desc->pSubobjects[i].pDesc << L"\n";
		}
		if (desc->pSubobjects[i].Type == D3D12_STATE_SUBOBJECT_TYPE_LOCAL_ROOT_SIGNATURE)
		{
			wstr << L"Local Root Signature 0x" << desc->pSubobjects[i].pDesc << L"\n";
		}
		if (desc->pSubobjects[i].Type == D3D12_STATE_SUBOBJECT_TYPE_NODE_MASK)
		{
			wstr << L"Node Mask: 0x" << std::hex << std::setfill(L'0') << std::setw(8) << *static_cast<const UINT*>(desc->pSubobjects[i].pDesc) << std::setw(0) << std::dec << L"\n";
		}
		if (desc->pSubobjects[i].Type == D3D12_STATE_SUBOBJECT_TYPE_CACHED_STATE_OBJECT)
		{
			wstr << L"Cached State Object (not yet defined)\n";
		}
		if (desc->pSubobjects[i].Type == D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY)
		{
			wstr << L"DXIL Library 0x";
			auto lib = static_cast<const D3D12_DXIL_LIBRARY_DESC*>(desc->pSubobjects[i].pDesc);
			wstr << lib->DXILLibrary.pShaderBytecode << L", " << lib->DXILLibrary.BytecodeLength << L" bytes\n";
			wstr << ExportTree(1, lib->NumExports, lib->pExports);
		}
		if (desc->pSubobjects[i].Type == D3D12_STATE_SUBOBJECT_TYPE_EXISTING_COLLECTION)
		{
			wstr << L"Existing Library 0x";
			auto collection = static_cast<const D3D12_EXISTING_COLLECTION_DESC*>(desc->pSubobjects[i].pDesc);
			wstr << collection->pExistingCollection << L"\n";
			wstr << ExportTree(1, collection->NumExports, collection->pExports);
		}
		if (desc->pSubobjects[i].Type == D3D12_STATE_SUBOBJECT_TYPE_SUBOBJECT_TO_EXPORTS_ASSOCIATION)
		{
			wstr << L"Subobject to Exports Association (Subobject [";
			auto association = static_cast<const D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION*>(desc->pSubobjects[i].pDesc);
			UINT index = static_cast<UINT>(association->pSubobjectToAssociate - desc->pSubobjects);
			wstr << index << L"])\n";
			for (UINT j = 0; j < association->NumExports; j++) wstr << (j == association->NumExports - 1 ? L" \xC0" : L" \xC3") << L"[" << j << L"]: " << association->pExports[j] << L"\n";
		}
		if (desc->pSubobjects[i].Type == D3D12_STATE_SUBOBJECT_TYPE_DXIL_SUBOBJECT_TO_EXPORTS_ASSOCIATION)
		{
			wstr << L"DXIL Subobjects to Exports Association (";
			auto association = static_cast<const D3D12_DXIL_SUBOBJECT_TO_EXPORTS_ASSOCIATION*>(desc->pSubobjects[i].pDesc);
			wstr << association->SubobjectToAssociate << L")\n";
			for (UINT j = 0; j < association->NumExports; j++) wstr << (j == association->NumExports - 1 ? L" \xC0" : L" \xC3") << L"[" << j << L"]: " << association->pExports[j] << L"\n";
		}
		if (desc->pSubobjects[i].Type == D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_SHADER_CONFIG)
		{
			wstr << L"Raytracing Shader Config\n";
			auto config = static_cast<const D3D12_RAYTRACING_SHADER_CONFIG*>(desc->pSubobjects[i].pDesc);
			wstr << L" \xC3" << L"[0]: Max Payload Size: " << config->MaxPayloadSizeInBytes << L" bytes\n";
			wstr << L" \xC0" << L"[1]: Max Attribute Size: " << config->MaxAttributeSizeInBytes << L" bytes\n";
		}
		if (desc->pSubobjects[i].Type == D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_PIPELINE_CONFIG)
		{
			wstr << L"Raytracing Pipeline Config\n";
			auto config = static_cast<const D3D12_RAYTRACING_PIPELINE_CONFIG*>(desc->pSubobjects[i].pDesc);
			wstr << L" \xC0" << L"[0]: Max Recursion Depth: " << config->MaxTraceRecursionDepth << L"\n";
		}
		if (desc->pSubobjects[i].Type == D3D12_STATE_SUBOBJECT_TYPE_HIT_GROUP)
		{
			wstr << L"Hit Group (";
			auto hitGroup = static_cast<const D3D12_HIT_GROUP_DESC*>(desc->pSubobjects[i].pDesc);
			wstr << (hitGroup->HitGroupExport ? hitGroup->HitGroupExport : L"[none]") << L")\n";
			wstr << L" \xC3" << L"[0]: Any Hit Import: " << (hitGroup->AnyHitShaderImport ? hitGroup->AnyHitShaderImport : L"[none]") << L"\n";
			wstr << L" \xC3" << L"[1]: Closest Hit Import: " << (hitGroup->ClosestHitShaderImport ? hitGroup->ClosestHitShaderImport : L"[none]") << L"\n";
			wstr << L" \xC0" << L"[2]: Intersection Import: " << (hitGroup->IntersectionShaderImport ? hitGroup->IntersectionShaderImport : L"[none]") << L"\n";
		}
	}
	OutputDebugStringW(wstr.str().c_str());
}
