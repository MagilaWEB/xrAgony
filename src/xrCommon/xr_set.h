#pragma once
#include <set>

template <typename K, class P = std::less<K>, typename allocator = std::allocator<K>>
using xr_set = std::set<K, P, allocator>;

template <typename K, class P = std::less<K>, typename allocator = std::allocator<K>>
using xr_multiset = std::multiset<K, P, allocator>;

#define DEFINE_SET(T, N, I)\
	using N = xr_set<T>;\
	using I = N::iterator;

#define DEFINE_SET_PRED(T, N, I, P)\
	using N = xr_set<T, P>;\
	using I = N::iterator;
