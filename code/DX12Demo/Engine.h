#ifndef __ENGINE_HEADER_H__
#define __ENGINE_HEADER_H__

#include "DXDevice.h"
#include "Keyboard.h"

class Engine
{
public:
	Engine(DXDevice::WinParam winParam);
	~Engine();

	void Run();

	Keyboard& GetKeyboard() { return m_keyboard; }
	static Engine* GetInstance() { return s_instance; }
private:
	bool _HandleMessage();

private:
	DXDevice m_device;
	Keyboard m_keyboard;

	static Engine* s_instance;
};

#endif // _DEBUG

