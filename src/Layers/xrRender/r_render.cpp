#include "stdafx.h"
#include "xrEngine/CustomHUD.h"
#include "xrEngine/IGame_Persistent.h"
#include "xrEngine/xr_object.h"
#include "xrRender/FBasicVisual.h"
#include "xrRender/QueryHelper.h"
#include "xrRender/SkeletonCustom.h"

#define DBG_LINHT FALSE

#if DBG_LINHT
#pragma comment(lib, "xrUICore.lib")
#include "xrEngine/gamefont.h"
#include <xrUICore\ui_base.h>

IC void dbg_light_renderer(light* L, u32 color = color_rgba(0, 255, 100, 255), int sectors = 0)
{
	Fvector4		v_res;
	Device.mFullTransform.transform(v_res, L->position);

	float x = (1.f + v_res.x) / 2.f * (Device.dwWidth);
	float y = (1.f - v_res.y) / 2.f * (Device.dwHeight);

	if (v_res.z < 0 || v_res.w < 0)
		return;

	if (v_res.x < -1.f || v_res.x > 1.f || v_res.y < -1.f || v_res.y>1.f)
		return;
	if (!L->flags.bActive) return;
	if (L->flags.bStatic) return;

	UI().Font().pFontStat->SetAligment(CGameFont::alCenter);
	UI().Font().pFontStat->SetColor(color);

	LPCSTR l_type = " ";
	if (L->flags.type == IRender_Light::POINT)
		l_type = "POINT";
	if (L->flags.type == IRender_Light::OMNIPART)
		l_type = "OMNIPART";
	if (L->flags.type == IRender_Light::SPOT)
		l_type = "SPOT";
	if (L->flags.type == IRender_Light::DIRECT)
		l_type = "DIRECT";
	if (L->flags.type == IRender_Light::REFLECTED)
		l_type = "REFLECTED";
	UI().Font().pFontStat->Out(x, y, "%s %s sc[%d]", l_type, L->flags.bShadow ? "Shdw" : " ", sectors);
}
#endif

