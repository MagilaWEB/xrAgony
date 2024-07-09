#include "stdafx.h"

xrCriticalSection::xrCriticalSection()
{
	InitializeCriticalSection(&pmutex);
}

xrCriticalSection::~xrCriticalSection()
{
	DeleteCriticalSection(&pmutex);
}

void xrCriticalSection::Enter()
{
	EnterCriticalSection(&pmutex);
}

void xrCriticalSection::Leave()
{
	LeaveCriticalSection(&pmutex);
}

BOOL xrCriticalSection::TryEnter()
{
	return TryEnterCriticalSection(&pmutex);
}

xrCriticalSection::raii::raii(xrCriticalSection& other) : critical_section(&std::forward<xrCriticalSection&>(other))
{
	VERIFY(critical_section);
	critical_section->Enter();
}

xrCriticalSection::raii::~raii()
{
	critical_section->Leave();
}

FastLock::FastLock() { InitializeSRWLock(&srw); }

void FastLock::Enter() { AcquireSRWLockExclusive(&srw); }

bool FastLock::TryEnter() { return 0 != TryAcquireSRWLockExclusive(&srw); }

void FastLock::Leave() { ReleaseSRWLockExclusive(&srw); }

void FastLock::EnterShared() { AcquireSRWLockShared(&srw); }

bool FastLock::TryEnterShared() { return 0 != TryAcquireSRWLockShared(&srw); }

void FastLock::LeaveShared() { ReleaseSRWLockShared(&srw); }

void* FastLock::GetHandle() { return reinterpret_cast<void*>(&srw); }

FastLock::raii::raii(FastLock& other) : fast_lock(&std::forward<FastLock&>(other))
{
	VERIFY(fast_lock);
	fast_lock->Enter();
}

FastLock::raii::~raii()
{
	fast_lock->Leave();
}