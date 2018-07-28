#ifndef __KEYBOARD_H__
#define __KEYBOARD_H__

class Keyboard
{
public:
	enum class KeyCode {
		W,
		Count,
	};
public:
	Keyboard();
	~Keyboard();

	void Update();
	bool IsPressed(KeyCode keycode);

private:
	bool m_isPressed[static_cast<int>(KeyCode::Count)] = {};
};

#endif // _DEBUG

