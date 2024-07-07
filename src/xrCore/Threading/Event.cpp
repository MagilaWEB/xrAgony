#include "stdafx.h"

Event::Event()
{
	handle = (void*)CreateEvent(nullptr, FALSE, FALSE, nullptr);
}

Event::~Event()
{
	CloseHandle(handle);
}

void Event::Reset() const
{
	ResetEvent(handle);
}

void Event::Set() const
{
	SetEvent(handle);
}

void Event::Wait() const
{
	WaitForSingleObject(handle, INFINITE);
}

bool Event::Wait(size_t millisecondsTimeout) const
{
	return WaitForSingleObject(handle, millisecondsTimeout) != WAIT_TIMEOUT;
}

void Event::WaitEx(size_t millisecondsTimeout) const
{
	DWORD WaitResult = WAIT_TIMEOUT;
	do
	{
		WaitResult = WaitForSingleObject(handle, millisecondsTimeout);
		if (WaitResult == WAIT_TIMEOUT)
		{
			MSG msg;
			if (PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE))
			{
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
		}
	} while (WaitResult == WAIT_TIMEOUT);
}

void* Event::GetHandle()
{
	return handle;
}
