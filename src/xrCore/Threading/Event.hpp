#pragma once
#include "Common/Noncopyable.hpp"

// Event Manager
class XRCORE_API Event : Noncopyable
{
	void* handle;

public:
	Event();
	~Event();

	// Reset the event to the unsignalled state.
	void Reset() const;

	// Set the event to the signalled state.
	void Set() const;

	// Wait indefinitely for the object to become signalled.
	void Wait() const;

	// Wait, with a time limit, for the object to become signalled.
	bool Wait(size_t millisecondsTimeout) const;

	// Expected thread can issue a exception. But it require to process one message from HWND message queue, otherwise, thread can't show error message
	void WaitEx(size_t millisecondsTimeout) const;

	void* GetHandle();

	bool operator==(const Event& other) const
	{
		return handle == other.handle;
	}
};