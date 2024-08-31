#ifndef dxFontRender_included
#define dxFontRender_included
#pragma once

#include "Include/xrRender/FontRender.h"

class dxFontRender : public IFontRender
{
public:
	dxFontRender() = default;
	virtual ~dxFontRender();

	virtual void Initialize(LPCSTR cShader, LPCSTR cTexture) override;
	virtual void OnRender(CGameFont& owner) override;

	virtual void CreateFontAtlas(u32 width, u32 height, pcstr name, void* bitmap) override;

private:
	ref_shader pShader;
	ref_geom pGeom;
	ref_texture pTexture;
};

#endif //	FontRender_included
