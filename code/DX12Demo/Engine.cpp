#include "Engine.h"

Engine::Engine(DXDevice::WinParam winParam)
	: m_device(winParam)
{

}


Engine::~Engine()
{
}

void Engine::Run()
{
	while (true)
	{
		if (_HandleMessage()) { break; }

		m_device.Begin();
		m_device.Run();
		m_device.End();
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