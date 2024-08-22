#pragma once
#include <string>

// string(char)
using xr_string = std::basic_string<char, std::char_traits<char>, std::allocator<char>>;

IC void xr_strlwr(xr_string& src)
{
	for (auto& it : src)
		it = xr_string::value_type(tolower(it));
}

IC bool IsUTF8(const char* string)
{
	if (!string)
		return true;

	const unsigned char* bytes = (const unsigned char*)string;
	int num;
	while (*bytes != 0x00)
	{
		if ((*bytes & 0x80) == 0x00)
		{
			// U+0000 to U+007F
			num = 1;
		}
		else if ((*bytes & 0xE0) == 0xC0)
		{
			// U+0080 to U+07FF
			num = 2;
		}
		else if ((*bytes & 0xF0) == 0xE0)
		{
			// U+0800 to U+FFFF
			num = 3;
		}
		else if ((*bytes & 0xF8) == 0xF0)
		{
			// U+10000 to U+10FFFF
			num = 4;
		}
		else
			return false;

		bytes += 1;
		for (int i = 1; i < num; ++i)
		{
			if ((*bytes & 0xC0) != 0x80)
				return false;
			bytes += 1;
		}
	}
	return true;
}

IC wchar_t* ANSI_TO_TCHAR(const char* C)
{
	int len = (int)strlen(C);
	static thread_local wchar_t WName[4096];
	RtlZeroMemory(&WName, sizeof(WName));

	// Converts the path to wide characters
	[[maybe_unused]] int needed = MultiByteToWideChar(CP_UTF8, 0, C, len + 1, WName, len + 1);
	return WName;
}

IC xr_string ANSI_TO_UTF8(const xr_string& ansi)
{
	wchar_t* wcs = nullptr;
	int need_length = MultiByteToWideChar(1251, 0, ansi.c_str(), (int)ansi.size(), wcs, 0);
	wcs = new wchar_t[need_length + 1];
	MultiByteToWideChar(1251, 0, ansi.c_str(), (int)ansi.size(), wcs, need_length);
	wcs[need_length] = L'\0';

	char* u8s = nullptr;
	need_length = WideCharToMultiByte(CP_UTF8, 0, wcs, (int)std::wcslen(wcs), u8s, 0, nullptr, nullptr);
	u8s = new char[need_length + 1];
	WideCharToMultiByte(CP_UTF8, 0, wcs, (int)std::wcslen(wcs), u8s, need_length, nullptr, nullptr);
	u8s[need_length] = '\0';

	xr_string result(u8s);
	delete[] wcs;
	delete[] u8s;
	return result;
}