#include "stdafx.h"
#include "dxFontRender.h"
#include "xrEngine/GameFont.h"
#include "xrCore/Text/MbHelpers.h"

dxFontRender::~dxFontRender()
{
	pShader.destroy();
	pGeom.destroy();
	pTexture.destroy();
}

void dxFontRender::Initialize(LPCSTR cShader, LPCSTR cTexture)
{
	pShader.create(cShader, cTexture);
	pGeom.create(FVF::F_TL, RCache.Vertex.Buffer(), RCache.QuadIB);

	if (pTexture._get() == nullptr)
		pTexture.create(cTexture);
}

extern ENGINE_API BOOL g_bRendering;
void dxFontRender::OnRender(CGameFont& owner)
{
	VERIFY(g_bRendering);

	if (pShader)
		RCache.set_Shader(pShader);

	const float fWidth = (float)pTexture->get_Width();
	const float fHeight = (float)pTexture->get_Height();

	//#TODO mb need use optimization for minimize vertexes allocations
	for (CGameFont::String& str : owner.strings)
	{
		const int length = xr_strlen(str.string);

		if (length)
		{
			// lock AGP memory
			u32	vOffset;
			FVF::TL* vertexes = (FVF::TL*)RCache.Vertex.Lock(length * 4, pGeom.stride(), vOffset);
			FVF::TL* start = vertexes;

			const float coof_size = owner.SizeCoof();
			const float correction_y = (1.f - coof_size) * owner.Size / 4;

			float X = str.x;
			float Y = str.y + correction_y;
			float Y2 = Y + str.height * coof_size;

			if (str.align)
			{
				const float width = owner.SizeOf_(str.string);

				switch (str.align)
				{
				case CGameFont::alCenter:
					X -= width / 2;
					break;
				case CGameFont::alRight:
					X -= width;
					break;
				}
			}

			u32	clr, clr2;
			clr2 = clr = str.c;
			if (owner.uFlags & CGameFont::fsGradient)
			{
				u32	_R = color_get_R(clr) / 2;
				u32	_G = color_get_G(clr) / 2;
				u32	_B = color_get_B(clr) / 2;
				u32	_A = color_get_A(clr);
				clr2 = color_rgba(_R, _G, _B, _A);
			}

			wchar_t* UniStr = nullptr;

			if (IsUTF8(str.string))
				UniStr = ANSI_TO_TCHAR(str.string);
			else
				UniStr = ANSI_TO_TCHAR(str.string_utf8.c_str());

			for (int i = 0; i < length; i++)
			{
				CGameFont::Glyph* glyphInfo = nullptr;

				glyphInfo = const_cast<CGameFont::Glyph*>(owner.GetGlyphInfo((u8)str.string[i]));

				if (glyphInfo == nullptr)
				{
					glyphInfo = const_cast<CGameFont::Glyph*>(owner.GetGlyphInfo(UniStr[i]));
					if (glyphInfo == nullptr)
						continue;
				}

				if (i != 0)
					X += glyphInfo->Abc.abcA * coof_size;

				const float yOffset = glyphInfo->yOffset * coof_size;
				const float GlyphY = Y + yOffset;
				const float GlyphY2 = Y2 + yOffset;

				const float X2 = X + glyphInfo->Abc.abcB * coof_size;

				const float u1 = float(glyphInfo->TextureCoord.left) / fWidth;
				const float u2 = float(glyphInfo->TextureCoord.right) / fWidth;

				const float v1 = float(glyphInfo->TextureCoord.top) / fHeight;
				const float v2 = float(glyphInfo->TextureCoord.bottom) / fHeight ;

				vertexes->set(X, GlyphY2, clr2, u1, v2);
				++vertexes;
				vertexes->set(X, GlyphY, clr, u1, v1);
				++vertexes;
				vertexes->set(X2, GlyphY2, clr2, u2, v2);
				++vertexes;
				vertexes->set(X2, GlyphY, clr, u2, v1);
				++vertexes;

				X = X2 + glyphInfo->Abc.abcC * coof_size;
			}

			// Unlock and draw
			const u32 vertexesCount = (u32)(vertexes - start);
			RCache.Vertex.Unlock(vertexesCount, pGeom.stride());

			if (vertexesCount > 0)
			{
				RCache.set_Geometry(pGeom);
				RCache.Render(D3DPT_TRIANGLELIST, vOffset, 0, vertexesCount, 0, vertexesCount / 2);
			}
		}
	}
}

void dxFontRender::CreateFontAtlas(u32 width, u32 height, pcstr name, void* bitmap)
{
	ID3DTexture2D* pSurface = nullptr;
	D3D_TEXTURE2D_DESC descFontAtlas;
	ZeroMemory(&descFontAtlas, sizeof(D3D_TEXTURE2D_DESC));
	descFontAtlas.Width = width;
	descFontAtlas.Height = height;
	descFontAtlas.MipLevels = 1;
	descFontAtlas.ArraySize = 1;
	descFontAtlas.SampleDesc.Count = 1;
	descFontAtlas.SampleDesc.Quality = 0;
	descFontAtlas.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
	descFontAtlas.Usage = D3D_USAGE_DEFAULT;
	descFontAtlas.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	descFontAtlas.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	descFontAtlas.MiscFlags = 0;

	D3D_SUBRESOURCE_DATA FontData;
	FontData.pSysMem = bitmap;
	FontData.SysMemSlicePitch = 0;
	FontData.SysMemPitch = width * 4;

	if (HW.pDevice->CreateTexture2D(&descFontAtlas, &FontData, &pSurface) != S_OK)
	{
		Msg("! D3D_USAGE_DEFAULT may not be working");
		_RELEASE(pSurface);
		descFontAtlas.Usage = D3D_USAGE_DYNAMIC;
		R_CHK(HW.pDevice->CreateTexture2D(&descFontAtlas, &FontData, &pSurface));
	}

	pTexture.create(name);
	pTexture->surface_set(pSurface);

	_RELEASE(pSurface);
}