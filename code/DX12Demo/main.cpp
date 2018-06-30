#include "Engine.h"

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow)
{
	const int s_width = 1280;
	const int s_height = 720;

	Engine engine({ hInstance, nCmdShow, s_width, s_height });
	engine.Run();
	return 0;
}