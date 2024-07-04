// DetailManager.cpp: implementation of the CDetailManager class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "DetailManager.h"

#ifdef _EDITOR
#include "ESceneClassList.h"
#include "Scene.h"
#include "SceneObject.h"
#include "IGame_Persistent.h"
#include "Environment.h"
#else
#include "xrEngine/IGame_Persistent.h"
#include "xrEngine/Environment.h"
#endif

const float dbgOffset = 0.f;
const int	dbgItems = 128;

//--------------------------------------------------- Decompression
static int magic4x4[4][4] =
{
	{ 0, 14,  3, 13},
	{11,  5,  8,  6},
	{12,  2, 15,  1},
	{ 7,  9,  4, 10}
};

void bwdithermap(int levels, int magic[16][16])
{
	/* Get size of each step */
	float N = 255.0f / (levels - 1);

	/*
	* Expand 4x4 dither pattern to 16x16.  4x4 leaves obvious patterning,
	* and doesn't give us full intensity range (only 17 sublevels).
	*
	* magicfact is (N - 1)/16 so that we get numbers in the matrix from 0 to
	* N - 1: mod N gives numbers in 0 to N - 1, don't ever want all
	* pixels incremented to the next level (this is reserved for the
	* pixel value with mod N == 0 at the next level).
	*/

	float	magicfact = (N - 1) / 16;
	for (int i = 0; i < 4; i++)
		for (int j = 0; j < 4; j++)
			for (int k = 0; k < 4; k++)
				for (int l = 0; l < 4; l++)
					magic[4 * k + i][4 * l + j] =
					(int)(0.5 + magic4x4[i][j] * magicfact +
						(magic4x4[k][l] / 16.) * magicfact);
}
//--------------------------------------------------- Decompression

void CDetailManager::SSwingValue::lerp(const SSwingValue& A, const SSwingValue& B, float f)
{
	float fi = 1.f - f;
	amp1 = fi * A.amp1 + f * B.amp1;
	amp2 = fi * A.amp2 + f * B.amp2;
	rot1 = fi * A.rot1 + f * B.rot1;
	rot2 = fi * A.rot2 + f * B.rot2;
	speed = fi * A.speed + f * B.speed;
}
//---------------------------------------------------

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
CDetailManager::CDetailManager()
{
	dtFS = 0;
	dtSlots = 0;
	hw_BatchSize = 0;
	hw_VB = 0;
	hw_IB = 0;
	m_time_rot_1 = 0;
	m_time_rot_2 = 0;
	m_time_pos = 0;

	dm_size_MAX = iFloor((float)ps_r__detail_radius_MAX / 4) * 2;
	dm_cache1_line_MAX = dm_size_MAX * 2 / 4;		// assuming cache1_count = 4
	dm_cache_line_MAX = dm_size_MAX + 1 + dm_size_MAX;
	dm_cache_size_MAX = dm_cache_line_MAX * dm_cache_line_MAX;

	cache_level1 = (CacheSlot1**)xr_malloc(dm_cache1_line_MAX * sizeof(CacheSlot1*));

	for (u32 i = 0; i < dm_cache1_line_MAX; ++i)
	{
		cache_level1[i] = (CacheSlot1*)xr_malloc(dm_cache1_line_MAX * sizeof(CacheSlot1));
		for (u32 j = 0; j < dm_cache1_line_MAX; ++j)
			new (&(cache_level1[i][j])) CacheSlot1();
	}

	cache = (Slot***)xr_malloc(dm_cache_line_MAX * sizeof(Slot**));
	for (u32 i = 0; i < dm_cache_line_MAX; ++i)
		cache[i] = (Slot**)xr_malloc(dm_cache_line_MAX * sizeof(Slot*));

	cache_pool = (Slot*)xr_malloc(dm_cache_size_MAX * sizeof(Slot));
	for (u32 i = 0; i < dm_cache_size_MAX; ++i)
		new (&(cache_pool[i])) Slot();

	detail_size();
}

CDetailManager::~CDetailManager()
{
	if (dtFS)
		FS.r_close(dtFS);

	for (u32 i = 0; i < dm_cache_size_MAX; ++i)
		cache_pool[i].~Slot();
	xr_free(cache_pool);

	for (u32 i = 0; i < dm_cache_line_MAX; ++i)
		xr_free(cache[i]);
	xr_free(cache);

	for (u32 i = 0; i < dm_cache1_line_MAX; ++i)
	{
		for (u32 j = 0; j < dm_cache1_line_MAX; ++j)
			cache_level1[i][j].~CacheSlot1();
		xr_free(cache_level1[i]);
	}
	xr_free(cache_level1);

	dtFS = nullptr;
}

