#include "stdafx.h"

#include "xrEngine/IRenderable.h"
#include "xrEngine/CustomHUD.h"

#include "FBasicVisual.h"
#include "SkeletonCustom.h"

using namespace R_dsgraph;

extern float r_ssaHZBvsTEX;
extern float r_ssaGLOD_start, r_ssaGLOD_end;

ICF float calcLOD(float ssa /*fDistSq*/, float /*R*/)
{
	return _sqrt(clampr((ssa - r_ssaGLOD_end) / (r_ssaGLOD_start - r_ssaGLOD_end), 0.f, 1.f));
}

template <class T> IC bool cmp_second_ssa(const T& lhs, const T& rhs) { return (lhs->second.ssa > rhs->second.ssa); }
template <class T> IC bool cmp_ssa(const T& lhs, const T& rhs) { return (lhs.ssa > rhs.ssa); }

template <class T> IC bool cmp_ps_second_ssa(const T& lhs, const T& rhs)
{
	return (lhs->second.mapCS.ssa > rhs->second.mapCS.ssa);
}

template <class T> IC bool cmp_textures_lex2(const T& lhs, const T& rhs)
{
	auto t1 = lhs->first;
	auto t2 = rhs->first;

	if ((*t1)[0] < (*t2)[0]) return true;
	if ((*t1)[0] > (*t2)[0]) return false;
	if ((*t1)[1] < (*t2)[1]) return true;
	else				return false;
}
template <class T> IC bool cmp_textures_lex3(const T& lhs, const T& rhs)
{
	auto t1 = lhs->first;
	auto t2 = rhs->first;

	if ((*t1)[0] < (*t2)[0]) return true;
	if ((*t1)[0] > (*t2)[0]) return false;
	if ((*t1)[1] < (*t2)[1]) return true;
	if ((*t1)[1] > (*t2)[1]) return false;
	if ((*t1)[2] < (*t2)[2]) return true;
	else				return false;
}
template <class T> IC bool cmp_textures_lexN(const T& lhs, const T& rhs)
{
	auto t1 = lhs->first;
	auto t2 = rhs->first;

	return std::lexicographical_compare(t1->begin(), t1->end(), t2->begin(), t2->end());
}

template <class T> void sort_tlist(auto&& lst, auto&& temp, T& textures)
{
	int amount = textures.begin()->first->size();

	if (amount <= 1)
	{
		// Just sort by SSA
		textures.getANY_P(lst);
		lst.sort(cmp_second_ssa<T::template value_type*>);
	}
	else
	{
		// Split into 2 parts
		for (auto& it : textures)
		{
			if (it.second.ssa > r_ssaHZBvsTEX)
				lst.push_back(std::move(&it));
			else
				temp.push_back(std::move(&it));
		}

		// 1st - part - SSA, 2nd - lexicographically
		lst.sort(cmp_second_ssa<T::template value_type*>);
		if (2 == amount)
			temp.sort(cmp_textures_lex2<T::template value_type*>);
		else if (3 == amount)
			temp.sort(cmp_textures_lex3<T::template value_type*>);
		else
			temp.sort(cmp_textures_lexN<T::template value_type*>);

		// merge lists
		lst.insert(lst.end(), temp.begin(), temp.end());
	}
}

