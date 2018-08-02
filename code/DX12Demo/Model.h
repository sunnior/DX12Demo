#pragma once

#include <vector>
#include <DirectXMath.h>
#include <d3d12.h>
#include <wrl.h>

class Model
{
public:
	struct Vertex_t {
		DirectX::XMFLOAT3 pos;
		DirectX::XMFLOAT3 normal;
	};
	typedef uint16_t Index_t;

public:
	Model(const wchar_t* name, const std::vector<Vertex_t>& vertices, const std::vector<Index_t>& indices);
	virtual ~Model();

	void Draw();

private:
	std::vector<Vertex_t> m_vertices;
	std::vector<Index_t> m_indices;

	Microsoft::WRL::ComPtr<ID3D12Resource> m_vertexBuffer;
	Microsoft::WRL::ComPtr<ID3D12Resource> m_indexBuffer;

	Microsoft::WRL::ComPtr<ID3D12Resource> m_bottomLevelAccelerationStructure;
	Microsoft::WRL::ComPtr<ID3D12Resource> m_instanceDescs;

};

