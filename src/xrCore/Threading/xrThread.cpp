#include "stdafx.h"
#include "combaseapi.h"
#include "xrThread.h"

using namespace std;

xrThread::xrThread(const LPCSTR name, bool infinity, bool locked)
{
	thread_name.append(name);
	thread_infinity = infinity;
	thread_locked = locked;
}

xrThread::~xrThread()
{
	if (b_init && thread_state != dsExit)
		Stop();
}

void xrThread::worker_main(stop_token s_token)
{
	thread_id = GetCurrentThreadId();
	
	Msg("* Start %s ID %d", thread_name.c_str(), thread_id);

	const HRESULT co_initialize = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
	if (S_FALSE == co_initialize)
		Msg("~ The COM library has already been initialized in this thread %s ID %d", thread_name.c_str(), thread_id);
	else if (RPC_E_CHANGED_MODE == co_initialize)
	{
		Msg("~ A previous call to CoInitializeEx defined the concurrency model for the thread[%s] ID[%d] as a multithreaded apartment (MTA).\n\
		It may also indicate that there has been a change from a neutral-flow module to a single-threaded module.", thread_name.c_str(), thread_id);
	}
	
	if (thread_infinity)
	{
		while (!s_token.stop_requested())
		{
			if (thread_state == dsSleep)
			{
				this_thread::yield();
				this_thread::sleep_for(100ms);
				continue;
			}

			if (thread_locked)
				process.Wait();

			if (thread_state == dsExit)
				break;

			timer.Start();

			update_function();

			float ms = timer.GetElapsed_sec() * 1000;
			ms >= ms_time ? ms_time = ms : ms_time -= ((ms_time - ms) * .03f);

			if (thread_locked)
				done.Set();
		}

		Msg("* Stop %s ID %d", thread_name.c_str(), thread_id);
		done.Set();
	}
	else
	{
		if (thread_locked)
			process.Wait();

		timer.Start();
		update_function();
		ms_time = timer.GetElapsed_sec() * 1000;

		if (thread_locked)
			done.Set();

		b_init = false;
	}
}

void xrThread::g_State(const ThreadState new_state)
{
	if (new_state == dsExit)
		Stop();
	else
		thread_state = new_state;
}

void xrThread::Init(function<void()> &&fn, ParallelState state)
{
	if (!IsInit())
	{
		thread_state = dsOK;
		update_function = move(fn);
		b_init = true;
		all_obj_thread.push_back(move(this));

		Thread = new jthread{ [this](stop_token s_token) -> void {
			worker_main(s_token);
		}};

		global_parallel = state;
	}
}

LPCSTR xrThread::Name()
{
	return thread_name.c_str();
}

DWORD xrThread::ID() const
{
	return thread_id;
}

bool xrThread::IsInit() const
{
	return b_init;
}

void xrThread::Start() const
{
	if (thread_locked && IsInit())
	{
		if (thread_state == dsOK)
			process.Set();
	}
}

void xrThread::Wait() const
{
	if (thread_locked && IsInit())
		if (thread_state == dsOK)
			done.Wait();
}

void xrThread::StartWait() const
{
	if (thread_locked && thread_state == dsOK)
	{
		process.Set();
		done.Wait();
	}
}

void xrThread::State(const ThreadState new_state)
{
	send_local_state = true;
	g_State(new_state);
}

const xrThread::ThreadState xrThread::GetState() const
{
	return thread_state;
}

void xrThread::DeviceParallel(ParallelState state)
{
	global_parallel = state;
}

bool xrThread::IsProcess() const
{
	return thread_state == dsOK;
}

void xrThread::Stop()
{
	thread_state = dsExit;
	if (b_init)
	{
		b_init = false;

		auto thread_obj = all_obj_thread.find(this);
		if (thread_obj != all_obj_thread.end())
			all_obj_thread.erase(thread_obj);

		if (thread_locked)
			process.Set();

		done.Wait();

		Thread->request_stop();
		xr_delete(Thread);
	}
}

float xrThread::GetTimeMs() const
{
	return ms_time;
}

void xrThread::id_main_thread(DWORD id)
{
	main_thread_id = id;
}

const DWORD xrThread::get_main_id()
{
	return main_thread_id;
}

void xrThread::GlobalState(const ThreadState new_state)
{
	static ThreadState cache_state{ dsExit };
	//We will not re-assign the same status to threads.
	if (cache_state != new_state)
	{
		if (dsExit == new_state)
		{
			while (!all_obj_thread.empty())
				all_obj_thread.front()->g_State(new_state);
		}
		else
		{
			for (xrThread*& thread : all_obj_thread)
				if (thread && !thread->send_local_state)
					thread->g_State(new_state);
		}

		cache_state = new_state;
	}
}

void xrThread::ForThreads(const function<void(xrThread&, bool&)>&& upd) noexcept
{
	for (xrThread*& thread : all_obj_thread)
	{
		if (thread)
		{
			bool finish{ false };
			upd(*thread, finish);
			if (finish)
				break;
		}
	}
}

void xrThread::StartGlobal(ParallelState s_state)
{
	ForThreads([s_state](xrThread& thread, bool&) -> void {
		if (thread.global_parallel == s_state)
		{
			thread.global_parallel_process = true;
			thread.Start();
		}
	});
}

void xrThread::WaitGlobal()
{
	ForThreads([](xrThread& thread, bool&) -> void {
		if (thread.global_parallel_process && thread.global_parallel != sParalelNone)
		{
			thread.global_parallel_process = false;
			thread.Wait();
		}
	});
}