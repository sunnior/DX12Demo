#ifndef __ENGINE_HEADER_H__
#define __ENGINE_HEADER_H__

#include "DXDevice.h"

class Engine
{
public:
	Engine(DXDevice::WinParam winParam);
	~Engine();

	void Run();
private:
	bool _HandleMessage();

private:
	DXDevice m_device;
};

#endif // _DEBUG