void CRender::render_main(bool deffered)
{
	PIX_EVENT(render_main);
	marker++;
	const bool dont_test_sectors = Sectors.size() <= 1;

	// Calculate sector(s) and their objects
	if (pLastSector)
	{
		//!!!
		//!!! BECAUSE OF PARALLEL HOM RENDERING TRY TO DELAY ACCESS TO HOM AS MUCH AS POSSIBLE
		//!!!
		{
			// Traverse object database
			g_SpatialSpace->q_frustum
			(
				lstRenderables,
				ISpatial_DB::O_ORDERED,
				STYPE_RENDERABLE + STYPE_RENDERABLESHADOW + STYPE_PARTICLE + STYPE_LIGHTSOURCE,
				ViewBase
			);

			// (almost) Exact sorting order (front-to-back)
			if (!lstRenderables.empty())
				lstRenderables.sort([](ISpatial* _1, ISpatial* _2) {
					float d1 = _1->GetSpatialData().sphere.P.distance_to_sqr(Device.vCameraPosition);
					float d2 = _2->GetSpatialData().sphere.P.distance_to_sqr(Device.vCameraPosition);
					return d1 < d2;
				});

			// Determine visibility for dynamic part of scene
			set_Object(nullptr);
			u32 uID_LTRACK = 0xffffffff;
			if (phase == PHASE_NORMAL)
			{
				uLastLTRACK++;
				if (lstRenderables.size())
					uID_LTRACK = uLastLTRACK % lstRenderables.size();

				// update light-vis for current entity / actor
				IGameObject* O = g_pGameLevel->CurrentViewEntity();
				if (O)
					if (CROS_impl* R = reinterpret_cast<CROS_impl*>(O->ROS()))
						R->update(O);

				// update light-vis for selected entity
				// track lighting environment
				if (lstRenderables.size())
				{
					uLastLTRACK++;
					uID_LTRACK = uLastLTRACK % lstRenderables.size();

					// update light-vis for selected entity
					// track lighting environment
					if (IRenderable* renderable = reinterpret_cast<IRenderable*>(lstRenderables[uID_LTRACK]->dcast_Renderable()))
						if (CROS_impl* T = reinterpret_cast<CROS_impl*>(renderable->renderable_ROS()))
							T->update(renderable);
				}
			}
		}

		// Traverse sector/portal structure
		if (!dont_test_sectors)
		{
			PortalTraverser.traverse
			(
				pLastSector,
				ViewBase,
				Device.vCameraPosition,
				Device.mFullTransform,
				CPortalTraverser::VQ_HOM + CPortalTraverser::VQ_SSA + CPortalTraverser::VQ_FADE
				//. disabled scissoring (HW.Caps.bScissor?CPortalTraverser::VQ_SCISSOR:0)	// generate scissoring info
			);
		}

		// Determine visibility for static geometry hierrarhy
		if (psDeviceFlags.test(rsDrawStatic))
		{
			if (dont_test_sectors)
			{
				CSector* sector = reinterpret_cast<CSector*>(Sectors[0]);
				set_Frustum(&ViewBase);
				add_Geometry(sector->root());
			}
			else
			{
				for (u32 s_it = 0; s_it < PortalTraverser.r_sectors.size(); s_it++)
				{
					CSector* sector = reinterpret_cast<CSector*>(PortalTraverser.r_sectors[s_it]);
					dxRender_Visual* root = sector->root();
					for (u32 v_it = 0; v_it < sector->r_frustums.size(); v_it++) {
						set_Frustum(&(sector->r_frustums[v_it]));
						add_Geometry(root);
					}
				}
			}
		}

		bool IsCalculateBones{ false };

		if(deffered)
			LIMIT_UPDATE_FPS_CODE(_CalculateBones, 40, IsCalculateBones = true;)

		// Traverse frustums
		for (ISpatial* spatial : lstRenderables)
		{
			if (0 == spatial)
				continue;

			spatial->spatial_updatesector();

			SpatialData& spatial_data = spatial->GetSpatialData();

			CSector* sector = reinterpret_cast<CSector*>(spatial_data.sector);

			if (0 == sector)
				continue;

			static Fbox sp_box;
			sp_box.setb
			(
				spatial_data.sphere.P,
				Fvector{
					spatial_data.sphere.R,
					spatial_data.sphere.R,
					spatial_data.sphere.R
				}
			);
			
			HOM.Enable();
			if (!HOM.visible(sp_box))
				continue;

			if ((spatial_data.type & STYPE_LIGHTSOURCE) && deffered)
			{
				// lightsource
				if (light* L = reinterpret_cast<light*>(spatial->dcast_Light()))
				{
					if (L->get_LOD() > EPS_L)
					{

						if (dont_test_sectors)
							Lights.add_light(L);
						else
						{
							for (u32 s_it = 0; s_it < L->m_sectors.size(); s_it++)
							{
								CSector* sector_ = reinterpret_cast<CSector*>(L->m_sectors[s_it]);
								if (PortalTraverser.i_marker == sector_->r_marker)
								{
									Lights.add_light(L);
									break;
								}
							}
						}
					}
				}
				continue;
			}

			auto CalcSSADynamic = [&](const Fvector& C, float R) -> float
			{
				Fvector4 v_res1, v_res2;
				Device.mFullTransform.transform(v_res1, C);
				Device.mFullTransform.transform(v_res2, Fvector(C).mad(Device.vCameraRight, R));
				return v_res1.sub(v_res2).magnitude();
			};

			constexpr float base_fov = 67.f;
			// Aproximate, adjusted by fov, distance from camera to position (For right work when looking though binoculars and scopes)
			auto GetDistFromCamera = [](const Fvector& from_position) -> float
			{
				float distance = Device.vCameraPosition.distance_to(from_position);
				float fov_K = base_fov / Device.fFOV;
				float adjusted_distane = distance / fov_K;

				return adjusted_distane;
			};

			if (dont_test_sectors)
			{
				if (spatial_data.type & STYPE_RENDERABLE && psDeviceFlags.test(rsDrawDynamic))
				{
					// renderable
					if (IRenderable* renderable = spatial->dcast_Renderable())
					{
						if (Device.vCameraPosition.distance_to_sqr(spatial_data.sphere.P) < _sqr(g_pGamePersistent->Environment().CurrentEnv->fog_distance))
						{
							if (CalcSSADynamic(spatial_data.sphere.P, spatial_data.sphere.R) > 0.002f && GetDistFromCamera(spatial_data.sphere.P) < 220.f)
							{
								if (deffered && IsCalculateBones)
								{
									CKinematics* pKin = reinterpret_cast<CKinematics*>(renderable->GetRenderData().visual);
									if (pKin)
									{
										pKin->CalculateBones(TRUE);
										pKin->CalculateWallmarks();
										//dbg_text_renderer(spatial->spatial.sphere.P);
									}
								}
								if (spatial_data.sphere.R > 1.f)
								{
									// Rendering
									set_Object(renderable);
									renderable->renderable_Render();
									set_Object(nullptr);
								}
							}
							if (spatial_data.sphere.R <= 1.f)
							{
								// Rendering
								set_Object(renderable);
								renderable->renderable_Render();
								set_Object(nullptr);
							}
						}
					}
				}

				if (spatial_data.type & STYPE_PARTICLE && !deffered)
				{
					// renderable
					if (IRenderable* renderable = spatial->dcast_Renderable())
					{
						// Rendering
						set_Object(renderable);
						renderable->renderable_Render();
						set_Object(nullptr);
					}
				}
			}
			else
			{
				if (PortalTraverser.i_marker != sector->r_marker)
					continue;	// inactive (untouched) sector

				for (u32 v_it = 0; v_it < sector->r_frustums.size(); v_it++)
				{
					CFrustum& view = sector->r_frustums[v_it];
					if (!view.testSphere_dirty(spatial_data.sphere.P, spatial_data.sphere.R))	continue;

					if (spatial_data.type & STYPE_RENDERABLE && psDeviceFlags.test(rsDrawDynamic))
					{
						// renderable
						if (IRenderable* renderable = spatial->dcast_Renderable())
						{
							if (Device.vCameraPosition.distance_to_sqr(spatial_data.sphere.P) < _sqr(g_pGamePersistent->Environment().CurrentEnv->fog_distance))
							{
								if (CalcSSADynamic(spatial_data.sphere.P, spatial_data.sphere.R) > 0.002f && GetDistFromCamera(spatial_data.sphere.P) < 220.f)
								{
									if (deffered && IsCalculateBones)
									{
										CKinematics* pKin = (CKinematics*)renderable->GetRenderData().visual;
										if (pKin)
										{
											pKin->CalculateBones(TRUE);
											pKin->CalculateWallmarks();
											//dbg_text_renderer(spatial->spatial.sphere.P);
										}
									}
									if (spatial_data.sphere.R > 1.f)
									{
										// Rendering
										set_Object(renderable);
										renderable->renderable_Render();
										set_Object(nullptr);
									}
								}
								if (spatial_data.sphere.R <= 1.f)
								{
									// Rendering
									set_Object(renderable);
									renderable->renderable_Render();
									set_Object(nullptr);
								}
							}
						}
					}

					if (spatial_data.type & STYPE_PARTICLE && !deffered)
					{
						// renderable
						if (IRenderable* renderable = spatial->dcast_Renderable())
						{
							// Rendering
							set_Object(renderable);
							renderable->renderable_Render();
							set_Object(nullptr);
						}
					}
				}
			}
		}

		if (g_pGameLevel && psDeviceFlags.test(rsDrawDynamic) && (phase == PHASE_NORMAL))
			g_hud->Render_Last();// HUD
	}
	else
	{
		set_Object(nullptr);
		if (g_pGameLevel && psDeviceFlags.test(rsDrawDynamic) && (phase == PHASE_NORMAL))
			g_hud->Render_Last();// HUD
	}
}

