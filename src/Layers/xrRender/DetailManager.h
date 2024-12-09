// DetailManager.h: interface for the CDetailManager class.
//
//////////////////////////////////////////////////////////////////////
#pragma once

#include "xrCore/xrPool.h"
#include "DetailFormat.h"
#include "DetailModel.h"

const int 		dm_cache1_count = 4;
const int		dm_max_objects = 64;
const int		dm_obj_in_slot = 4;
const float		dm_slot_size = DETAIL_SLOT_SIZE;


const u32 		dm_max_cache_size = 62001 * 2; // assuming max dm_size = 248
class ECORE_API CDetailManager final
{
public:
	enum VisiblesType
	{
		VISUAL,
		SHADOW_SUN,
		SHADOW_LIGHT
	};

	CDetailManager();
	virtual ~CDetailManager();

	void		Load();
	void		Unload();
	void		Frame();
	void		Render(VisiblesType type = VISUAL);
	void		detail_size();

private:
	int			dither[16][16];

	u32			dm_size = 24;
	u32 		dm_cache1_line = 12;		//dm_size*2/dm_cache1_count
	u32			dm_cache_line = 49;			//dm_size+1+dm_size
	u32			dm_cache_size = 2401;		//dm_cache_line*dm_cache_line
	float		dm_fade = 47.5;				//float(2*dm_size)-.5f;

	u32			dm_size_MAX;
	u32			dm_cache1_line_MAX;
	u32			dm_cache_line_MAX;
	u32			dm_cache_size_MAX;

	// swing values
	struct SSwingValue
	{
		float						rot1;
		float						rot2;
		float						amp1;
		float						amp2;
		float						speed;
		void						lerp(const SSwingValue& v1, const SSwingValue& v2, float factor);
	};

	struct SlotItem
	{								// один кустик
		float						scale;
		float						scale_random;
		Fmatrix						mRotY;
		Fmatrix						mRotYCache;
		bool						optimization;
		u32							vis_ID;					// индекс в visibility списке он же тип [не качается, качается1, качается2]
		float						c_hemi;
		float						c_sun;
		u32							sector_id;
		Fvector4					build_matrix[4];		// 0,1,2 Build matrix ( 3x4 matrix) 3 Build color
	};

	struct SlotPart
	{
		u32								id;					// ID модельки
		xr_c_vector<SlotItem*>	items;				// список кустиков
		//SlotItemVec					r_items[3];			// список кустиков for render
	};

	enum SlotType
	{
		stReady = 0,										// Ready to use
		stPending,											// Pending for decompression
		stFORCEDWORD = 0xffffffff
	};

	struct	Slot											// распакованый слот размером DETAIL_SLOT_SIZE
	{
		struct
		{
			u32						empty : 1;
			u32						type : 1;
		};
		int							sx, sz;					// координаты слота X x Y
		vis_data					vis;
		SlotPart					G[dm_obj_in_slot];

		IC Slot()
		{
			empty = 1;
			type = stReady;
			sx = sz = 0;
			vis.clear();
		}
	};

	struct 	CacheSlot1
	{
		u32							empty;
		vis_data 					vis;
		Slot**						slots[dm_cache1_count * dm_cache1_count];
		IC CacheSlot1()
		{
			empty = 1;
			vis.clear();
		}
	};

	typedef	xr_c_vector<xr_c_vector <SlotItem* > >	vis_list;
	typedef	svector<CDetail*, dm_max_objects>	DetailVec;
	typedef	DetailVec::iterator					DetailIt;
	typedef	poolSS<SlotItem, 512>				PSS;

	SSwingValue						swing_desc[2];
	SSwingValue						swing_current;
	float							m_time_rot_1;
	float							m_time_rot_2;
	float							m_time_pos;
	float							m_global_time_old;

	IReader*						dtFS;
	DetailHeader					dtH;
	DetailSlot*						dtSlots;			// note: pointer into VFS
	DetailSlot						DS_empty;

	xrThread						t_detail{ "DetailManager", true, true };
	FastLock						MT;

	// Hardware processor
	ref_geom						hw_Geom;
	u32								hw_BatchSize;
	ID3DVertexBuffer*				hw_VB;
	ID3DIndexBuffer*				hw_IB;
	ref_constant					hwc_consts;
	ref_constant					hwc_wave;
	ref_constant					hwc_wind;
	ref_constant					hwc_array;
	ref_constant					hwc_s_consts;
	ref_constant					hwc_s_xform;
	ref_constant					hwc_s_array;

	DetailVec						objects;
	vis_list						m_visibles[3];				// 0=still, 1=Wave1, 2=Wave2
	vis_list						m_visibles_shadow_sun[3];	// 0=still, 1=Wave1, 2=Wave2
	vis_list						m_visibles_shadow_light[3];	// 0=still, 1=Wave1, 2=Wave2

#ifndef _EDITOR	
	xrXRC							xrc;
#endif	

	CacheSlot1**					cache_level1;
	Slot***							cache;					// grid-cache itself
	xr_vector<Slot*>				cache_task;				// non-unpacked slots
	Slot*							cache_pool;				// just memory for slots

	int								cache_cx;
	int								cache_cz;

	PSS								poolSI;					// pool из которого выделяются SlotItem

	void							VisiblesClear();
	void							RessetScaleRandom();
	void							DetailResset();

	void							UpdateVisibleM(Fvector EYE);
	void							hw_Load();
	void							hw_Load_Geom();
	void							hw_Load_Shaders();
	void							hw_Unload();
	void							hw_Render(VisiblesType type);
	float							hw_timeDelta();
	void							hw_Render_dump(const Fvector4& consts, const Fvector4& wave, const Fvector4& wind, u32 var_id, u32 lod_id, const vis_list v_list);

	// get unpacked slot
	DetailSlot&						QueryDB(int sx, int sz);

	void							cache_Initialize();
	void							cache_Update(int sx, int sz, Fvector& view);
	void							cache_Task(int gx, int gz, Slot* D);
	void							cache_Decompress(Slot* S);
	void							spawn_Slots(Fvector& view);
	BOOL							cache_Validate();
	// cache grid to world
	int								cg2w_X(int x) const { return cache_cx - dm_size + x; }
	int								cg2w_Z(int z) const { return cache_cz - dm_size + (dm_cache_line - 1 - z); }
	// world to cache grid 
	int								w2cg_X(int x) const { return x - cache_cx + dm_size; }
	int								w2cg_Z(int z) const { return cache_cz - dm_size + (dm_cache_line - 1 - z); }
};