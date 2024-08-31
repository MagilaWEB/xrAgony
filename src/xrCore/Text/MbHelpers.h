#pragma once
#define MAX_MB_CHARS 4096

constexpr wchar_t CP1251ConvertationTable[]
{
	0x0402, // �
	0x0403, // �
	0x201A, // �
	0x0453, // �
	0x201E, // �
	0x2026, // �
	0x2020, // �
	0x2021, // �
	0x20AC, // �
	0x2030, // �
	0x0409, // �
	0x2039, // �
	0x040A, // �
	0x040C, // �
	0x040B, // �
	0x040F, // �

	0x0452, // �
	0x2018, // �
	0x2019, // �
	0x201C, // �
	0x201D, // �
	0x2022, // �
	0x2013, // �
	0x2014, // �
	0x0,    // empty (0x98)
	0x2122, // �
	0x0459, // �
	0x203A, // �
	0x045A, // �
	0x045C, // �
	0x045B, // �
	0x045F, // �

	0x00A0, //  
	0x040E, // �
	0x045E, // �
	0x0408, // �
	0x00A4, // �
	0x0490, // �
	0x00A6, // �
	0x00A7, // �
	0x0401, // �
	0x00A9, // �
	0x0404, // �
	0x00AB, // �
	0x00AC, // �
	0x00AD, // 
	0x00AE, // �
	0x0407, // �

	0x00B0, // �
	0x00B1, // �
	0x0406, // �
	0x0456, // �
	0x0491, // �
	0x00B5, // �
	0x00B6, // �
	0x00B7, // �
	0x0451, // �
	0x2116, // �
	0x0454, // �
	0x00BB, // �
	0x0458, // �
	0x0405, // �
	0x0455, // �
	0x0457, // �
};

IC wchar_t TranslateSymbolUsingCP1251(char Symbol)
{
	unsigned char RawSymbol = *(unsigned char*)&Symbol;

	if (RawSymbol < 0x80)
		return wchar_t(RawSymbol);

	if (RawSymbol < 0xc0)
		return CP1251ConvertationTable[RawSymbol - 0x80];

	return wchar_t(RawSymbol - 0xc0) + 0x410;
}

IC xr_vector<xr_string> split(const xr_string& s, const char delim)
{
	xr_vector<xr_string> elems;
	std::stringstream ss(s);
	xr_string item;
	while (std::getline(ss, item, delim))
		elems.push_back(_Trim(item));

	return std::move(elems);
}
