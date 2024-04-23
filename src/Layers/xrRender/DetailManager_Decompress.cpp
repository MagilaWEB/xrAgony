#include "stdafx.h"
#pragma hdrstop
#include "DetailManager.h"
#include "xrCDB/Intersect.hpp"
#ifdef _EDITOR
#include "scene.h"
#include "sceneobject.h"
#include "utils/ETools/ETools.h"
#endif

//--------------------------------------------------- Decompression
IC float	Interpolate(float* base, u32 x, u32 y, u32 size)
{
	float	f = float(size);
	float	fx = float(x) / f; float ifx = 1.f - fx;
	float	fy = float(y) / f; float ify = 1.f - fy;

	float	c01 = base[0] * ifx + base[1] * fx;
	float	c23 = base[2] * ifx + base[3] * fx;

	float	c02 = base[0] * ify + base[2] * fy;
	float	c13 = base[1] * ify + base[3] * fy;

	float	cx = ify * c01 + fy * c23;
	float	cy = ifx * c02 + fx * c13;
	return	(cx + cy) / 2;
}

IC bool		InterpolateAndDither(float* alpha255, u32 x, u32 y, u32 sx, u32 sy, u32 size, int dither[16][16])
{
	clamp(x, (u32)0, size - 1);
	clamp(y, (u32)0, size - 1);
	int		c = iFloor(Interpolate(alpha255, x, y, size) + .5f);
	clamp(c, 0, 255);

	u32	row = (y + sy) % 16;
	u32	col = (x + sx) % 16;
	return	c > dither[col][row];
}

#ifndef _EDITOR
#ifdef	DEBUG
//#include "../../Include/xrRender/DebugRender.h"
#include "dxDebugRender.h"
static void draw_obb(const Fmatrix& matrix, const u32& color)
{
	Fvector							aabb[8];
	matrix.transform_tiny(aabb[0], Fvector().set(-1, -1, -1)); // 0
	matrix.transform_tiny(aabb[1], Fvector().set(-1, +1, -1)); // 1
	matrix.transform_tiny(aabb[2], Fvector().set(+1, +1, -1)); // 2
	matrix.transform_tiny(aabb[3], Fvector().set(+1, -1, -1)); // 3
	matrix.transform_tiny(aabb[4], Fvector().set(-1, -1, +1)); // 4
	matrix.transform_tiny(aabb[5], Fvector().set(-1, +1, +1)); // 5
	matrix.transform_tiny(aabb[6], Fvector().set(+1, +1, +1)); // 6
	matrix.transform_tiny(aabb[7], Fvector().set(+1, -1, +1)); // 7

	u16								aabb_id[12 * 2] = {
		0,1,  1,2,  2,3,  3,0,  4,5,  5,6,  6,7,  7,4,  1,5,  2,6,  3,7,  0,4
	};

	rdebug_render->add_lines(aabb, sizeof(aabb) / sizeof(Fvector), &aabb_id[0], sizeof(aabb_id) / (2 * sizeof(u16)), color);
}

bool det_render_debug = false;
#endif
#endif

#include "../../xrEngine/GameMtlLib.h" 

