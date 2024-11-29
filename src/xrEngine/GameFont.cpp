#include "stdafx.h"
#pragma hdrstop

#include "GameFont.h"
#include "xrCore/Text/MbHelpers.h"
#include "Render.h"

#include "Include/xrRender/RenderFactory.h"
#include "Include/xrRender/FontRender.h"

#include <freetype/freetype.h>

#pragma comment(lib, "freetype.lib")

constexpr u32 TextureDimension = 2048 * 2;

constexpr std::pair<pcstr, u32> list_flags_string[]
{
	{ "fsNone",					CGameFont::fsNone },
	{ "fsGradient",				CGameFont::fsGradient },
	{ "fsDeviceIndependent",	CGameFont::fsDeviceIndependent },
	{ "fsValid",				CGameFont::fsValid },
	{ "fsMultibyte",			CGameFont::fsMultibyte },
	{ "fsForceDWORD",			CGameFont::fsForceDWORD }
};

#define DI2PX(x) float(iFloor((x + 1) * float(::Render->getTarget()->get_width()) / 2))
#define DI2PY(y) float(iFloor((y + 1) * float(::Render->getTarget()->get_height()) / 2))

FT_Library FreetypeLib{ nullptr };
FT_Face OurFont{ nullptr };
size_t* FontBitmap{ nullptr };

CGameFont::CGameFont(pcstr section, size_t flags, u16 size) : pSection(section)
{

	if (flags == fsNone)
	{
		if (pSettings->line_exist(section, "flags"))
		{
			const auto flags = split(pSettings->r_string(section, "flags"), '|');
			for (auto& name_flag : flags)
				for (auto& t : list_flags_string)
					if (!xr_strcmp(name_flag.c_str(), t.first))
						uFlags |= t.second;
		}
	}
	else
		uFlags = flags;
	
	// Read font name
	if (pSettings->line_exist(section, "name"))
		Data.Name = pSettings->r_string(section, "name");

	// Set font shader and style
	Data.Shader = pSettings->r_string(section, "shader");

	Data.File = pSettings->r_string(section, "file");

	/*if (pSettings->line_exist(section, "style"))
		Data.Style = pSettings->r_string(section, "style");*/

	if (pSettings->line_exist(section, "size"))
		Data.Size = pSettings->r_u16(section, "size");

	Size = size != 0 ? size : Data.Size;

	if (pSettings->line_exist(section, "opentype"))
		Data.OpenType = pSettings->r_bool(section, "opentype");

	if (pSettings->line_exist(section, "letter_spacing"))
		LetterSpacing = pSettings->r_float(section, "letter_spacing");

	if (pSettings->line_exist(section, "line_spacing"))
		LineSpacing = pSettings->r_float(section, "line_spacing");

	InitFile();
	Initialize(Data.Name, Data.Shader, nullptr);//  Data.Style
}

CGameFont::~CGameFont()
{
	// Shading
	::RenderFactory->DestroyFontRender(pFontRender);
	pFontRender = nullptr;
	GlyphData.clear();
}

void CGameFont::InitFile()
{
	FT_Error Error = FT_Init_FreeType(&FreetypeLib);
	R_ASSERT2(Error == 0, "Freetype2 initialize failed");

	string256 NameWithExt;
	xr_sprintf(NameWithExt, "%s\\%s.ttf", "all", Data.File);

	FS.update_path(Data.FileFullPath, _game_fonts_, NameWithExt);

	IReader* FontFile = FS.r_open(Data.FileFullPath);
	if (FontFile == nullptr)
	{
		xr_sprintf(NameWithExt, "%s\\%s.ttf", "rus", Data.File);
		FS.update_path(Data.FileFullPath, _game_fonts_, NameWithExt);
	}

	FontFile = FS.r_open(Data.FileFullPath);
	if (FontFile == nullptr)
	{
		Msg("! Can't open font file [%s].", Data.FileFullPath);

		xr_sprintf(NameWithExt, "%s\\arial.ttf", "all");
		FS.update_path(Data.FileFullPath, _game_fonts_, NameWithExt);
	}

	FontFile = FS.r_open(Data.FileFullPath);
	if (FontFile == nullptr)
		FATAL_F("The font could not be found [%s]", Data.FileFullPath);

	FT_Error FTError = FT_New_Memory_Face(FreetypeLib, (FT_Byte*)FontFile->pointer(), FontFile->length(), 0, &OurFont);
	R_ASSERT3(FTError == 0, "FT_New_Memory_Face return error", Data.FileFullPath);
}

void CGameFont::ReInit()
{
	InitFile();
	Initialize(Data.Name, Data.Shader, nullptr); // Data.Style
}