void D3DXRenderBase::r_dsgraph_render_graph(u32 _priority)
{
	PIX_EVENT(r_dsgraph_render_graph);
	BasicStats.Primitives.Begin();

	// **************************************************** NORMAL
	// Perform sorting based on ScreenSpaceArea
	// Sorting by SSA and changes minimizations
	RCache.set_xform_world(Fidentity);

	// Render several passes
	for (u32 iPass = 0; iPass < SHADER_PASSES_MAX; ++iPass)
	{
		mapNormalVS& vs = mapNormalPasses[_priority][iPass];

		vs.getANY_P(nrmVS);
		nrmVS.sort(cmp_second_ssa<mapNormalVS::value_type*>);
		for (auto& vs_it : nrmVS)
		{
			RCache.set_VS(vs_it->first);

			//	GS setup
			mapNormalGS& gs = vs_it->second;
			gs.ssa = 0;

			gs.getANY_P(nrmGS);
			nrmGS.sort(cmp_second_ssa<mapNormalGS::value_type*>);
			for (auto& gs_it : nrmGS)
			{
				RCache.set_GS(gs_it->first);

				mapNormalPS& ps = gs_it->second;
				ps.ssa = 0;

				ps.getANY_P(nrmPS);
				nrmPS.sort(cmp_ps_second_ssa<mapNormalPS::value_type*>);
				for (auto& ps_it : nrmPS)
				{
					RCache.set_PS(ps_it->first);
					RCache.set_HS(ps_it->second.hs);
					RCache.set_DS(ps_it->second.ds);

					mapNormalCS& cs = ps_it->second.mapCS;
					cs.ssa = 0;

					cs.getANY_P(nrmCS);
					nrmCS.sort(cmp_second_ssa<mapNormalCS::value_type*>);
					for (auto& cs_it : nrmCS)
					{
						RCache.set_Constants(cs_it->first);

						mapNormalStates& states = cs_it->second;
						states.ssa = 0;

						states.getANY_P(nrmStates);
						nrmStates.sort(cmp_second_ssa<mapNormalStates::value_type*>);
						for (auto& state_it : nrmStates)
						{
							RCache.set_States(state_it->first);

							mapNormalTextures& tex = state_it->second;
							tex.ssa = 0;

							sort_tlist<mapNormalTextures>(nrmTextures, nrmTexturesTemp, tex);
							for (auto& tex_it : nrmTextures)
							{
								RCache.set_Textures(tex_it->first);
								RImplementation.apply_lmaterial();

								mapNormalItems& items = tex_it->second;
								items.ssa = 0;

								items.sort(cmp_ssa<_NormalItem>);
								for (auto& it_it : items)
								{
									float LOD = calcLOD(it_it.ssa, it_it.pVisual->vis.sphere.R);
									RCache.LOD.set_LOD(LOD);

									it_it.pVisual->Render(LOD);
								}
								items.clear();
							}
							nrmTexturesTemp.clear();
							nrmTextures.clear();
							tex.clear();
						}
						nrmStates.clear();
						states.clear();
					}
					nrmCS.clear();
					cs.clear();
				}
				nrmPS.clear();
				ps.clear();
			}
			nrmGS.clear();
			gs.clear();
		}
		nrmVS.clear();
		vs.clear();


		// **************************************************** MATRIX
		// Perform sorting based on ScreenSpaceArea
		// Sorting by SSA and changes minimizations
		// Render several passes
		mapMatrixVS& mvs = mapMatrixPasses[_priority][iPass];

		mvs.getANY_P(matVS);
		matVS.sort(cmp_second_ssa<mapMatrixVS::value_type*>);
		for (auto& vs_id : matVS)
		{
			RCache.set_VS(vs_id->first);

			mapMatrixGS& gs = vs_id->second;
			gs.ssa = 0;

			gs.getANY_P(matGS);
			matGS.sort(cmp_second_ssa<mapMatrixGS::value_type*>);
			for (auto& gs_it : matGS)
			{
				RCache.set_GS(gs_it->first);

				mapMatrixPS& ps = gs_it->second;
				ps.ssa = 0;

				ps.getANY_P(matPS);
				matPS.sort(cmp_ps_second_ssa<mapMatrixPS::value_type*>);
				for (auto& ps_it : matPS)
				{
					RCache.set_PS(ps_it->first);
					RCache.set_HS(ps_it->second.hs);
					RCache.set_DS(ps_it->second.ds);

					mapMatrixCS& cs = ps_it->second.mapCS;
					cs.ssa = 0;

					cs.getANY_P(matCS);
					matCS.sort(cmp_second_ssa<mapMatrixCS::value_type*>);
					for (auto& cs_it : matCS)
					{
						RCache.set_Constants(cs_it->first);

						mapMatrixStates& states = cs_it->second;
						states.ssa = 0;

						states.getANY_P(matStates);
						matStates.sort(cmp_second_ssa<mapMatrixStates::value_type*>);
						for (auto& state_it : matStates)
						{
							RCache.set_States(state_it->first);

							mapMatrixTextures& tex = state_it->second;
							tex.ssa = 0;

							sort_tlist<mapMatrixTextures>(matTextures, matTexturesTemp, tex);
							for (auto& tex_it : matTextures)
							{
								RCache.set_Textures(tex_it->first);
								RImplementation.apply_lmaterial();

								mapMatrixItems& items = tex_it->second;
								items.ssa = 0;

								items.sort(cmp_ssa<_MatrixItem>);
								for (auto& ni_it : items)
								{
									RCache.set_xform_world(ni_it.Matrix);
									RImplementation.apply_object(ni_it.pObject);
									RImplementation.apply_lmaterial();

									float LOD = calcLOD(ni_it.ssa, ni_it.pVisual->vis.sphere.R);
									RCache.LOD.set_LOD(LOD);

									ni_it.pVisual->Render(LOD);
								}
								items.clear();
							}
							matTexturesTemp.clear();
							matTextures.clear();
							tex.clear();
						}
						matStates.clear();
						states.clear();
					}
					matCS.clear();
					cs.clear();
				}
				matPS.clear();
				ps.clear();
			}
			matGS.clear();
			gs.clear();
		}
		matVS.clear();
		mvs.clear();
	}

	BasicStats.Primitives.End();
}

