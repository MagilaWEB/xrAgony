#pragma once

#include "xrEngine/IGameFont.hpp"
#include "xrCommon/xr_vector.h"

class ENGINE_API CGameFont : public IGameFont
{
	friend class dxFontRender;
	friend class IFontRender;

	struct String
	{
		string4096 string;
		xr_string string_utf8;
		float x, y;
		float height{ 0.f };
		size_t c;
		EAligment align;
	};

	struct BaseData
	{
		bool OpenType = false;
		pcstr Name{ nullptr };
		pcstr Shader{ nullptr };
		//pcstr Style{ nullptr };
		pcstr File{ nullptr };
		string_path FileFullPath;
		u16 Size{ 14 };
	};

	struct Glyph
	{
		RECT TextureCoord;
		ABC Abc;
		float yOffset;
	};

	pcstr pSection{ nullptr };					//#TODO change type
	float LetterSpacing{ 0.f };					//that must be in CUIText from new font system
	float LineSpacing{ 0.f };					//that must be in CUIText from new font system

	u16 Size{ 14 };
	xr_map<int, Glyph*> GlyphData;
	bool is_renders{ false };
	
protected:
	//Style Style;
	BaseData Data;

	EAligment eCurrentAlignment;
	size_t dwCurrentColor;
	float fCurrentHeight{ 0.f };
	float fCurrentX, fCurrentY;
	xr_vector<String> strings;

	IFontRender* pFontRender{ nullptr };
	size_t uFlags{ fsNone };

public:
	CGameFont(pcstr section, size_t flags = fsNone, u16 size = 0);
	virtual ~CGameFont();

	void InitFile();
	void ReInit();

	IC pcstr GetSection() const { return pSection; };
	IC size_t GetFlags() const { return uFlags; };
	IC void Renders(bool renders)
	{
		is_renders = renders;
	};
	IC bool IsRenders() const
	{
		return is_renders;
	};
	virtual void Initialize(pcstr name, pcstr shader, pcstr style) override;
	virtual void SetColor(size_t C) override { dwCurrentColor = C; }
	virtual size_t GetColor() const override { return dwCurrentColor; }
	virtual void SetSize(u16 S) override;
	IC virtual u16 GetSize() const override { return Size; };
	virtual void SetHeight(float S) override;
	const Glyph* GetGlyphInfo(int ch);
	// returns symbol width in pixels
	virtual size_t WidthOf(int ch) override;
	virtual size_t WidthOf(pcstr str) override;
	virtual float GetHeight() const override { return fCurrentHeight * SizeCoof(); };
	virtual void SetAligment(EAligment aligment) override { eCurrentAlignment = aligment; }
	virtual float SizeOf_(pcstr s) override;
	//virtual float SizeOf_(const wchar_t* wsStr) override;
	virtual float SizeOf_(int cChar) override; // only ANSII
	virtual float SizeCoof() const override;
	virtual void OutSetI(float x, float y) override;
	virtual void OutSet(float x, float y) override;
	virtual Fvector2 GetPosition() const override { return { fCurrentX, fCurrentY }; }

	virtual void MasterOut(
		bool bCheckDevice,
		bool bUseCoords,
		bool bScaleCoords,
		bool bUseSkip,
		float _x, float _y, float _skip, pcstr fmt, va_list p) override;

	virtual BOOL IsMultibyte() const override { return uFlags & fsMultibyte; };
	//virtual u16 SplitByWidth(u16* puBuffer, u16 uBufferSize, float fTargetWidth, pcstr pszText) override;
	//virtual u16 GetCutLengthPos(float fTargetWidth, pcstr pszText) override;
	virtual void OutI(float _x, float _y, pcstr fmt, ...) override;
	virtual void Out(float _x, float _y, pcstr fmt, ...) override;
	virtual void OutNext(pcstr fmt, ...) override;
	virtual void OutSkip(float val = 1.f) override;
	virtual void OnRender() override;
	virtual void Clear() override { strings.clear(); }
};
