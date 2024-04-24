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

void xrThread::Init(std::function<void()> fn, ParallelState state)
{
	if (!IsInit())
	{
		lambda_function = new new_lambda(fn);
		Init(lambda_function, &new_lambda::Runfunction, state);
	}
};

void xrThread::worker_main()
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
		while (true)
		{
			if (thread_state != dsOK)
			{
				if (thread_state != dsExit)
				{
					Sleep(10);
					continue;
				}
			}

			if (thread_locked)
				process.Wait();

			if (thread_state == dsExit)
			{
				Msg("* Stop %s ID %d", thread_name.c_str(), thread_id);
				done.Set();
				break;
			}

			timer.Start();

			update_function();

			ms_time = timer.GetElapsed_sec() * 1000;

			if (thread_locked)
				done.Set();
		}

		if (thread_locked)
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
	if (thread_locked)
	{
		if (thread_state == dsOK)
		{
			process.Set();
			done.Wait();
		}
	}
}

bool xrThread::IsProcess() const
{
	if (thread_state == dsOK)
		return true;

	return false;
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

		if (lambda_function)
			delete lambda_function;
	}
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

void xrThread::ForThreads(const std::function<bool(xrThread*)> upd) noexcept
{
	for (xrThread*& thread : all_obj_thread)
		if (thread && upd(thread))
			break;
};