void CDetailManager::cache_Decompress(Slot* S)
{
	VERIFY(S);
	Slot& D = *S;
	D.type = stReady;
	if (D.empty)		return;

	DetailSlot& DS = QueryDB(D.sx, D.sz);

	// Select polygons
	Fvector		bC, bD;
	D.vis.box.get_CD(bC, bD);

#ifdef _EDITOR
	ETOOLS::box_options(CDB::OPT_FULL_TEST);
	// Select polygons
	SBoxPickInfoVec		pinf;
	Scene->BoxPickObjects(D.vis.box, pinf, GetSnapList());
	u32	triCount = pinf.size();
#else
	xrc.box_options(CDB::OPT_FULL_TEST);
	xrc.box_query(g_pGameLevel->ObjectSpace.GetStaticModel(), bC, bD);
#endif

	if (0 == xrc.r_count())	return;

	// Build shading table
	float		alpha255[dm_obj_in_slot][4];
	for (int i = 0; i < dm_obj_in_slot; i++)
	{
		alpha255[i][0] = 255.f * float(DS.palette[i].a0) / 15.f;
		alpha255[i][1] = 255.f * float(DS.palette[i].a1) / 15.f;
		alpha255[i][2] = 255.f * float(DS.palette[i].a2) / 15.f;
		alpha255[i][3] = 255.f * float(DS.palette[i].a3) / 15.f;
	}

	// Prepare to selection
	float		density = ps_r__Detail_density * GEnv.Render->grass_level_density;
	float		jitter = density / 1.7f;
	u32			d_size = iCeil(dm_slot_size / density);
	svector<int, dm_obj_in_slot>		selected;

	u32 p_rnd = D.sx * D.sz; // нужно для того чтобы убрать полосы(ряды)
	CRandom				r_selection(0x12071980 ^ p_rnd);
	CRandom				r_jitter(0x12071980 ^ p_rnd);
	CRandom				r_yaw(0x12071980 ^ p_rnd);
	CRandom				r_scale(0x12071980 ^ p_rnd);

	// Prepare to actual-bounds-calculations
	Fbox				Bounds{};
	Bounds.invalidate();

	// Decompressing itself
	for (u32 z = 0; z <= d_size; z++)
	{
		for (u32 x = 0; x <= d_size; x++)
		{
			// shift
			u32 shift_x = r_jitter.randI(16);
			u32 shift_z = r_jitter.randI(16);

			// Iterpolate and dither palette
			selected.clear();

			if ((DS.id0 != DetailSlot::ID_Empty) && InterpolateAndDither(alpha255[0], x, z, shift_x, shift_z, d_size, dither))
				selected.push_back(0);
			if ((DS.id1 != DetailSlot::ID_Empty) && InterpolateAndDither(alpha255[1], x, z, shift_x, shift_z, d_size, dither))
				selected.push_back(1);
			if ((DS.id2 != DetailSlot::ID_Empty) && InterpolateAndDither(alpha255[2], x, z, shift_x, shift_z, d_size, dither))
				selected.push_back(2);
			if ((DS.id3 != DetailSlot::ID_Empty) && InterpolateAndDither(alpha255[3], x, z, shift_x, shift_z, d_size, dither))
				selected.push_back(3);

			// Select
			if (selected.empty()) continue;

			u32 index;
			if (selected.size() == 1)
				index = selected[0];
			else
				index = selected[r_selection.randI(selected.size())];

			CDetail* Dobj = objects[DS.r_id(index)];
			SlotItem* ItemP = poolSI.create();
			SlotItem& Item = *ItemP;

			// Position (XZ)
			float		rx = (float(x) / float(d_size)) * dm_slot_size + D.vis.box.vMin.x;
			float		rz = (float(z) / float(d_size)) * dm_slot_size + D.vis.box.vMin.z;
			Fvector		Item_P{};

			Item_P.set(rx + r_jitter.randFs(jitter), D.vis.box.vMax.y + 1.f, rz + r_jitter.randFs(jitter));

			collide::rq_result RQ;

			// Adjust the position by the Y coordinate above the level.
			g_pGameLevel->ObjectSpace.RayQuery(
				collide::rq_results{},
				collide::ray_defs{ Item_P,  Fvector{ 0, -1.f, 0 }, 1000.f, CDB::OPT_CULL, collide::rqtStatic },
				[](collide::rq_result& result, LPVOID params)
			{
				auto LOCAL_RQ = reinterpret_cast<collide::rq_result*>(params);
				if (result.O)
				{
					*LOCAL_RQ = result;
					return FALSE;
				}
				else
				{
					CDB::TRI* T = g_pGameLevel->ObjectSpace.GetStaticTris() + result.element;
					SGameMtl* mtl = GMLib.GetMaterialByIdx(T->material);
					// Ignore liquid and dynamic.
					if (mtl->Flags.is(SGameMtl::flLiquid) || mtl->Flags.is(SGameMtl::flDynamic))
						return TRUE;
				}
				*LOCAL_RQ = result;
				return FALSE;
			},
				&RQ,
				nullptr,
				nullptr
			);

			// The exorbitant value means only one thing, the beam did not meet the geometry and flew away to the maximum distance.
			if (RQ.range > 99.f)
				continue;

			if (RQ.range > 0.f)
				Item_P.y = Item_P.y - RQ.range;

			// Ignore if there is an obstacle.
			if (g_pGameLevel->ObjectSpace.RayPick(Fvector{ Item_P }.add(Fvector{ 0.f, 0.05f, 0.f }), Fvector{ 0.f, 1.f, 0.f }, 1.4f, collide::rqtStatic, RQ, nullptr))
				continue;

			// Ignore if the material does not match.
			if (g_pGameLevel->ObjectSpace.RayPick(Fvector{ Item_P }.add(Fvector{ 0.f, 0.1f, 0.f }), Fvector{ 0.f, -1.f, 0.f }, 1000.f, collide::rqtStatic, RQ, nullptr))
			{

				CDB::TRI* T = g_pGameLevel->ObjectSpace.GetStaticTris() + RQ.element;

				SGameMtl* mtl = GMLib.GetMaterialByIdx(T->material);
				xr_string name_material{ mtl->m_Name.c_str() };

				// Where will we allow the grass to appear.
				xr_string list_material[5]{
					"grass",		// трава
					"dirt",			// грязь
					"water",		// вода
					"earth",		// земля
					"sand"			// песок? На некоторых локациях песок является травой...
				};

				bool ignore = true;

				for (u32 it = 0; it < sizeof(list_material) / sizeof(xr_string); it++)
				{
					if (name_material.find(list_material[it]) != xr_string::npos)
					{
						ignore = false;
						break;
					}
				}

				if (ignore)
					continue;
			}
			else
				continue;

			// humidity
			/*if (g_pGameLevel->ObjectSpace.RayPick(Item_P, Fvector{ 0.f, 1.f, 0.f }, 1000.f, collide::rqtStatic, RQ, nullptr))
			{
				CDB::TRI* T = g_pGameLevel->ObjectSpace.GetStaticTris() + RQ.element;

				SGameMtl* mtl = GMLib.GetMaterialByIdx(T->material);
				Item.is_shelter = mtl && !!mtl->Flags.test(SGameMtl::flPassable);
			}*/

			// Angles and scale
			Item.scale = r_scale.randF(Dobj->m_fMinScale, Dobj->m_fMaxScale);
			Item.scale *= GEnv.Render->grass_level_scale;

			// X-Form BBox
			Fmatrix		mScale{}, mXform{};
			Fbox		ItemBB{};

			Item.mRotY.rotateY(r_yaw.randF(0, PI_MUL_2));

			Item.mRotY.translate_over(Item_P);
			mScale.scale(Item.scale, Item.scale, Item.scale);
			mXform.mul_43(Item.mRotY, mScale);
			ItemBB.xform(Dobj->bv_bb, mXform);
			Bounds.merge(ItemBB);
			Item.mRotYCache.set(Item.mRotY);

			// Collision of a blade of grass (its slopes from physical impact)
			//Item.collision_size = 1.f;

			//Item.humidity = .0f;

#ifndef _EDITOR
#ifdef		DEBUG
			if (det_render_debug)
				draw_obb(mXform, color_rgba(255, 0, 0, 255));	//Fmatrix().mul_43(mXform, Fmatrix().scale(5,5,5))
#endif
#endif

			Item.c_hemi = DS.r_qclr(DS.c_hemi, 15);
			Item.c_sun = DS.r_qclr(DS.c_dir, 15);

			// Vis-sorting
			if (Dobj->m_Flags.is(DO_NO_WAVING))
				Item.vis_ID = 0;
			else
			{
				if (::Random.randI(0, 3) == 0)
					Item.vis_ID = 2; // Second wave
				else
					Item.vis_ID = 1; // First wave
			}

			// Save it
			D.G[index].items.push_back(std::move(ItemP));
		}
	}

	// Update bounds to more tight and real ones
	D.vis.clear();
	D.vis.box.set(Bounds);
	D.vis.box.getsphere(D.vis.sphere.P, D.vis.sphere.R);
}