//////////////////////////////////////////////////////////////////////////
// Helper classes and functions

/*
Предназначен для установки режима отрисовки HUD и возврата оригинального после отрисовки.
*/
struct hud_transform_helper
{

	hud_transform_helper()
	{
		mProject.build_projection(deg2rad(::IDevice->cast()->fFOV * .75f  /* *::IDevice->cast()->fASPECT*/), ::IDevice->cast()->fASPECT,
			VIEWPORT_NEAR_HUD, g_pGamePersistent->Environment().CurrentEnv->far_plane);
		RCache.set_xform_project(mProject);

		RImplementation.rmNear();
	}

	~hud_transform_helper()
	{
		RImplementation.rmNormal();
		// Restore projection
		RCache.set_xform_project(::IDevice->cast()->mProject);
	}

private:
	Fmatrix mProject;
};

IC void __fastcall render_item(auto* item)
{
	dxRender_Visual* V = item->second.pVisual;
	VERIFY(V && V->shader._get());
	RCache.set_Element(item->second.se);
	RCache.set_xform_world(item->second.Matrix);
	RImplementation.apply_object(item->second.pObject);
	RImplementation.apply_lmaterial();
	V->Render(calcLOD(item->first, V->vis.sphere.R));
}

IC void sort_front_to_back_render_and_clean(auto&& vec)
{
	vec.traverseLR(render_item);
	vec.clear();
}

IC void sort_back_to_front_render_and_clean(auto&& vec)
{
	vec.traverseLR(render_item);
	vec.clear();
}

//////////////////////////////////////////////////////////////////////////
// HUD render
void D3DXRenderBase::r_dsgraph_render_hud()
{
	PIX_EVENT(r_dsgraph_render_hud);

	hud_transform_helper helper;

	sort_front_to_back_render_and_clean(mapHUD);
}

void D3DXRenderBase::r_dsgraph_render_hud_ui()
{
	VERIFY(g_hud && g_hud->RenderActiveItemUIQuery());

	PIX_EVENT_TEXT(L"Render Hud Item");
	hud_transform_helper helper;

	// Targets, use accumulator for temporary storage
	const ref_rt rt_null;
	RCache.set_RT(0, 1);
	RCache.set_RT(0, 2);
	auto zb = HW.pBaseZB;

	if (RImplementation.o.dx10_msaa)
		zb = RImplementation.Target->rt_MSAADepth->pZRT;

	RImplementation.Target->u_setrt(
		RImplementation.o.albedo_wo ? RImplementation.Target->rt_Accumulator : RImplementation.Target->rt_Color,
		rt_null, rt_null, zb);

	g_hud->RenderActiveItemUI();
}