#ifndef _EDITOR

void CDetailManager::Load()
{
	// Open file stream
	if (!FS.exist("$level$", "level.details"))
	{
		dtFS = nullptr;
		return;
	}

	string_path			fn;
	FS.update_path(fn, "$level$", "level.details");
	dtFS = FS.r_open(fn);

	// Header
	dtFS->r_chunk_safe(0, &dtH, sizeof(dtH));
	R_ASSERT(dtH.version() == DETAIL_VERSION);
	u32 m_count = dtH.object_count();

	// Models
	IReader* m_fs = dtFS->open_chunk(1);
	for (u32 m_id = 0; m_id < m_count; m_id++)
	{
		CDetail* dt = new CDetail();
		IReader* S = m_fs->open_chunk(m_id);
		dt->Load(S);
		objects.push_back(dt);
		S->close();
	}
	m_fs->close();

	// Get pointer to database (slots)
	IReader* m_slots = dtFS->open_chunk(2);
	dtSlots = (DetailSlot*)m_slots->pointer();
	m_slots->close();

	// Initialize 'vis' and 'cache'
	for (u32 i = 0; i < 3; ++i)
	{
		m_visibles[i].resize(objects.size());
		m_visibles_shadow_sun[i].resize(objects.size());
		m_visibles_shadow_light[i].resize(objects.size());
	}

	cache_Initialize();

	// Make dither matrix
	bwdithermap(2, dither);

	hw_Load();

	// swing desc
	// normal
	swing_desc[0].amp1 = pSettings->r_float("details", "swing_normal_amp1");
	swing_desc[0].amp2 = pSettings->r_float("details", "swing_normal_amp2");
	swing_desc[0].rot1 = pSettings->r_float("details", "swing_normal_rot1");
	swing_desc[0].rot2 = pSettings->r_float("details", "swing_normal_rot2");
	swing_desc[0].speed = pSettings->r_float("details", "swing_normal_speed");
	// fast
	swing_desc[1].amp1 = pSettings->r_float("details", "swing_fast_amp1");
	swing_desc[1].amp2 = pSettings->r_float("details", "swing_fast_amp2");
	swing_desc[1].rot1 = pSettings->r_float("details", "swing_fast_rot1");
	swing_desc[1].rot2 = pSettings->r_float("details", "swing_fast_rot2");
	swing_desc[1].speed = pSettings->r_float("details", "swing_fast_speed");
	t_detail.Init([this]() { Frame(); }, xrThread::sParalelFrame);
}
#endif

void CDetailManager::Unload()
{
	t_detail.Stop();
	hw_Unload();

	for (CDetail* detail : objects)
	{
		detail->Unload();
		xr_delete(detail);
	}

	objects.clear();

	VisiblesClear();

	if (dtFS)
		FS.r_close(dtFS);

	dtFS = nullptr;
}

void CDetailManager::detail_size()
{
	dm_size = iFloor((float)ps_r__detail_radius / 4) * 2;
	dm_cache1_line = dm_size * 2 / 4;		// assuming cache1_count = 4
	dm_cache_line = dm_size + 1 + dm_size;
	dm_cache_size = dm_cache_line * dm_cache_line;
	dm_fade = float(2 * dm_size) - .5f;
}

void CDetailManager::VisiblesClear()
{
	// Clean up
	for (auto& vec : m_visibles)
		for (auto& vis : vec)
			vis.clear();

	for (auto& vec : m_visibles_shadow_sun)
		for (auto& vis : vec)
			vis.clear();

	for (auto& vec : m_visibles_shadow_light)
		for (auto& vis : vec)
			vis.clear();
}

