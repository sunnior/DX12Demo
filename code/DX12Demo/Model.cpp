#include "Model.h"
#include "Engine.h"

Model::Model(const wchar_t* name, const std::vector<Vertex_t>& vertices, const std::vector<Index_t>& indices)
	: m_vertices(vertices)
	, m_indices(indices)
{
	DXDevice& device = Engine::GetInstance()->GetDevice();
	m_vertexBuffer = device.CreateUploadBuffer(m_vertices.data(), sizeof(Vertex_t)* m_vertices.size(), name);
	m_indexBuffer = device.CreateUploadBuffer(m_indices.data(), sizeof(Index_t)* m_indices.size(), name);

}

Model::~Model()
{

}

void Model::Draw()
{
	DXDevice& device = Engine::GetInstance()->GetDevice();
	//todo async build.
	if (m_bottomLevelAccelerationStructure == nullptr)
	{
		device.CreateBottomLevelAS(
			m_vertexBuffer->GetGPUVirtualAddress(), static_cast<UINT>(m_vertexBuffer->GetDesc().Width) / sizeof(Vertex_t), sizeof(Vertex_t),
			m_indexBuffer->GetGPUVirtualAddress(), static_cast<UINT>(m_indexBuffer->GetDesc().Width) / sizeof(Index_t),
			m_instanceDesc, m_transform,
			&m_bottomLevelAccelerationStructure);
	}

	device.AddInstance(m_instanceDesc);
}

void Model::SetTransform(const DirectX::XMFLOAT4X3& transform)
{
	m_transform = transform;
}