//////////////////////////////////////////////////////////////////////////
// strict-sorted render
void D3DXRenderBase::r_dsgraph_render_sorted()
{
	PIX_EVENT(r_dsgraph_render_sorted);

	sort_back_to_front_render_and_clean(mapSorted);

	hud_transform_helper helper;

	sort_back_to_front_render_and_clean(mapHUDSorted);
}

//////////////////////////////////////////////////////////////////////////
// strict-sorted render
void D3DXRenderBase::r_dsgraph_render_emissive()
{
	PIX_EVENT(r_dsgraph_render_emissive);

	sort_front_to_back_render_and_clean(mapEmissive);

	hud_transform_helper helper;

	sort_front_to_back_render_and_clean(mapHUDEmissive);
}

//////////////////////////////////////////////////////////////////////////
// strict-sorted render
void D3DXRenderBase::r_dsgraph_render_wmarks()
{
	PIX_EVENT(r_dsgraph_render_wmarks);

	sort_back_to_front_render_and_clean(mapWmark);
}

//////////////////////////////////////////////////////////////////////////
// strict-sorted render
void D3DXRenderBase::r_dsgraph_render_distort()
{
	PIX_EVENT(r_dsgraph_render_distort);

	sort_back_to_front_render_and_clean(mapDistort);
}

//////////////////////////////////////////////////////////////////////////
// sub-space rendering - shortcut to render with frustum extracted from matrix
void D3DXRenderBase::r_dsgraph_render_subspace(
	IRender_Sector* _sector, Fmatrix& mCombined, Fvector& _cop, BOOL _dynamic, BOOL _precise_portals)
{
	CFrustum temp;
	temp.CreateFromMatrix(mCombined, FRUSTUM_P_ALL & (~FRUSTUM_P_NEAR));
	r_dsgraph_render_subspace(_sector, &temp, mCombined, _cop, _dynamic, _precise_portals);
}