extern u32 g_r;
void CRender::Render()
{
	PIX_EVENT(CRender_Render);

	g_r = 1;
	VERIFY(0 == mapDistort.size());

	rmNormal();


	if (!(g_pGameLevel && g_hud))
	{
		Target->u_setrt(Device.dwWidth, Device.dwHeight, HW.pBaseRT, nullptr, nullptr, HW.pBaseZB);
		return;
	}

	if (m_bFirstFrameAfterReset)
	{
		m_bFirstFrameAfterReset = false;
		return;
	}

	// Configure
	RImplementation.o.distortion = FALSE; // disable distorion
	Fcolor sunColor = ((light*)Lights.sun_adapted._get())->color;
	BOOL bSUN = !o.sunstatic && ps_r2_ls_flags.test(R2FLAG_SUN) &&
		(u_diffuse2s(sunColor.r, sunColor.g, sunColor.b) > EPS);

	// HOM
	ViewBase.CreateFromMatrix(Device.mFullTransform, FRUSTUM_P_LRTB + FRUSTUM_P_FAR);
	View = 0;
	if (!ps_r2_ls_flags.test(R2FLAG_EXP_MT_CALC))
	{
		HOM.Enable();
		HOM.Render(ViewBase);
	}

	//******* Z-prefill calc - DEFERRER RENDERER
	Target->phase_scene_prepare();

	//******* Main calc - DEFERRER RENDERER
	// Main calc
	RImplementation.BasicStats.Culling.Begin();
	r_pmask(true, false, true); // enable priority "0",+ capture wmarks
	if (bSUN)
		set_Recorder(&main_coarse_structure);
	else
		set_Recorder(nullptr);

	phase = PHASE_NORMAL;
	render_main(true);
	set_Recorder(nullptr);
	r_pmask(true, false); // disable priority "1"
	RImplementation.BasicStats.Culling.End();

	const BOOL split_the_scene_to_minimize_wait = ps_r2_ls_flags.test(R2FLAG_EXP_SPLIT_SCENE);

	//******* Main render :: PART-0	-- first
	if (!split_the_scene_to_minimize_wait)
	{
		// level, DO NOT SPLIT
		PIX_EVENT_TEXT(L"Deferred Part0: No Split");
		Target->phase_scene_begin();
		r_dsgraph_render_hud();
		r_dsgraph_render_graph(0);
		r_dsgraph_render_lods(true, true);
		if (Details)
			Details->Render();
		Target->phase_scene_end();
	}
	else
	{
		// level, SPLIT
		PIX_EVENT_TEXT(L"Deferred Part0: Split");
		Target->phase_scene_begin();
		r_dsgraph_render_graph(0);
		Target->disable_aniso();
	}

	//******* Occlusion testing of volume-limited light-sources
	Target->phase_occq();
	LP_normal.clear();

	if (RImplementation.o.dx10_msaa)
		RCache.set_ZB(RImplementation.Target->rt_MSAADepth->pZRT);

	{
		PIX_EVENT(DEFER_TEST_LIGHT_VIS);
		// perform tests
		auto count = 0;
		light_Package& LP = Lights.package;

		// stats
		Stats.l_shadowed = LP.v_shadowed.size();
		Stats.l_unshadowed = LP.v_point.size() + LP.v_spot.size();
		Stats.l_total = Stats.l_shadowed + Stats.l_unshadowed;

		// perform tests
		LP_normal.v_point = LP.v_point;
		LP_normal.v_shadowed = LP.v_shadowed;
		LP_normal.v_spot = LP.v_spot;
		LP_normal.vis_prepare();
	}
	LP_normal.sort();

	//******* Main render :: PART-1 (second)
	if (split_the_scene_to_minimize_wait)
	{
		PIX_EVENT(DEFER_PART1_SPLIT);

		// level
		Target->phase_scene_begin();
		r_dsgraph_render_hud();
		r_dsgraph_render_lods(true, true);
		if (Details)
			Details->Render();
		Target->phase_scene_end();
	}

	if (g_hud && g_hud->RenderActiveItemUIQuery())
	{
		Target->phase_wallmarks();
		r_dsgraph_render_hud_ui();
	}

	// Wall marks
	if (Wallmarks)
	{
		PIX_EVENT(DEFER_WALLMARKS);
		Target->phase_wallmarks();
		g_r = 0;
		Wallmarks->Render(); // wallmarks has priority as normal geometry
	}

	// full screen pass to mark msaa-edge pixels in highest stencil bit
	if (RImplementation.o.dx10_msaa)
	{
		PIX_EVENT(MARK_MSAA_EDGES);
		Target->mark_msaa_edges();
	}

	//	TODO: DX10: Implement DX10 rain.
	if (ps_r2_ls_flags.test(R3FLAG_DYN_WET_SURF))
	{
		PIX_EVENT(DEFER_RAIN);
		render_rain();
	}

	// Directional light - fucking sun
	if (bSUN)
	{
		PIX_EVENT(DEFER_SUN);
		RImplementation.Stats.l_visible++;
		render_sun_cascades();

		Target->accum_direct_blend();
	}

	{
		PIX_EVENT(DEFER_SELF_ILLUM);
		Target->phase_accumulator();
		// Render emissive geometry, stencil - write 0x0 at pixel pos
		RCache.set_xform_project(Device.mProject);
		RCache.set_xform_view(Device.mView);
		// Stencil - write 0x1 at pixel pos -
		if (!RImplementation.o.dx10_msaa)
			RCache.set_Stencil(
				TRUE, D3DCMP_ALWAYS, 0x01, 0xff, 0xff, D3DSTENCILOP_KEEP, D3DSTENCILOP_REPLACE, D3DSTENCILOP_KEEP);
		else
			RCache.set_Stencil(
				TRUE, D3DCMP_ALWAYS, 0x01, 0xff, 0x7f, D3DSTENCILOP_KEEP, D3DSTENCILOP_REPLACE, D3DSTENCILOP_KEEP);
		// RCache.set_Stencil
		// (TRUE,D3DCMP_ALWAYS,0x00,0xff,0xff,D3DSTENCILOP_KEEP,D3DSTENCILOP_REPLACE,D3DSTENCILOP_KEEP);
		RCache.set_CullMode(CULL_CCW);
		RCache.set_ColorWriteEnable();
		RImplementation.r_dsgraph_render_emissive();
	}

	// Lighting, non dependant on OCCQ
	{
		PIX_EVENT(DEFER_LIGHT_NO_OCCQ);
		Target->phase_accumulator();
		HOM.Disable();
		LP_normal.vis_update();
		render_lights(LP_normal);
	}

	// Lighting, dependant on OCCQ
	/*{
		PIX_EVENT(DEFER_LIGHT_OCCQ);
		render_lights(LP_pending);
	}*/

	// Postprocess
	{
		PIX_EVENT(DEFER_LIGHT_COMBINE);
		Target->phase_combine();
	}

	VERIFY(0 == mapDistort.size());
}

void CRender::render_forward()
{
	VERIFY(0 == mapDistort.size());
	RImplementation.o.distortion = RImplementation.o.distortion_enabled; // enable distorion

	//******* Main render - second order geometry (the one, that doesn't support deffering)
	//.todo: should be done inside "combine" with estimation of of luminance, tone-mapping, etc.
	{
		// level
		r_pmask(false, true); // enable priority "1"
		phase = PHASE_NORMAL;
		render_main(false); //
		//	Igor: we don't want to render old lods on next frame.
		mapLOD.clear();
		r_dsgraph_render_graph(1); // normal level, secondary priority
		PortalTraverser.fade_render(); // faded-portals
		r_dsgraph_render_sorted(); // strict-sorted geoms
		g_pGamePersistent->Environment().RenderLast(); // rain/thunder-bolts
	}

	RImplementation.o.distortion = FALSE; // disable distorion
}

// После рендера мира и пост-эффектов --#SM+#-- +SecondVP+
void CRender::AfterWorldRender()
{
	if (Device.m_ScopeVP.IsSVPRender())
	{
		ID3DResource* res;
		HW.pBaseRT->GetResource(&res);
		HW.pContext->CopyResource(Target->rt_secondVP->pSurface, res);
	}
}
