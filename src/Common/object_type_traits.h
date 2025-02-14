////////////////////////////////////////////////////////////////////////////
//  Module	  : object_type_traits.h
//  Created	 : 21.01.2003
//  Modified	: 12.07.2004
//  Author	  : Dmitriy Iassenev
//  Description : Object type traits
////////////////////////////////////////////////////////////////////////////
#pragma once
#ifndef object_type_traits_h_included
#define object_type_traits_h_included

#include <type_traits>

#define declare_has(a)											\
	template <typename T>										 \
	struct has_##a												\
	{															 \
		template <typename P>									 \
		static detail::yes select(detail::other<typename P::a>*); \
		template <typename P>									 \
		static detail::no select(...);							\
		enum													  \
		{														 \
			value = sizeof(detail::yes) == sizeof(select<T>(0))	\
		};														\
	};

template <bool expression, typename T1, typename T2>
struct _if
{
	using result = typename std::conditional<expression, T1, T2>::type;
};

template <typename T>
struct type
{
	typedef T result;
};

namespace object_type_traits
{
namespace detail
{
struct yes
{
	char a[1];
};
struct no
{
	char a[2];
};
template <typename T>
struct other
{
};
};

using std::remove_pointer;

template <typename T>
struct remove_reference
{
	typedef T type;
};

template <typename T>
struct remove_reference<T&>
{
	typedef T type;
};

template <typename T>
struct remove_reference<T const&>
{
	typedef T type;
};

using std::remove_const;

template <typename T>
struct remove_noexcept;

template <typename R, typename... Args>
struct remove_noexcept<R(Args...) noexcept>
{
	using type = R(Args...);
};

template <typename R, typename... Args>
struct remove_noexcept<R (*)(Args...) noexcept>
{
	using type = R (*)(Args...);
};

template <typename C, typename R, typename... Args>
struct remove_noexcept<R (C::*)(Args...) noexcept>
{
	using type = R (C::*)(Args...);
};

template <typename C, typename R, typename... Args>
struct remove_noexcept<R (C::*)(Args...) const noexcept>
{
	using type = R (C::*)(Args...) const;
};

#define REMOVE_NOEXCEPT(fn) (object_type_traits::remove_noexcept<decltype(fn)>::type)(fn)

using std::is_void;
using std::is_const;
using std::is_pointer;
using std::is_reference;

template <typename _T1, typename _T2>
struct is_same
{
	typedef typename remove_const<_T1>::type T1;
	typedef typename remove_const<_T2>::type T2;

	static constexpr bool value = std::is_same<T1, T2>::value;
};

template <typename _T1, typename _T2>
struct is_base_and_derived
{
	typedef typename remove_const<_T1>::type T1;
	typedef typename remove_const<_T2>::type T2;

	static detail::yes select(T1*);
	static detail::no select(...);

	enum
	{
		value = std::is_class<T1>::value && std::is_class<T2>::value && !is_same<T1, T2>::value &&
			sizeof(detail::yes) == sizeof(select((T2*)(0)))
	};
};

declare_has(iterator);
declare_has(const_iterator);
declare_has(value_type);
declare_has(size_type);

template <typename T>
struct is_stl_container
{
	enum
	{
		value = has_iterator<T>::value && has_const_iterator<T>::value &&
			has_size_type<T>::value && has_value_type<T>::value
	};
};

};
#endif //	object_type_traits_h_included
