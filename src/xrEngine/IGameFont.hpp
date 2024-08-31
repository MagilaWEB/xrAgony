#pragma once
#include <stdarg.h>
#include "xrCore/_types.h"
#include "xrCore/_vector2.h"

class IGameFont
{
	friend class dxFontRender;

	/*enum EStyle : u64
	{
		eBold = 4196692,
		eStrike = 4196725,
		eUnderline = 4196715,
		eItalic = 4196708
	};*/

public:
	enum EAligment
	{
		alLeft = 0,
		alRight,
		alCenter
	};

	enum
	{
		fsNone = 0,
		fsGradient = (1 << 0),
		fsDeviceIndependent = (1 << 1),
		fsValid = (1 << 2),
		fsMultibyte = (1 << 3),
		fsForceDWORD = u32(-1)
	};

	/*struct Style
	{
		u32 bold : 1;
		u32 italic : 1;
		u32 underline : 1;
		u32 strike : 1;
	};*/

public:
	virtual ~IGameFont() = 0;

	virtual void Initialize(pcstr name, pcstr shader, pcstr style) = 0;
	virtual void SetColor(size_t C) = 0;
	virtual size_t GetColor() const = 0;
	virtual void SetSize(u16 S) = 0;
	virtual u16 GetSize() const = 0;
	virtual void SetHeight(float S) = 0;
	virtual float GetHeight() const = 0;
	virtual void SetAligment(EAligment aligment) = 0;
	virtual float SizeOf_(pcstr s) = 0;
//	virtual float SizeOf_(const wchar_t* wsStr) = 0;
	virtual float SizeOf_(int cChar) = 0; // only ANSI
	virtual float SizeCoof() const = 0;
	virtual void OutSetI(float x, float y) = 0;
	virtual void OutSet(float x, float y) = 0;
	virtual Fvector2 GetPosition() const = 0;

	virtual void MasterOut(
		bool bCheckDevice,
		bool bUseCoords,
		bool bScaleCoords,
		bool bUseSkip,
		float _x, float _y, float _skip, pcstr fmt, va_list p) = 0;

	virtual BOOL IsMultibyte() const = 0;
	//virtual u16 SplitByWidth(u16* puBuffer, u16 uBufferSize, float fTargetWidth, pcstr pszText) = 0;
	//virtual u16 GetCutLengthPos(float fTargetWidth, pcstr pszText) = 0;
	virtual void OutI(float _x, float _y, pcstr fmt, ...) = 0;
	virtual void Out(float _x, float _y, pcstr fmt, ...) = 0;
	virtual void OutNext(pcstr fmt, ...) = 0;
	virtual void OutSkip(float val = 1.f) = 0;
	virtual void OnRender() = 0;
	virtual void Clear() = 0;

	virtual size_t WidthOf(int ch) = 0;
	virtual size_t WidthOf(pcstr str) = 0;
};

inline IGameFont::~IGameFont() {}