// sub-space rendering - main procedure
void D3DXRenderBase::r_dsgraph_render_subspace(IRender_Sector* _sector, CFrustum* _frustum, Fmatrix& mCombined,
	Fvector& _cop, BOOL _dynamic, BOOL _precise_portals)
{
	VERIFY(_sector);
	PIX_EVENT(r_dsgraph_render_subspace);
	RImplementation.marker++; // !!! critical here

	// Save and build new frustum, disable HOM
	CFrustum ViewSave = ::IDevice->cast()->ViewFromMatrix;
	View = &(::IDevice->cast()->ViewFromMatrix = *_frustum);

	if (_precise_portals && RImplementation.rmPortals)
	{
		// Check if camera is too near to some portal - if so force DualRender
		Fvector box_radius;
		constexpr float box_side = EPS_L * 20;
		box_radius.set(box_side, box_side, box_side);
		RImplementation.Sectors_xrc.box_query(CDB::OPT_FULL_TEST, RImplementation.rmPortals, _cop, box_radius);
		for (int K = 0; K < RImplementation.Sectors_xrc.r_count(); K++)
		{
			CPortal*& pPortal =
				reinterpret_cast<CPortal*&>(RImplementation.Portals[RImplementation.rmPortals->get_tris()[RImplementation.Sectors_xrc.r_begin()[K].id].dummy]);
			pPortal->bDualRender = TRUE;
		}
	}

	// Traverse sector/portal structure
	PortalTraverser.traverse(_sector, ::IDevice->cast()->ViewFromMatrix, _cop, mCombined, 0);

	// Determine visibility for static geometry hierrarhy
	for (u32 s_it = 0; s_it < PortalTraverser.r_sectors.size(); s_it++)
	{
		CSector* sector = reinterpret_cast<CSector*>(PortalTraverser.r_sectors[s_it]);
		dxRender_Visual* root = sector->root();
		for (CFrustum& frustum : sector->r_frustums)
		{
			set_Frustum(&frustum);
			add_Geometry(root);
		}
	}

	if (_dynamic)
	{
		set_Object(nullptr);

		auto renderable_spatial = [this, ViewSave](ISpatial* spatial) -> void
		{
			if (CSector* sector = reinterpret_cast<CSector*>(spatial->GetSpatialData().sector))// disassociated from S/P structure
			{
				if (PortalTraverser.i_marker == sector->r_marker)
				{
					if (auto renderable = spatial->dcast_Renderable())
					{
						extern bool VisibleToRender(IRenderVisual * pVisual, bool isStatic, bool sm, Fmatrix & transform_matrix, bool ignore_optimize = false);

						if (!VisibleToRender(renderable->GetRenderData().visual, false, true, renderable->GetRenderData().xform))
							return;

						for (CFrustum& frustum : sector->r_frustums)
						{
							set_Frustum(&frustum);
							if (View->testSphere_dirty(spatial->GetSpatialData().sphere.P, spatial->GetSpatialData().sphere.R))
							{
								if (CKinematics* pKin = reinterpret_cast<CKinematics*>(renderable->GetRenderData().visual))
									pKin->CalculateBones(TRUE);

								renderable->renderable_Render();
								break;
							}
						}
					}
				}
			}
		};

		// Determine visibility for dynamic part of scene
		g_SpatialSpace->q_frustum_it
		(
			renderable_spatial,
			ISpatial_DB::O_ORDERED,
			STYPE_RENDERABLE + STYPE_RENDERABLESHADOW,
			::IDevice->cast()->ViewFromMatrix
		);

		if (g_pGameLevel && (phase == RImplementation.PHASE_SMAP) && ps_actor_shadow_flags.test(RFLAG_ACTOR_SHADOW))
		{
			if (auto Spatial = g_hud->Render_Actor_Shadow())
			{
				PIX_EVENT_TEXT(L"Render Actor Shadow");
				renderable_spatial(Spatial);
			}
		}
	}

	// Restore
	::IDevice->cast()->ViewFromMatrix = ViewSave;
	View = nullptr;
}

#include "FLOD.h"
void D3DXRenderBase::r_dsgraph_render_R1_box(IRender_Sector* S, Fbox& BB, int sh)
{
	PIX_EVENT(r_dsgraph_render_R1_box);

	lstVisuals.clear();
	lstVisuals.push_back(std::move(reinterpret_cast<CSector*>(S)->root()));

	for (dxRender_Visual*& visual: lstVisuals)
	{
		switch (visual->Type)
		{
		case MT_HIERRARHY: {
			for (dxRender_Visual*& Vis : reinterpret_cast<FHierrarhyVisual*>(visual)->children)
				if (BB.intersect(Vis->vis.box))
					lstVisuals.push_back(std::move(Vis));
		}
		break;
		case MT_SKELETON_ANIM:
		case MT_SKELETON_RIGID: {
			auto pV = reinterpret_cast<CKinematics*>(visual);
			pV->CalculateBones(TRUE);

			for (dxRender_Visual*& Vis : pV->children)
				if (BB.intersect(Vis->vis.box))
					lstVisuals.push_back(std::move(Vis));
		}
		break;
		case MT_LOD:{
			for (dxRender_Visual*& Vis : reinterpret_cast<FLOD*>(visual)->children)
				if (BB.intersect(Vis->vis.box))
					lstVisuals.push_back(std::move(Vis));
		}
		break;
		default:
		{
			// Renderable visual
			ShaderElement* E = visual->shader->E[sh]._get();
			if (E && !(E->flags.bDistort))
			{
				for (u32 pass = 0; pass < E->passes.size(); pass++)
				{
					RCache.set_Element(E, pass);
					visual->Render(-1.f);
				}
			}
		}
		break;
		}
	}
}
