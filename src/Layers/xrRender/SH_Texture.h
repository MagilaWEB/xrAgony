#ifndef SH_TEXTURE_H
#define SH_TEXTURE_H
#pragma once

#include "xrCore/xr_resource.h"

class ENGINE_API CAviPlayerCustom;
class CTheoraSurface;

class ECORE_API CTexture : public xr_resource_named
{
public:
	enum	MaxTextures
	{
		//	Actually these values are 128
		mtMaxPixelShaderTextures = 16,
		mtMaxVertexShaderTextures = 4,
		mtMaxGeometryShaderTextures = 16,
		mtMaxHullShaderTextures = 16,
		mtMaxDomainShaderTextures = 16,
		mtMaxComputeShaderTextures = 16,
		mtMaxCombinedShaderTextures =
		mtMaxPixelShaderTextures
		+ mtMaxVertexShaderTextures
		+ mtMaxGeometryShaderTextures
		+ mtMaxHullShaderTextures
		+ mtMaxDomainShaderTextures
		+ mtMaxComputeShaderTextures
	};

	//	Since DX10 allows up to 128 unique textures,
	//	distance between enum values should be at leas 128
	enum ResourceShaderType //	Don't change this since it's hardware-dependent
	{
		rstPixel = 0,
		// Default texture offset
		rstVertex = D3DVERTEXTEXTURESAMPLER0,
		rstGeometry = rstVertex + 256,
		rstHull = rstGeometry + 256,
		rstDomain = rstHull + 256,
		rstCompute = rstDomain + 256,
		rstInvalid = rstCompute + 256
	};

public:
	void apply_load(u32 stage);
	void apply_theora(u32 stage);
	void apply_avi(u32 stage);
	void apply_seq(u32 stage);
	void apply_normal(u32 stage);

	void Preload();
	void Load();
	void PostLoad();
	void Unload(void);
	// void Apply(u32 dwStage);

	void surface_set(ID3DBaseTexture* surf);
	ID3DBaseTexture* surface_get();


	BOOL isUser() { return flags.bUser; }

	u32 get_Width()
	{
		desc_enshure();
		return m_width;
	}

	u32 get_Height()
	{
		desc_enshure();
		return m_height;
	}

	void video_Sync(u32 _time) { m_play_time = _time; }
	void video_Play(BOOL looped, u32 _time = 0xFFFFFFFF);
	void video_Pause(BOOL state);
	void video_Stop();
	BOOL video_IsPlaying();

	CTexture();
	virtual ~CTexture();

	ID3DShaderResourceView* get_SRView() { return m_pSRView; }

private:
	BOOL desc_valid() { return pSurface == desc_cache; }

	void desc_enshure()
	{
		if (!desc_valid())
			desc_update();
	}

	void desc_update();
	void Apply(u32 dwStage);
	void ProcessStaging();
	D3D_USAGE GetUsage();

	//	Class data
public: //	Public class members (must be encapsulated further)
	struct
	{
		bool bLoaded{ false };
		bool bUser{ false };
		bool bSeqCycles{ false };
		bool bLoadedAsStaging{ false };
		size_t MemoryUsage{ 0 };
	} flags;

	fastdelegate::FastDelegate<void(u32)> bind;

	CAviPlayerCustom* pAVI;
	CTheoraSurface* pTheora;
	float m_material;
	shared_str m_bumpmap;

	union
	{
		u32 m_play_time; // sync theora time
		u32 seqMSPF; // Sequence data milliseconds per frame
	};

private:
	ID3DBaseTexture* pSurface;
	// Sequence data
	xr_vector<ID3DBaseTexture*> seqDATA;

	// Description
	u32 m_width;
	u32 m_height;
	ID3DBaseTexture* desc_cache;
	D3D_TEXTURE2D_DESC desc;

	ID3DShaderResourceView* m_pSRView;
	// Sequence view data
	xr_vector<ID3DShaderResourceView*> m_seqSRView;
};

struct resptrcode_texture : public resptr_base<CTexture>
{
	void create(LPCSTR _name);
	void destroy() { _set(nullptr); }
	shared_str bump_get() { return _get()->m_bumpmap; }
	bool bump_exist() { return 0 != bump_get().size(); }
};

typedef resptr_core<CTexture, resptrcode_texture> ref_texture;

#endif