void CDetailManager::UpdateVisibleM(Fvector	EYE)
{
	xrCriticalSection::raii mt{ MT };

	RImplementation.BasicStats.DetailVisibility.Begin();
	const float dist_optimization = (10 - (ps_r__details_opt_intensity - 1)) * 550.f;

	CFrustum View{};
	View.CreateFromMatrix(Device.mFullTransformSaved, FRUSTUM_P_LRTB + FRUSTUM_P_FAR);
	float fade_limit = dm_fade * dm_fade;
	float fade_start = 2.f;
	float fade_range = fade_limit - fade_start;

	// Initialize 'vis' and 'cache'
	// Collect objects for rendering
	for (u32 _mz = 0; _mz < dm_cache1_line; _mz++)
	{
		for (u32 _mx = 0; _mx < dm_cache1_line; _mx++)
		{
			CacheSlot1& MS = cache_level1[_mz][_mx];
			if (MS.empty)
				continue;

			if (!View.testSphere_dirty(MS.vis.sphere.P, MS.vis.sphere.R))
				continue;	// invisible-view frustum

			for (u32 _i = 0; _i < (dm_cache1_count * dm_cache1_count); _i++)
			{
				Slot* S = *MS.slots[_i];
				if (!S)
					continue;

				if (S->empty)
					continue;

				if (!View.testSphere_dirty(S->vis.sphere.P, S->vis.sphere.R))
					continue;	// invisible-view frustum

				if (!RImplementation.HOM.visible(S->vis))
					continue;	// invisible-occlusion

				float dist_sq = EYE.distance_to_sqr(S->vis.sphere.P);
				if (dist_sq > fade_limit)
					continue;

				bool optimization{ ps_r__details_opt_intensity && (dist_sq > dist_optimization) };
				float dist_sq_rcp = 1.f / dist_sq;

				for (SlotPart& sp : S->G)
				{
					if (sp.id == DetailSlot::ID_Empty)	continue;

					float R = objects[sp.id]->bv_sphere.R;
					float Rq_drcp = R * R * dist_sq_rcp; // reordered expression for 'ssa' calc

					for (u32 it = 0; it < sp.items.size(); it++)
					{
						if (optimization && (it % ps_r__details_opt_intensity != 0))
							continue;

						SlotItem*& Item = sp.items[it];
						if (!Item) continue;

						if (RImplementation.pOutdoorSector && PortalTraverser.i_marker != RImplementation.pOutdoorSector->r_marker)
							continue;

						CSector* sector = (CSector*)RImplementation.getSector(Item->sector_id);
						if (sector && PortalTraverser.i_marker != sector->r_marker)
							continue;

						if (optimization)
						{
							if (!Item->optimization)
							{
								float w = ps_r__details_opt_intensity * .5f;
								clamp(w, 2.f, 4.f);
								Item->mRotY.k.mul(Fvector{ w, 1.f, w });
								Item->mRotY.i.mul(Fvector{ w, 1.f, w });
								Item->optimization = true;
							}
						}
						else if (Item->optimization)
						{
							Item->mRotY.set(Item->mRotYCache);
							Item->optimization = false;
						}

						if (Item->scale_random <= .0f)
						{
							if (ps_r__detail_scale_random != 1.f)
							{
								const float coff = ps_r__detail_scale_random / 4;
								Item->scale_random = Random.randF(coff, 1.f + (1.f - coff));
							}
							else
								Item->scale_random = 1.f;
						}

						Item->scale_calculated = Item->scale;

						if (Item->scale_calculated > 0.f)
						{
							//colision
							/*if ((!optimization) && (!Item->collision_save))
							{
								Item->collision_size_cache = 1.f;

								for (const Fvector4& col : ::Render->grass_colision_pos)
								{
									float dist = Fvector{ col.x, col.z, col.y }.distance_to_sqr(Item->mRotY.c);
									float radius = ps_r__grass_collision_radius * col.w;
									if (dist < radius)
									{
										float reduce = (dist / radius);
										if (reduce < ps_r__grass_collision_minimal)
										{
											reduce = ps_r__grass_collision_minimal;
											Item->collision_save = true;
											Item->collision_size_cache = reduce;
											break;
										}
										Item->collision_size_cache = reduce;
									}
								}

								if ((!Item->collision_save) && Item->collision_size_cache >= Item->collision_size)
								{
									Item->collision_size += (Device.fTimeDelta * ps_r__grass_collision_speed);
									clamp(Item->collision_size, 0.01f, Item->collision_size_cache);
								}
								else
									Item->collision_size = Item->collision_size_cache;
							}*/

							Item->mRotY.j.set(Item->mRotYCache.j);
							//Item->mRotY.j.mul(Item->collision_size);

							//humidity
							/*if (!Item->is_shelter)
							{
								if (::Render->grass_humidity > .0f && Item->humidity < ::Render->grass_humidity)
									Item->humidity += (::Render->grass_humidity * Device.fTimeDelta * 0.05f);
								else if (Item->humidity > .0f)
									Item->humidity -= 0.01f * Device.fTimeDelta;

								clamp(Item->humidity, 0.f, 1.f);

								float humidity = 1.f - (Item->humidity / 3);
								Item->mRotY.j.mul(humidity);
							}*/

							Item->scale_calculated *= ps_r__detail_scale * Item->scale_random;
							

							const u32 vis_id = Item->vis_ID;

							//Device.position_render_ui(Item->mRotY.c);
							m_visibles[vis_id][sp.id].push_back(Item);

							// Shadow!
							if (dist_sq < ps_r__Detail_shadow_sun_density)
								m_visibles_shadow_sun[vis_id][sp.id].push_back(Item);

							if (dist_sq < ps_r__Detail_shadow_light_density)
								m_visibles_shadow_light[vis_id][sp.id].push_back(Item);
						}
					}
				}
			}
		}
	}
	//::Render->grass_colision_pos.clear();
	RImplementation.BasicStats.DetailVisibility.End();
}

