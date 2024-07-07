#pragma once
#include "Common/Noncopyable.hpp"

// Desc: Simple wrapper for critical section
class XRCORE_API xrCriticalSection : Noncopyable
{
	CRITICAL_SECTION pmutex;

public:
	class XRCORE_API raii
	{
	public:
		raii(xrCriticalSection&);
		~raii();

	private:
		xrCriticalSection* critical_section;
	};

public:
	xrCriticalSection();
	~xrCriticalSection();

	void Enter();
	void Leave();
	BOOL TryEnter();
};

// Non recursive
class XRCORE_API FastLock : Noncopyable
{
public:
	enum EFastLockType : ULONG
	{
		Exclusive = 0,
		Shared = CONDITION_VARIABLE_LOCKMODE_SHARED
	};

public:
	FastLock();
	~FastLock() {};

	void Enter();
	bool TryEnter();
	void Leave();

	void EnterShared();
	bool TryEnterShared();
	void LeaveShared();

	void* GetHandle();

private:
	SRWLOCK srw;
};