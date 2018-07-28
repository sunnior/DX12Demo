#include "Keyboard.h"
#include <Windows.h>
#include <sstream>

Keyboard::Keyboard()
{

}

Keyboard::~Keyboard()
{

}

void Keyboard::Update()
{
	const int w_key = 0x57;
	const int s_key = 0x53;
	const int q_key = 0x51;
	const int e_key = 0x45;
	const int a_key = 0x41;
	const int d_key = 0x44;

	_Checkkey(KeyCode::W, w_key);
	_Checkkey(KeyCode::S, s_key);
	_Checkkey(KeyCode::Q, q_key);
	_Checkkey(KeyCode::E, e_key);
	_Checkkey(KeyCode::A, a_key);
	_Checkkey(KeyCode::D, d_key);
}

bool Keyboard::IsPressed(KeyCode keycode)
{
	return m_isPressed[static_cast<int>(keycode)];
}

void Keyboard::_Checkkey(KeyCode keycode, int vk)
{
	SHORT result = GetAsyncKeyState(vk);
	m_isPressed[static_cast<int>(keycode)] = result != 0;


}
