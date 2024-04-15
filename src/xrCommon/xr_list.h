#pragma once
#include <list>

template <typename T, typename allocator = std::allocator<T>>
class xr_list : public std::list<T, allocator>
{
private:
	typedef std::list<T, allocator> inherited;

public:
	xr_list() : inherited() {};
	xr_list(std::initializer_list<T> initializer_list) : inherited(initializer_list) {}
	
	xr_list(size_t _count, const T& _value) : inherited(_count, _value) {}
	explicit xr_list(size_t _count) : inherited(_count) {}

	template <class _Ty>
	IC const_iterator find(const _Ty& obj) const
	{
		return std::find(inherited::begin(), inherited::end(), obj);
	}

	template <class _Ty>
	IC iterator find(const _Ty& obj)
	{
		return std::find(inherited::begin(), inherited::end(), obj);
	}

	template <class _Ty>
	IC bool contain(const _Ty& obj) const
	{
		return find(obj) != inherited::end();
	}

	template <class _Ty>
	IC bool contain(_Ty& obj)
	{
		return find(obj) != inherited::end();
	}
};

#define DEF_LIST(N, T)\
    using N = xr_list<T>;\
    using N##_it = N::iterator;

#define DEFINE_LIST(T, N, I)\
    using N = xr_list<T>;\
    using I = N::iterator;
