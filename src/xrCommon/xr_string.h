#pragma once
#include <string>

// string(char)
using xr_string = std::basic_string<char, std::char_traits<char>, std::allocator<char>>;

inline void xr_strlwr(xr_string& src)
{
	for (auto& it : src)
		it = xr_string::value_type(tolower(it));
}