void CGameFont::Initialize(pcstr name, pcstr shader, pcstr style)
{
	if (pFontRender)
	{
		::RenderFactory->DestroyFontRender(pFontRender);
		pFontRender = nullptr;
	}
	pFontRender = ::RenderFactory->CreateFontRender();

	/*if (style != nullptr)
	{
		xr_string StyleDesc(style);
		xr_vector<xr_string> StyleTokens = split(StyleDesc, '|');
		for (const xr_string& token : StyleTokens)
		{
			if (token == "bold")
			{
				Style.bold = 1;
			}
			else if (token == "italic")
			{
				Style.italic = 1;
			}
			else if (token == "underline")
			{
				Style.underline = 1;
			}
			else if (token == "strike")
			{
				Style.strike = 1;
			}
		}
	}*/

	//CStringTable::LangName();
	// ? CStringTable::LangName()

	Ivector2 Target{ 0, 0 }, Target2{0, 0};

	FT_Set_Pixel_Sizes(OurFont, 0, Data.Size);

#define FT_CEIL(X)  (X / 64)

	fCurrentHeight = (float)FT_CEIL(OurFont->size->metrics.height);

	FontBitmap = new size_t[TextureDimension * TextureDimension];

	//const char* Format = FT_Get_Font_Format(OurFont);

	size_t index = 0;
	auto glyphID = FT_Get_First_Char(OurFont, &index);
	while (index != 0)
	{
		size_t TrueGlyph = glyphID;
		if (Data.OpenType)
			TrueGlyph = TranslateSymbolUsingCP1251((const char)glyphID);

		FT_Error FTError = FT_Load_Char(OurFont, TrueGlyph, FT_LOAD_RENDER);
		R_ASSERT3(FTError == 0, "FT_Load_Glyph return error", pSection);

		Target2.x = Target.x + OurFont->glyph->bitmap.width;
		if (Target2.x >= TextureDimension)
		{
			Target.x = 0;
			Target2.x = OurFont->glyph->bitmap.width;
			Target.y += size_t(fCurrentHeight);

			R_ASSERT2(Target.y <= TextureDimension, "Font too large, or dimension texture is too small");
		}

		size_t SourceX = 0;
		size_t SourceY = 0;

		Target2.y = Target.y + size_t(fCurrentHeight);
		size_t TargetYSaved = Target.y;
		Target.y = Target.y + size_t(fCurrentHeight - (float)OurFont->glyph->bitmap.rows);

		if (OurFont->glyph->bitmap.pixel_mode == FT_PIXEL_MODE_GRAY)
		{
			for (size_t y = Target.y; y < Target2.y; y++, SourceY++)
			{
				for (size_t x = Target.x; x < Target2.x; x++, SourceX++)
				{
					size_t SourcePixel = OurFont->glyph->bitmap.buffer[(SourceY * OurFont->glyph->bitmap.pitch) + SourceX];

					size_t FinalPixel = SourcePixel;
					FinalPixel |= (SourcePixel << 8);
					FinalPixel |= (SourcePixel << 16);
					FinalPixel |= (SourcePixel << 24);

					FontBitmap[(y * TextureDimension) + x] = FinalPixel;
				}
				SourceX = 0;
			}
		}
		else if (OurFont->glyph->bitmap.pixel_mode == FT_PIXEL_MODE_BGRA)
		{
			for (size_t y = Target.y; y < Target2.y; y++, SourceY++)
			{
				for (size_t x = Target.x; x < Target2.x; x++, SourceX++)
				{
					size_t SourcePixel = OurFont->glyph->bitmap.buffer[(SourceY * OurFont->glyph->bitmap.pitch) + SourceX];

					u8 Alpha = (SourcePixel & 0x000000FF);
					u8 Red = (SourcePixel & 0x0000FF00) >> 8;
					u8 Green = (SourcePixel & 0x00FF0000) >> 16;
					u8 Blue = (SourcePixel & 0xFF000000) >> 24;

					size_t FinalPixel = Alpha;
					FinalPixel |= (size_t(Blue) << 8);
					FinalPixel |= (size_t(Green) << 16);
					FinalPixel |= (size_t(Red) << 24);

					FontBitmap[(y * TextureDimension) + x] = FinalPixel;
				}
				SourceX = 0;
			}
		}

		Target.y = TargetYSaved;

		RECT region;
		region.left = Target.x;
		region.right = Target.x + OurFont->glyph->bitmap.width;
		region.top = Target.y + 1;
		region.bottom = long(Target.y + fCurrentHeight);

		ABC widths;
		widths.abcA = FT_CEIL(OurFont->glyph->metrics.horiBearingX);
		widths.abcB = OurFont->glyph->bitmap.width;
		widths.abcC = FT_CEIL(OurFont->glyph->metrics.horiAdvance) - widths.abcB - widths.abcA;

		float GlyphTopScanlineOffset = fCurrentHeight - float(OurFont->glyph->bitmap.rows);
		float yOffset = float(-OurFont->glyph->bitmap_top) - GlyphTopScanlineOffset;
		yOffset += fCurrentHeight; // Return back to the center pos
		yOffset -= fCurrentHeight / 4;

		GlyphData.emplace(glyphID, new Glyph{ region, widths, yOffset });

		Target.x = Target2.x;
		Target.x += 4;

		glyphID = FT_Get_Next_Char(OurFont, glyphID, &index);
	}

	string128 textureName;
	xr_sprintf(textureName, "$user$%s", pSection); //#TODO optimize

	pFontRender->CreateFontAtlas(TextureDimension, Target2.y, textureName, FontBitmap);
	pFontRender->Initialize(shader, textureName);
	FT_Done_Face(OurFont);
	FT_Done_FreeType(FreetypeLib);

	if (FontBitmap)
		xr_delete(FontBitmap);

	Renders(true);
}

