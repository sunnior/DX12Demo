#include "Engine.h"
#include "Timer.h"
#include "Model.h"
#include "MeshGenerator.h"

using namespace DirectX;
Engine* Engine::s_instance = nullptr;

Engine::Engine(DXDevice::WinParam winParam)
	: m_device(winParam)
	, m_camera(static_cast<FLOAT>(winParam.width) / static_cast<FLOAT>(winParam.height))
{
	if (!s_instance)
	{
		s_instance = this;
	}
}


Engine::~Engine()
{
	s_instance = nullptr;
}

void Engine::Init()
{
	
	m_box = MeshGenerator::CreateBox(L"box1");
	m_box2 = MeshGenerator::CreateBox(L"box2");
	m_sphere = MeshGenerator::CreateSphere(L"sphere", 0.3f, 10, 10);
	m_sphere->SetTransform(DirectX::XMFLOAT4X3{
		1, 0, 0, 2,
		0, 1, 0, 0,
		0, 0, 1, 0 }
	);

	m_box->SetTransform(DirectX::XMFLOAT4X3{
		1, 0, 0, 0,
		0, 1, 0, 0,
		0, 0, 1, 2 }
	);
	m_box2->SetTransform(DirectX::XMFLOAT4X3{
		1, 0, 0, 0,
		0, 1, 0, 0,
		0, 0, 1, -2 });
}

void Engine::Run()
{
	float dt = 0.0f;
	while (true)
	{
		DWORD beginTime = Timer::GetTimeMS();

		if (_HandleMessage()) { break; }

		m_keyboard.Update();
		m_camera.Update(dt);

		m_box->Draw();
		//m_box2->Draw();
		m_sphere->Draw();

		m_device.SetCamera(m_camera.GetProjectionToWorld(), m_camera.GetPosition());

		m_device.Begin();
		m_device.Run();
		m_device.End();

		DWORD endTime = Timer::GetTimeMS();
		dt = static_cast<float>(endTime - beginTime) / 1000;


	}
}

bool Engine::_HandleMessage()
{
	MSG msg = {};
	while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
	{
		TranslateMessage(&msg);

		switch (msg.message)
		{
		case WM_PAINT:
			return false;
			break;
		case WM_QUIT:
			return true;
			break;
		default:
			break;
		}
		DispatchMessage(&msg);
	}

	return false;
}