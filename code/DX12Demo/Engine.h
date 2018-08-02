#ifndef __ENGINE_HEADER_H__
#define __ENGINE_HEADER_H__

#include <memory>
#include "DXDevice.h"
#include "Keyboard.h"
#include "Camera.h"

class Engine
{
public:
	Engine(DXDevice::WinParam winParam);
	~Engine();

	void Init();
	void Run();

	Keyboard& GetKeyboard() { return m_keyboard; }
	DXDevice& GetDevice() { return m_device; }

	static Engine* GetInstance() { return s_instance; }
private:
	bool _HandleMessage();

private:
	DXDevice m_device;
	Keyboard m_keyboard;
	Camera m_camera;
	std::unique_ptr<class Model> m_box;

	static Engine* s_instance;
};

#endif // _DEBUG

