#pragma once
struct xr_allocator
{
	template <typename T>
	struct helper
	{
		typedef std::allocator<T> result;
	};

	template <typename T, typename... Args>
	static void construct(T* ptr, Args&&... args)
	{
		new (ptr) T(std::forward<Args>(args)...);
	}

	template <typename T>
	static void destroy(T* p)
	{
		p->~T();
	}

	static void* alloc(const size_t& n) { return xr_alloc<size_t>(n); }

	template <typename T>
	static void dealloc(T*& p)
	{
		xr_free(p);
	}
};