void CGameFont::OutSet(float x, float y)
{
	fCurrentX = x;
	fCurrentY = y;
}

void CGameFont::OutSetI(float x, float y)
{
	OutSet(DI2PX(x), DI2PY(y));
}

void CGameFont::OnRender()
{
	pFontRender->OnRender(*this);
	strings.clear();
}

//u16 CGameFont::GetCutLengthPos(float fTargetWidth, pcstr pszText)
//{
//	return 0;
//}

//u16 CGameFont::SplitByWidth(u16* puBuffer, u16 uBufferSize, float fTargetWidth, pcstr pszText)
//{
//	return 0;
//}

void CGameFont::MasterOut(bool bCheckDevice, bool bUseCoords, bool bScaleCoords, bool bUseSkip, float _x, float _y,
	float _skip, pcstr fmt, va_list p)
{
	if (bCheckDevice && (!::IDevice->cast()->b_is_Active.load()))
		return;

	String rs;

	rs.x = (bUseCoords ? (bScaleCoords ? (DI2PX(_x)) : _x) : fCurrentX);
	rs.y = (bUseCoords ? (bScaleCoords ? (DI2PY(_y)) : _y) : fCurrentY);
	rs.c = dwCurrentColor;
	rs.height = fCurrentHeight;
	rs.align = eCurrentAlignment;
	const int vs_sz = _vsnprintf(rs.string, sizeof(rs.string), fmt, p);
	// VERIFY( ( vs_sz != -1 ) && ( rs.string[ vs_sz ] == '\0' ) );

	if (!IsUTF8(rs.string))
		rs.string_utf8 = ANSI_TO_UTF8(rs.string);

	rs.string[sizeof(rs.string) - 1] = 0;
	if (vs_sz == -1)
		return;

	if (vs_sz)
		strings.push_back(rs);

	if (bUseSkip)
		OutSkip(_skip);
}

#define MASTER_OUT(CHECK_DEVICE, USE_COORDS, SCALE_COORDS, USE_SKIP, X, Y, SKIP)	\
va_list p;																			\
va_start(p, fmt);																	\
MasterOut(CHECK_DEVICE, USE_COORDS, SCALE_COORDS, USE_SKIP, X, Y, SKIP, fmt, p);	\
va_end(p);																			\

void __cdecl CGameFont::OutI(float _x, float _y, pcstr fmt, ...)
{
	MASTER_OUT(false, true, true, false, _x, _y, 0.0f);
}

void __cdecl CGameFont::Out(float _x, float _y, pcstr fmt, ...)
{
	MASTER_OUT(true, true, false, false, _x, _y, 0.0f);
}

void __cdecl CGameFont::OutNext(pcstr fmt, ...) {
	MASTER_OUT(TRUE, FALSE, FALSE, TRUE, 0.0f, 0.0f, 1.0f);
}

void CGameFont::OutSkip(float val)
{
	fCurrentY += val * GetHeight();
}

float CGameFont::SizeOf_(int cChar)
{
	if (cChar < 0)
		cChar = u8(cChar);

	return static_cast<float>(WidthOf(cChar)) * SizeCoof();
}

float CGameFont::SizeOf_(pcstr s)
{
	return static_cast<float> (WidthOf(s)) * SizeCoof();
}

//float CGameFont::SizeOf_(const wchar_t* wsStr)
//{
//	return 0;
//}

float CGameFont::SizeCoof() const
{
	extern float r_font_scale;
	return ((float)Size / Data.Size) * ::IDevice->cast()->screen_magnitude * r_font_scale;
}

void CGameFont::SetSize(u16 S)
{
	if (Size != S)
	{
		Size = S;
		//ReInit();
	}
}

void CGameFont::SetHeight(float S)
{
	if (uFlags & fsDeviceIndependent)
		fCurrentHeight = S;
};

const CGameFont::Glyph* CGameFont::GetGlyphInfo(int ch)
{
	auto it_Glyph = GlyphData.find(ch);
	if(it_Glyph != GlyphData.end())
		return (*it_Glyph).second;

	return nullptr;
}

size_t CGameFont::WidthOf(int ch)
{
	const Glyph* glyphInfo = GetGlyphInfo(ch);
	return glyphInfo ? glyphInfo->Abc.abcA + glyphInfo->Abc.abcB + glyphInfo->Abc.abcC : Data.Size / 2;
}

size_t CGameFont::WidthOf(pcstr str)
{
	if (!str || !str[0])
		return 0;

	size_t size = 0;

	if (IsUTF8(str))
	{
		auto asda = ANSI_TO_TCHAR(str);
		size_t length = std::wcslen(asda);
		for (size_t i = 0; i < length; i++)
			size += WidthOf(asda[i]);
	}
	else
	{
		size_t length = xr_strlen(str);
		for (size_t i = 0; i < length; i++)
			size += WidthOf((u8)str[i]);
	}

	return size;
}