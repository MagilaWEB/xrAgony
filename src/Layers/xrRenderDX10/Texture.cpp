#include "stdafx.h"
#pragma hdrstop

void fix_texture_name(LPSTR fn)
{
	LPSTR _ext = strext(fn);
	if (_ext && (!xr_stricmp(_ext, ".tga") || !xr_stricmp(_ext, ".dds") || !xr_stricmp(_ext, ".bmp") || !xr_stricmp(_ext, ".ogm")))
	{
		*_ext = 0;
	}
}

static int get_texture_load_lod(LPCSTR fn)
{
	ENGINE_API bool is_enough_address_space_available();
	static bool enough_address_space_available = is_enough_address_space_available();
	if (psTextureLOD < 1)
		return enough_address_space_available ? 0 : 1;

	return psTextureLOD;
}

static u32 calc_texture_size(int lod, u32 mip_cnt, u32 orig_size)
{
	if (1 == mip_cnt)
		return orig_size;

	int _lod = lod;
	float res = float(orig_size);

	while (_lod > 0)
	{
		--_lod;
		res -= res / 1.333f;
	}
	return iFloor(res);
}

//////////////////////////////////////////////////////////////////////
// Utility pack
//////////////////////////////////////////////////////////////////////
static u32 GetPowerOf2Plus1(u32 v)
{
	u32 cnt = 0;
	while (v)
	{
		v >>= 1;
		cnt++;
	};
	return cnt;
}

static void Reduce(int& w, int& h, int& l, int& skip)
{
	while ((l > 1) && skip)
	{
		w /= 2;
		h /= 2;
		l -= 1;

		skip--;
	}
	if (w < 1)
		w = 1;
	if (h < 1)
		h = 1;
}

static void Reduce(size_t& w, size_t& h, size_t& l, int skip)
{
	while ((l > 1) && skip)
	{
		w /= 2;
		h /= 2;
		l -= 1;

		skip--;
	}
	if (w < 1)
		w = 1;
	if (h < 1)
		h = 1;
}

ID3DBaseTexture* CRender::texture_load(LPCSTR fRName, u32& ret_msize)
{
	//  Moved here just to avoid warning
	DirectX::TexMetadata IMG;
	DirectX::ScratchImage texture;

	ID3DBaseTexture* pTexture2D = nullptr;
	// IDirect3DCubeTexture9*	pTextureCUBE	= nullptr;
	string_path fn;
	// u32					  dwWidth,dwHeight;
	u32 img_size = 0;
	int img_loaded_lod = 0;
	// D3DFORMAT				fmt;
	u32 mip_cnt = u32(-1);
	// validation
	R_ASSERT(fRName);
	R_ASSERT(fRName[0]);

	// make file name
	string_path fname;
	xr_strcpy(fname, fRName); //. andy if (strext(fname)) *strext(fname)=0;
	fix_texture_name(fname);
	IReader* S = nullptr;
	if (!FS.exist(fn, "$game_textures$", fname, ".dds") && strstr(fname, "_bump"))
		goto _BUMP_from_base;
	if (FS.exist(fn, "$level$", fname, ".dds"))
		goto _DDS;
	if (FS.exist(fn, "$game_saves$", fname, ".dds"))
		goto _DDS;
	if (FS.exist(fn, "$game_textures$", fname, ".dds"))
		goto _DDS;

#ifdef _EDITOR
	ELog.Msg(mtError, "Can't find texture '%s'", fname);
	return 0;
#else

	Msg("! Can't find texture '%s'", fname);
	R_ASSERT(FS.exist(fn, "$game_textures$", "ed\\ed_not_existing_texture", ".dds"));
	goto _DDS;

	//  xrDebug::Fatal(DEBUG_INFO,"Can't find texture '%s'",fname);

#endif

_DDS:
	{
		// Load and get header
		S = FS.r_open(fn);
		img_size = S->length();
#ifdef DEBUG
		Msg("* Loaded: %s[%zu]", fn, img_size);
#endif // DEBUG
		R_ASSERT(S);

		R_CHK2(LoadFromDDSMemory(S->pointer(), S->length(), DirectX::DDS_FLAGS_PERMISSIVE, &IMG, texture), fn);

		// Check for LMAP and compress if needed
		xr_strlwr(fn);

		img_loaded_lod = get_texture_load_lod(fn);

		size_t mip_lod = 0;
		if (img_loaded_lod && !IMG.IsCubemap())
		{
			const auto old_mipmap_cnt = IMG.mipLevels;
			Reduce(IMG.width, IMG.height, IMG.mipLevels, img_loaded_lod);
			mip_lod = old_mipmap_cnt - IMG.mipLevels;
		}

		// DirectX requires compressed texture size to be
		// a multiple of 4. Make sure to meet this requirement.
		if (DirectX::IsCompressed(IMG.format))
		{
			IMG.width = (IMG.width + 3u) & ~0x3u;
			IMG.height = (IMG.height + 3u) & ~0x3u;
		}

		R_CHK2(CreateTextureEx(HW.pDevice, texture.GetImages() + mip_lod, texture.GetImageCount(), IMG,
			D3D_USAGE_IMMUTABLE, D3D_BIND_SHADER_RESOURCE, 0, IMG.miscFlags, DirectX::CREATETEX_DEFAULT,
			&pTexture2D), fn
		);
		FS.r_close(S);

		// OK
		mip_cnt = IMG.mipLevels;
		ret_msize = calc_texture_size(img_loaded_lod, mip_cnt, img_size);
		return pTexture2D;
	}

_BUMP_from_base:
	{
		// Msg		  ("! auto-generated bump map: %s",fname);
		Msg("! Fallback to default bump map: %s", fname);
		//////////////////
		if (strstr(fname, "_bump#"))
		{
			R_ASSERT2(FS.exist(fn, "$game_textures$", "ed\\ed_dummy_bump#", ".dds"), "ed_dummy_bump#");
			S = FS.r_open(fn);
			R_ASSERT2(S, fn);
			img_size = S->length();
			goto _DDS;
		}
		if (strstr(fname, "_bump"))
		{
			R_ASSERT2(FS.exist(fn, "$game_textures$", "ed\\ed_dummy_bump", ".dds"), "ed_dummy_bump");
			S = FS.r_open(fn);

			R_ASSERT2(S, fn);

			img_size = S->length();
			goto _DDS;
		}
		//////////////////
	}

	return 0;
}
