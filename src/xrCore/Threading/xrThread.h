#pragma once
////////////////////////////////////////////////////////////////////////////
//	Module 		: xrThread.h
//	Created 	: 11.05.2023
//  Modified 	: 03.04.2024
//	Author		: MAGILA (Kirill Belash Alexandrovich)
//	Description : Creation control and management of auxiliary threads
////////////////////////////////////////////////////////////////////////////

#ifndef E_EDITOR
#include <tbb.h>
#endif

#include "xrCore/_types.h"
#include "xrCore/Threading/xrSyncronize.hpp"
#include <xrCommon\xr_list.h>

// Event Manager
class XRCORE_API Event
{
private:
	void* handle;

public:
	Event() { handle = (void*)CreateEvent(NULL, FALSE, FALSE, NULL); }
	~Event() { CloseHandle(handle); }

	// Reset the event to the unsignalled state.
	IC void Reset() const { ResetEvent(handle); }
	// Set the event to the signalled state.
	IC void Set() const { SetEvent(handle); }
	// Wait indefinitely for the object to become signalled.
	IC void Wait() const { WaitForSingleObject(handle, INFINITE); }
	// Wait, with a time limit, for the object to become signalled.
	IC bool Wait(u32 millisecondsTimeout) const { return WaitForSingleObject(handle, millisecondsTimeout) != WAIT_TIMEOUT; }
	// Expected thread can issue a exception. But it require to process one message from HWND message queue, otherwise, thread can't show error message
	IC void WaitEx(u32 millisecondsTimeout) const
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

	void* GetHandle() { return handle; }

	bool operator==(const Event& other) const
	{
		return handle == other.handle;
	}
};

class XRCORE_API xrThread
{
public:
	xrThread(const LPCSTR name, bool infinity = false, bool locked = false);
	~xrThread();

	enum ThreadState
	{
		dsOK = 0,
		dsSleep,
		dsExit
	};

	enum ParallelState
	{
		sParalelNone = 0,
		sParalelFrame,
		sParalelRender
	};
	
	float ms_time{ 0.f };

private:
	xr_string thread_name{ "X-RAY thread " };
	CTimer timer;
	DWORD thread_id{ 0 };
	Event process;
	Event done;
	bool thread_infinity{ false };
	bool thread_locked{ false };
	bool b_init{ false };
	bool send_local_state{ false };
	ParallelState global_parallel{ sParalelNone };
	bool global_parallel_process{ false };
	bool debug_info{ true };

	static IC DWORD main_thread_id;
	static IC xr_list<xrThread*> all_obj_thread;

	std::function<void()> update_function{ []() {} };
	ThreadState thread_state{ dsSleep };

	IC void g_State(const ThreadState new_state)
	{
		if (new_state == dsExit)
			Stop();
		else
			thread_state = new_state;
	}

	//Main function for worker thread.
	void worker_main();

public:

	static IC void init_main_thread()
	{
		main_thread_id = GetCurrentThreadId();
	};

	static IC const DWORD get_main_id()
	{
		return main_thread_id;
	};

	IC LPCSTR Name()
	{
		return thread_name.c_str();
	};

	IC bool UsName(LPCSTR name)
	{
		return  thread_name.find(name) != xr_string::npos;
	};

	IC DWORD ID() const
	{
		return thread_id;
	};

	//Initializing and creating a Thread.
	void Init(std::function<void()> new_function, ParallelState state = sParalelNone);

	//Is the thread initialized.
	bool IsInit() const;

	//At the place of the call, the thread unlocks
	void Start() const;

	//At the place of the call, the thread is waiting for completion
	void Wait() const;

	//At the place of the call, it starts and waits for the iteration of the thread to complete.
	void StartWait() const;

	//Setting the status of threads (dsOK = 0, dsSleep = 1, dsExit = 2).
	IC void State(const ThreadState new_state)
	{
		send_local_state = true;
		g_State(new_state);
	};

	//Get the status of the thread;
	IC const ThreadState GetState() const { return thread_state; };

	//Enable synchronization with Device.
	IC void DeviceParallel(ParallelState state)
	{
		global_parallel = state;
	};

	IC void DebugFalse()
	{
		debug_info = false;
	};

	IC const bool DebugInfo() const
	{
		return debug_info;
	};

	//Set the status of all threads created via xrThread.
	static void GlobalState(const ThreadState new_state);

	//For all threads.
	static void ForThreads(const std::function<bool(xrThread*)> upd) noexcept;

	//Global Start threads.
	IC static void StartGlobal(ParallelState s_state)
	{
		ForThreads([&](xrThread* thread) {
			if (thread->global_parallel == s_state)
			{
				thread->global_parallel_process = true;
				thread->Start();
			}

			return false;
		});
	};

	//Global Wait threads.
	IC static void WaitGlobal()
	{
		ForThreads([&](xrThread* thread) {
			if (thread->global_parallel_process && thread->global_parallel != sParalelNone)
			{
				thread->global_parallel_process = false;
				thread->Wait();
			}

			return false;
		});
	};

	//Whether the threads process is running.
	bool IsProcess() const;

	//Terminates the thread and destroys
	void Stop();
};