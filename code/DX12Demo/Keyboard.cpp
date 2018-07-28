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
	SHORT result = GetAsyncKeyState(w_key);
	m_isPressed[static_cast<int>(KeyCode::W)] = result != 0;
}

bool Keyboard::IsPressed(KeyCode keycode)
{
	return m_isPressed[static_cast<int>(keycode)];
}
