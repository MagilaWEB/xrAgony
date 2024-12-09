#pragma once
#include "concurrent_vector.h"

template <typename T, typename allocator = std::allocator<T> >
class xr_c_vector : public ::concurrency::concurrent_vector < T, allocator >
{
private:
	using inherited = ::concurrency::concurrent_vector<T, allocator>;

public:
	using const_iterator = inherited::const_iterator;
	using iterator = inherited::iterator;

public:
	xr_c_vector() : inherited() {}
	xr_c_vector(size_t _count, const T& _value) : inherited(_count, _value) {}
	explicit xr_c_vector(size_t _count) : inherited(_count) {}

	IC const_iterator find(const T& obj) const
	{
		return std::find(inherited::begin(), inherited::end(), obj);
	}

	IC iterator find(T& obj)
	{
		return std::find(inherited::begin(), inherited::end(), obj);
	}

	IC bool contain(const T& obj) const
	{
		return find(obj) != inherited::end();
	}

	IC bool contain(T& obj)
	{
		return find(obj) != inherited::end();
	}

	template <typename _Pr>
	IC iterator find_if(_Pr obj)
	{
		return std::find_if(inherited::begin(), inherited::end(), obj);
	}

	template <typename _Pr>
	IC bool contain_if(_Pr obj)
	{
		return find_if(obj) != inherited::end();
	}
};

#define DEF_C_VECTOR(N, T)\
	using N = xr_vector<T>;\
	using N##_it = N::iterator;

#define DEFINE_C_VECTOR(T, N, I)\
	using N = xr_vector<T>;\
	using I = N::iterator;