void CDetailManager::RessetScaleRandom()
{
	static float dm_current_detail_scale_random = ps_r__detail_scale_random;
	if (ps_r__detail_scale_random != dm_current_detail_scale_random)
	{

		FOR_START(u32, 0, dm_cache1_line, _mz)
		{
			for (u32 _mx = 0; _mx < dm_cache1_line; _mx++)
			{
				CacheSlot1& MS = cache_level1[_mz][_mx];
				if (MS.empty)
					continue;

				for (u32 _i = 0; _i < (dm_cache1_count * dm_cache1_count); _i++)
				{
					Slot* S = *MS.slots[_i];
					if (S->empty)
						continue;

					for (SlotPart& sp : S->G)
					{
						if (sp.id == DetailSlot::ID_Empty)	continue;

						for (SlotItem*& Item : sp.items)
						{
							if (!Item) continue;

							if (ps_r__detail_scale_random != 1.f)
							{
								const float coff = ps_r__detail_scale_random / 3;
								Item->scale_random = Random.randF(coff, 1.f + (1.f - coff));
							}
							else
								Item->scale_random = 1.f;
						}
					}
				}
			}
		}
		FOR_END

			dm_current_detail_scale_random = ps_r__detail_scale_random;
	}
}

void CDetailManager::DetailResset()
{
	static float current_grass_level_scale = ::Render->grass_level_scale;
	static float current_grass_level_density = ::Render->grass_level_density;
	static float current_detail_density = ps_r__Detail_density;
	static int dm_current_size = ps_r__detail_radius;
	static int details_opt_intensity = ps_r__details_opt_intensity;
	if (
		current_detail_density != ps_r__Detail_density ||
		dm_current_size != ps_r__detail_radius ||
		current_grass_level_scale != ::Render->grass_level_scale ||
		current_grass_level_density != ::Render->grass_level_density ||
		details_opt_intensity != ps_r__details_opt_intensity
		)
	{
		current_detail_density = ps_r__Detail_density;
		dm_current_size = ps_r__detail_radius;
		current_grass_level_scale = ::Render->grass_level_scale;
		current_grass_level_density = ::Render->grass_level_density;
		details_opt_intensity = ps_r__details_opt_intensity;
		// Initialize 'vis' and 'cache'
		detail_size();
		cache_Initialize();
		//Msg("- New parameters part density [%f], size [%d].", current_detail_density, dm_current_size);
	}
}

void CDetailManager::Frame()
{
	if (!RImplementation.Details) return;	// possibly deleted
	if (!dtFS) return;
	if (!psDeviceFlags.is(rsDetails)) return;
	if (Device.ActiveMain()) return;

	LIMIT_UPDATE_FPS(DetailManagerFPS, 30)

	RImplementation.BasicStats.DetailCache.Begin();
	Fvector EYE = Device.vCameraPositionSaved;

	int s_x = iFloor(EYE.x / dm_slot_size + .5f);
	int s_z = iFloor(EYE.z / dm_slot_size + .5f);
	
	VisiblesClear();

	UpdateVisibleM(EYE);

	xrCriticalSection::raii mt{ MT };
	LIMIT_UPDATE_FPS_CODE(detail_cache_Update, ps_r__detail_limit_update, cache_Update(s_x, s_z, EYE);)
	spawn_Slots(EYE);
	RImplementation.BasicStats.DetailCache.End();
}

void CDetailManager::Render(VisiblesType type)
{
	if (!RImplementation.Details) return;	// possibly deleted
	if (!dtFS) return;
	if (!psDeviceFlags.is(rsDetails)) return;
	if (Device.ActiveMain()) return;

	{
		xrCriticalSection::raii mt{ MT };
		DetailResset();
		RessetScaleRandom();
	}

	RImplementation.BasicStats.DetailRender.Begin();

	swing_current.lerp(swing_desc[0], swing_desc[1], g_pGamePersistent->Environment().wind_strength_factor);

	RCache.set_CullMode(CULL_NONE);
	RCache.set_xform_world(Fidentity);
	hw_Render(type);
	RCache.set_CullMode(CULL_CCW);
	RImplementation.BasicStats.DetailRender.End();
}