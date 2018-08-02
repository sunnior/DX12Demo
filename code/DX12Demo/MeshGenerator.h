#pragma once

#include <memory>
#include "Model.h"

class MeshGenerator
{
public:
	static std::unique_ptr<Model> CreateBox(const wchar_t* name);
	static std::unique_ptr<Model> CreateSphere(const wchar_t* name, float radius, UINT sliceCount, UINT stackCount);

};