#pragma once

#include "_types.h"
#include "cpuid.h"

namespace CPU
{
	XRCORE_API extern u64 qpc_freq;
	XRCORE_API extern u64 qpc_overhead;
	XRCORE_API extern u32 qpc_counter;

	XRCORE_API extern _processor_info ID;

	XRCORE_API extern u64 QPC();

	IC u64 GetCLK() { return __rdtsc(); }

	extern void Detect();
};

extern XRCORE_API void _initialize_cpu();