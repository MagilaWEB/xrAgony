#pragma once
#define MAX_MB_CHARS 4096

constexpr wchar_t CP1251ConvertationTable[]
{
	0x0402, // Ä
	0x0403, // Å
	0x201A, // Ç
	0x0453, // É
	0x201E, // Ñ
	0x2026, // Ö
	0x2020, // Ü
	0x2021, // á
	0x20AC, // à
	0x2030, // â
	0x0409, // ä
	0x2039, // ã
	0x040A, // å
	0x040C, // ç
	0x040B, // é
	0x040F, // è

	0x0452, // ê
	0x2018, // ë
	0x2019, // í
	0x201C, // ì
	0x201D, // î
	0x2022, // ï
	0x2013, // ñ
	0x2014, // ó
	0x0,    // empty (0x98)
	0x2122, // ô
	0x0459, // ö
	0x203A, // õ
	0x045A, // ú
	0x045C, // ù
	0x045B, // û
	0x045F, // ü

	0x00A0, //  
	0x040E, // °
	0x045E, // ¢
	0x0408, // £
	0x00A4, // §
	0x0490, // •
	0x00A6, // ¶
	0x00A7, // ß
	0x0401, // ®
	0x00A9, // ©
	0x0404, // ™
	0x00AB, // ´
	0x00AC, // ¨
	0x00AD, // 
	0x00AE, // Æ
	0x0407, // Ø

	0x00B0, // ∞
	0x00B1, // ±
	0x0406, // ≤
	0x0456, // ≥
	0x0491, // ¥
	0x00B5, // µ
	0x00B6, // ∂
	0x00B7, // ∑
	0x0451, // ∏
	0x2116, // π
	0x0454, // ∫
	0x00BB, // ª
	0x0458, // º
	0x0405, // Ω
	0x0455, // æ
	0x0457, // ø
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
