#include "Engine.h"
#include "Timer.h"

Engine* Engine::s_instance = nullptr;

Engine::Engine(DXDevice::WinParam winParam)
	: m_device(winParam)
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

void Engine::Run()
{
	float dt = 0.0f;
	while (true)
	{
		DWORD beginTime = Timer::GetTimeMS();

		if (_HandleMessage()) { break; }

		m_keyboard.Update();

		m_device.Begin();
		m_device.Run(dt);
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