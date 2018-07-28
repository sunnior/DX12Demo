#ifndef __KEYBOARD_H__
#define __KEYBOARD_H__

class Keyboard
{
public:
	enum class KeyCode {
		W,
		S,
		Q,
		E,
		A,
		D,
		Count,
	};
public:
	Keyboard();
	~Keyboard();

	void Update();
	bool IsPressed(KeyCode keycode);

private:
	void _Checkkey(KeyCode keycode, int vk);
private:
	bool m_isPressed[static_cast<int>(KeyCode::Count)] = {};
};

#endif // _DEBUG

