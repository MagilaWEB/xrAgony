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
	::IDevice->cast()->mFullTransform.transform(v_res, L->position);

	float x = (1.f + v_res.x) / 2.f * (::IDevice->cast()->dwWidth);
	float y = (1.f - v_res.y) / 2.f * (::IDevice->cast()->dwHeight);

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
			if (deffered)
			{
				size_t lock_fps = ::IDevice->cast()->FPS / 7;
				clamp<size_t>(lock_fps, 2, 40);
				LIMIT_UPDATE_FPS_CODE(TEST_Renderables, lock_fps, {
					g_SpatialSpace->q_frustum
					(
						lstRenderables,
						ISpatial_DB::O_ORDERED,
						STYPE_RENDERABLE + STYPE_RENDERABLESHADOW + STYPE_PARTICLE + STYPE_LIGHTSOURCE,
						::IDevice->cast()->ViewFromMatrix
					);
				})
			}

			// (almost) Exact sorting order (front-to-back)
	/*		if (!lstRenderables.empty())
				lstRenderables.sort([](ISpatial* _1, ISpatial* _2) {
					float d1 = _1->GetSpatialData().sphere.P.distance_to_sqr(::IDevice->cast()->vCameraPosition);
					float d2 = _2->GetSpatialData().sphere.P.distance_to_sqr(::IDevice->cast()->vCameraPosition);
					return d1 < d2;
				});*/

			// Determine visibility for dynamic part of scene
			set_Object(nullptr);
			
			if (phase == PHASE_NORMAL)
			{
				uLastLTRACK++;
				// update light-vis for current entity / actor
				if (IGameObject* O = g_pGameLevel->CurrentViewEntity())
					if (CROS_impl* R = reinterpret_cast<CROS_impl*>(O->ROS()))
						R->update(O);

				// update light-vis for selected entity
				// track lighting environment
				if (lstRenderables.size())
				{
					uLastLTRACK++;
					size_t uID_LTRACK = uLastLTRACK % lstRenderables.size();

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
				::IDevice->cast()->ViewFromMatrix,
				::IDevice->cast()->vCameraPosition,
				::IDevice->cast()->mFullTransform,
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
				set_Frustum(&::IDevice->cast()->ViewFromMatrix);
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

		// Traverse frustums
		for (ISpatial* spatial : lstRenderables)
		{
			spatial->spatial_updatesector();

			SpatialData& spatial_data = spatial->GetSpatialData();

			CSector* sector = reinterpret_cast<CSector*>(spatial_data.sector);

			if (!sector)
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

			if (deffered && (spatial_data.type & STYPE_LIGHTSOURCE))
			{
				// lightsource
				if (PortalTraverser.i_marker == sector->r_marker)
				{
					if (light* L = reinterpret_cast<light*>(spatial->dcast_Light()))
						Lights.add_light(L);
				}
				else if (light* L = reinterpret_cast<light*>(spatial->dcast_Light()))
					if (L->position.distance_to_sqr(::IDevice->cast()->vCameraPosition) < (10 + _sqr(L->range)))
						Lights.add_light(L);

				continue;
			}

			if (PortalTraverser.i_marker != sector->r_marker)
				continue;

			auto renderable = spatial->dcast_Renderable();
			if (!renderable)
				continue;

			RenderData& render_data = renderable->GetRenderData();

			extern bool VisibleToRender(IRenderVisual* pVisual, bool isStatic, bool phase_smap, Fmatrix& transform_matrix);

			if(!VisibleToRender(render_data.visual, false, false, render_data.xform))
				continue;

			if (dont_test_sectors)
			{
				if (deffered && spatial_data.type & STYPE_RENDERABLE && psDeviceFlags.test(rsDrawDynamic))
				{
					if (auto pKin = reinterpret_cast<CKinematics*>(render_data.visual))
					{
						pKin->CalculateBones(TRUE);
						pKin->CalculateWallmarks();
						//dbg_text_renderer(spatial->spatial.sphere.P);
					}

					// Rendering
					set_Object(renderable);
					renderable->renderable_Render();
					set_Object(nullptr);
				}
				else if ((!deffered) && spatial_data.type & STYPE_PARTICLE)
				{
					// Rendering
					set_Object(renderable);
					renderable->renderable_Render();
					set_Object(nullptr);
				}
			}
			else
			{
				for (auto& view : sector->r_frustums)
				{
					if (!view.testSphere_dirty(spatial_data.sphere.P, spatial_data.sphere.R))
						continue;

					if (deffered && spatial_data.type & STYPE_RENDERABLE && psDeviceFlags.test(rsDrawDynamic))
					{
						if (auto pKin = reinterpret_cast<CKinematics*>(render_data.visual))
						{
							pKin->CalculateBones(TRUE);
							pKin->CalculateWallmarks();
						}

						// Rendering
						set_Object(renderable);
						renderable->renderable_Render();
						set_Object(nullptr);
					}
					else if ((!deffered) && spatial_data.type & STYPE_PARTICLE)
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
	else
		set_Object(nullptr);
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
		Target->u_setrt(::IDevice->cast()->dwWidth, ::IDevice->cast()->dwHeight, HW.pBaseRT, nullptr, nullptr, HW.pBaseZB);
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
	View = 0;
	HOM.Enable();

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

	//******* Main render :: PART-0	-- first
	// level, SPLIT
	PIX_EVENT_TEXT(L"Deferred Part0: Split");
	Target->phase_scene_begin();
	r_dsgraph_render_graph(0);
	Target->disable_aniso();

	//******* Occlusion testing of volume-limited light-sources
	Target->phase_occq();

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
		Lights.package.vis_prepare();
	}

	if (g_pGameLevel && psDeviceFlags.test(rsDrawDynamic))
		g_hud->Render_Last();// HUD

	//******* Main render :: PART-1 (second)
	PIX_EVENT(DEFER_PART1_SPLIT);

	// level
	Target->phase_scene_begin();
	r_dsgraph_render_hud();
	r_dsgraph_render_lods(true, true);

	if (Details)
		Details->Render();

	Target->phase_scene_end();

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
		RCache.set_xform_project(::IDevice->cast()->mProject);
		RCache.set_xform_view(::IDevice->cast()->mView);
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
		Lights.package.vis_update();
		render_lights(Lights.package);
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
	if (::IDevice->cast()->m_ScopeVP.IsSVPRender())
	{
		ID3DResource* res;
		HW.pBaseRT->GetResource(&res);
		HW.pContext->CopyResource(Target->rt_secondVP->pSurface, res);
	}
}
