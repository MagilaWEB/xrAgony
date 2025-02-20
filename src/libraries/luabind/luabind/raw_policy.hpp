// Copyright (c) 2003 Daniel Wallin and Arvid Norberg

// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense,
// and/or sell copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included
// in all copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF
// ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED
// TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
// PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT
// SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR
// ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
// ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE
// OR OTHER DEALINGS IN THE SOFTWARE.


#ifndef LUABIND_RAW_POLICY_HPP_INCLUDED
#define LUABIND_RAW_POLICY_HPP_INCLUDED

#include <luabind/config.hpp>
#include <luabind/detail/policy.hpp>

namespace luabind {
	namespace detail {

		struct raw_converter
		{
			enum { consumed_args = 0 };

			lua_State* to_cpp(lua_State* L, by_pointer<lua_State>, int)
			{
				return L;
			}

			static int match(...)
			{
				return 0;
			}

			void converter_postcall(lua_State*, by_pointer<lua_State>, int) {}
		};

		struct raw_policy
		{
			template<class T, class Direction>
			struct specialize
			{
				using type = raw_converter;
			};
		};

	}
	
	namespace policy
	{
		template<unsigned int N>
		using raw = converter_policy_injector<N, detail::raw_policy>;
	}
} // namespace luabind

#endif // LUABIND_RAW_POLICY_HPP_INCLUDED

