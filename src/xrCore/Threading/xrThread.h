#pragma once
////////////////////////////////////////////////////////////////////////////
//	Module 		: xrThread.h
//	Created 	: 11.05.2023
//  Modified 	: 03.04.2024
//	Author		: MAGILA (Kirill Belash Alexandrovich)
//	Description : Creation control and management of auxiliary threads
////////////////////////////////////////////////////////////////////////////

#include <tbb.h>
#include "xrCore/_types.h"
#include "xrCore/Threading/xrSyncronize.hpp"
#include "xrCore/Threading/Event.hpp"
#include <xrCommon\xr_list.h>

class XRCORE_API xrThread
{
	xr_string thread_name{ "X-RAY thread " };
	CTimer timer;
	DWORD thread_id{ 0 };
	Event process;
	Event done;
	std::jthread* Thread{ nullptr };
	bool thread_infinity{ false };
	bool thread_locked{ false };
	std::atomic_bool b_init{ false };
	std::atomic_bool send_local_state{ false };
	std::atomic_bool global_parallel_process{ false };

	std::atomic<float> ms_time{ 0.f };

	static IC DWORD main_thread_id;
	static IC xr_list<xrThread*> all_obj_thread;

public:
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
	
private:
	ParallelState global_parallel{ sParalelNone };
	std::atomic<ThreadState> thread_state{ dsSleep };

	std::function<void()> update_function;

private:
	void g_State(const ThreadState new_state);

	//Main function for worker thread.
	void worker_main(std::stop_token s_token);

public:
	xrThread(const LPCSTR name, bool infinity = false, bool locked = false);
	~xrThread();
	
	bool debug_info{ true };

	// Initializing and creating a Thread.
	void Init(std::function<void()> &&fn, ParallelState state = sParalelNone);

	// Name of thread.
	LPCSTR Name();

	// Get the id of the current thread.
	DWORD ID() const;

	// Is the thread initialized.
	bool IsInit() const;

	// At the place of the call, the thread unlocks.
	void Start() const;

	// At the place of the call, the thread is waiting for completion.
	void Wait() const;

	// At the place of the call, it starts and waits for the iteration of the thread to complete.
	void StartWait() const;

	// Setting the status of threads (dsOK = 0, dsSleep = 1, dsExit = 2).
	void State(const ThreadState new_state);

	// Get the status of the thread.
	const ThreadState GetState() const;

	// Enable synchronization with Device.
	void DeviceParallel(ParallelState state);

	// Whether the threads process is running.
	bool IsProcess() const;

	// Terminates the thread and destroys.
	void Stop();

	// Get the latest execution time in milliseconds.
	float GetTimeMs() const;

	// Remember the id of the main thread.
	static void id_main_thread(DWORD id);

	// Get the main thread ID.
	static const DWORD get_main_id();

	//Set the status of all threads created via xrThread.
	static void GlobalState(const ThreadState new_state);

	//For all threads.
	static void ForThreads(const std::function<void(xrThread& thread, bool& finish)>&& upd) noexcept;

	//Global Start threads.
	static void StartGlobal(ParallelState s_state);

	//Global Wait threads.
	static void WaitGlobal();
};