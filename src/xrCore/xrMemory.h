#pragma once

#include "_types.h"

class XRCORE_API xrMemory
{
	// Additional 16 bytes of memory almost like in original xr_aligned_offset_malloc
	// But for DEBUG we don't need this if we want to find memory problems
#ifdef DEBUG
	const size_t reserved = 0;
#else
	const size_t reserved = 16;
#endif

public:
	xrMemory();
	void _initialize();
	void _destroy();

	u32 stat_calls;

public:
	struct SProcessMemInfo
	{
		u64 PeakWorkingSetSize;
		u64 WorkingSetSize;
		u64 PagefileUsage;
		u64 PeakPagefileUsage;

		u64 TotalPhysicalMemory;
		s64 FreePhysicalMemory;
		u64 TotalVirtualMemory;
		u32 MemoryLoad;
	};

	void GetProcessMemInfo(SProcessMemInfo& minfo);
	size_t mem_usage();
	void mem_compact();
	IC void* mem_alloc(size_t size)
	{
		stat_calls++;
		return malloc(size + reserved);
	};
	IC void* mem_realloc(void* ptr, size_t size)
	{
		stat_calls++;
		return realloc(ptr, size + reserved);
	};
	IC void mem_free(void* ptr)
	{
		stat_calls++;
		free(ptr);
	};
};

extern XRCORE_API xrMemory Memory;

#undef ZeroMemory
#undef CopyMemory
#undef FillMemory
#define ZeroMemory(a, b) memset(a, 0, b)
#define CopyMemory(a, b, c) memcpy(a, b, c)
#define FillMemory(a, b, c) memset(a, c, b)

/*
Начиная со стандарта C++11 нет необходимости объявлять все формы операторов new и delete.
*/
IC void* operator new(size_t size) { return Memory.mem_alloc(size); }

IC void operator delete(void* ptr) noexcept { Memory.mem_free(ptr); }


template <class T>
IC void xr_delete(T*& ptr) noexcept
{
	if (ptr)
	{
		delete ptr;
		ptr = nullptr;
	}
}

// generic "C"-like allocations/deallocations
template <class T>
IC T* xr_alloc(size_t count)
{
	return (T*)Memory.mem_alloc(count * sizeof(T));
}
template <class T>
IC void xr_free(T*& ptr) noexcept
{
	if (ptr)
	{
		Memory.mem_free((void*)ptr);
		ptr = nullptr;
	}
}
IC void* xr_malloc(size_t size) { return Memory.mem_alloc(size); }
IC void* xr_realloc(void* ptr, size_t size) { return Memory.mem_realloc(ptr, size); }

XRCORE_API pstr xr_strdup(pcstr string);

XRCORE_API void log_vminfo();
