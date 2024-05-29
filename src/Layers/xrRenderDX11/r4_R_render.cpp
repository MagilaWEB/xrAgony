#include "stdafx.h"
#include "xrEngine/CustomHUD.h"
#include "xrEngine/IGame_Persistent.h"
#include "xrEngine/xr_object.h"
#include "../xrRender/FBasicVisual.h"
#include "../xrRender/QueryHelper.h"

IC bool pred_sp_sort(ISpatial* _1, ISpatial* _2)
{
	try
	{
		float d1 = _1->GetSpatialData().sphere.P.distance_to_sqr(Device.vCameraPosition);
		float d2 = _2->GetSpatialData().sphere.P.distance_to_sqr(Device.vCameraPosition);
		return d1 < d2;
	}
	catch (...)
	{
		return false;
	}
}

void CRender::render_main(Fmatrix& m_ViewProjection, bool _fportals)
{
	PIX_EVENT(render_main);
	marker++;

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
				STYPE_RENDERABLE + STYPE_LIGHTSOURCE,
				ViewBase
			);

			// (almost) Exact sorting order (front-to-back)
			lstRenderables.sort(pred_sp_sort);

			// Determine visibility for dynamic part of scene
			set_Object(0);
			u32 uID_LTRACK = 0xffffffff;
			if (phase == PHASE_NORMAL)
			{
				uLastLTRACK++;
				if (lstRenderables.size())
					uID_LTRACK = uLastLTRACK % lstRenderables.size();

				// update light-vis for current entity / actor
				IGameObject* O = g_pGameLevel->CurrentViewEntity();
				if (O)
				{
					CROS_impl* R = (CROS_impl*)O->ROS();
					if (R)
						R->update(O);
				}

				// update light-vis for selected entity
				// track lighting environment
				if (lstRenderables.size())
				{
					IRenderable* renderable = lstRenderables[uID_LTRACK]->dcast_Renderable();
					if (renderable)
					{
						CROS_impl* T = (CROS_impl*)renderable->renderable_ROS();
						if (T)
							T->update(renderable);
					}
				}
			}
		}

		// Traverse sector/portal structure
		PortalTraverser.traverse(pLastSector, ViewBase, Device.vCameraPosition, m_ViewProjection,
			CPortalTraverser::VQ_HOM + CPortalTraverser::VQ_SSA + CPortalTraverser::VQ_FADE
			//. disabled scissoring (HW.Caps.bScissor?CPortalTraverser::VQ_SCISSOR:0)	// generate scissoring info
		);

		// Determine visibility for static geometry hierrarhy
		for (u32 s_it = 0; s_it < PortalTraverser.r_sectors.size(); s_it++)
		{
			CSector* sector = (CSector*)PortalTraverser.r_sectors[s_it];
			dxRender_Visual* root = sector->root();
			for (u32 v_it = 0; v_it < sector->r_frustums.size(); v_it++)
			{
				set_Frustum(&(sector->r_frustums[v_it]));
				add_Geometry(root);
			}
		}

		// Traverse frustums
		for (u32 o_it = 0; o_it < lstRenderables.size(); o_it++)
		{
			ISpatial* spatial = lstRenderables[o_it];
			spatial->spatial_updatesector();
			CSector* sector = (CSector*)spatial->GetSpatialData().sector;
			if (0 == sector)
				continue; // disassociated from S/P structure

			if (spatial->GetSpatialData().type & STYPE_LIGHTSOURCE)
			{
				// lightsource
				light* L = (light*)(spatial->dcast_Light());
				VERIFY(L);
				float lod = L->get_LOD();
				if (lod > EPS_L)
				{
					vis_data& vis = L->get_homdata();
					if (HOM.visible(vis))
						Lights.add_light(L);
				}
				continue;
			}

			if (PortalTraverser.i_marker != sector->r_marker)
				continue; // inactive (untouched) sector
			for (u32 v_it = 0; v_it < sector->r_frustums.size(); v_it++)
			{
				CFrustum& view = sector->r_frustums[v_it];
				if (!view.testSphere_dirty(spatial->GetSpatialData().sphere.P, spatial->GetSpatialData().sphere.R))
					continue;

				if (spatial->GetSpatialData().type & STYPE_RENDERABLE)
				{
					// renderable
					IRenderable* renderable = spatial->dcast_Renderable();
					VERIFY(renderable);

					// Occlusion
					//	casting is faster then using getVis method
					vis_data& v_orig = ((dxRender_Visual*)renderable->GetRenderData().visual)->vis;
					vis_data v_copy = v_orig;
					v_copy.box.xform(renderable->GetRenderData().xform);
					BOOL bVisible = HOM.visible(v_copy);
					v_orig.marker = v_copy.marker;
					v_orig.accept_frame = v_copy.accept_frame;
					v_orig.hom_frame = v_copy.hom_frame;
					v_orig.hom_tested = v_copy.hom_tested;
					if (!bVisible)
						break; // exit loop on frustums

					// Rendering
					set_Object(renderable);
					renderable->renderable_Render();
					set_Object(0);
				}
				break; // exit loop on frustums
			}
		}
		if (g_pGameLevel && (phase == PHASE_NORMAL))
			g_hud->Render_Last(); // HUD
	}
	else
	{
		set_Object(0);
		if (g_pGameLevel && (phase == PHASE_NORMAL))
			g_hud->Render_Last(); // HUD
	}
}

extern u32 g_r;
void CRender::Render()
{
	PIX_EVENT(CRender_Render);

	g_r = 1;
	VERIFY(0 == mapDistort.size());

	rmNormal();

	IMainMenu* pMainMenu = g_pGamePersistent ? g_pGamePersistent->m_pMainMenu : 0;
	bool bMenu = pMainMenu ? pMainMenu->CanSkipSceneRendering() : false;

	if (!(g_pGameLevel && g_hud) || bMenu)
	{
		Target->u_setrt(Device.dwWidth, Device.dwHeight, HW.pBaseRT, NULL, NULL, HW.pBaseZB);
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
	if (ps_r2_ls_flags.test(R2FLAG_ZFILL))
	{
		PIX_EVENT(DEFER_Z_FILL);
		RImplementation.BasicStats.Culling.Begin();
		float z_distance = ps_r2_zfill;
		Fmatrix m_zfill, m_project;
		m_project.build_projection(deg2rad(Device.fFOV), Device.fASPECT, VIEWPORT_NEAR,
			z_distance * g_pGamePersistent->Environment().CurrentEnv->far_plane);
		m_zfill.mul(m_project, Device.mView);
		r_pmask(true, false); // enable priority "0"
		set_Recorder(NULL);
		phase = PHASE_SMAP;
		render_main(m_zfill, false);
		r_pmask(true, false); // disable priority "1"
		RImplementation.BasicStats.Culling.End();

		// flush
		Target->phase_scene_prepare();
		RCache.set_ColorWriteEnable(FALSE);
		r_dsgraph_render_graph(0);
		RCache.set_ColorWriteEnable();
	}
	else
	{
		Target->phase_scene_prepare();
	}

	//******* Main calc - DEFERRER RENDERER
	// Main calc
	RImplementation.BasicStats.Culling.Begin();
	r_pmask(true, false, true); // enable priority "0",+ capture wmarks
	if (bSUN)
		set_Recorder(&main_coarse_structure);
	else
		set_Recorder(NULL);
	phase = PHASE_NORMAL;
	render_main(Device.mFullTransform, true);
	set_Recorder(NULL);
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
	//LP_pending.clear();
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
		render_main(Device.mFullTransform, false); //
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
