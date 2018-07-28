#include "Timer.h"
#include <Windows.h>

DWORD Timer::GetTimeMS()
{
	return GetTickCount();
}
