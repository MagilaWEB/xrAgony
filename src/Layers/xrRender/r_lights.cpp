#include "stdafx.h"

void CRender::render_lights(light_Package& LP)
{
	//////////////////////////////////////////////////////////////////////////
	// Refactor order based on ability to pack shadow-maps
	// 1. calculate area + sort in descending order
	// const	u16		smap_unassigned		= u16(-1);
	tbb::parallel_for_each(LP.v_shadowed, [&](light* L)
	{
		if (L->vis.visible)
			LR.compute_xf_spot(L);
	});

	// 2. refactor - infact we could go from the backside and sort in ascending order
	{
		static xr_vector<light*> refactored;
		size_t total = LP.v_shadowed.size();
		refactored.reserve(total);

		for (size_t smap_ID = 0; refactored.size() != total; ++smap_ID)
		{
			LP_smap_pool.initialize(RImplementation.o.smapsize);

			std::erase_if(LP.v_shadowed, [&](light*& _light) {
				if (_light->vis.visible)
				{
					SMAP_Rect R{};
					if (LP_smap_pool.push(R, _light->X.S.size))
					{
						// OK
						_light->X.S.posX = R.min.x;
						_light->X.S.posY = R.min.y;
						_light->vis.smap_ID = smap_ID;
						refactored.push_back(std::move(_light));
						return true;
					}
					return false;
				}

				total--;
				return true;
			});
		}

		// save (lights are popped from back)
		LP.v_shadowed = std::move(refactored);
		refactored.clear();
	}

	//////////////////////////////////////////////////////////////////////////
	// sort lights by importance???
	// while (has_any_lights_that_cast_shadows) {
	//		if (has_point_shadowed)		->	generate point shadowmap
	//		if (has_spot_shadowed)		->	generate spot shadowmap
	//		switch-to-accumulator
	//		if (has_point_unshadowed)	-> 	accum point unshadowed
	//		if (has_spot_unshadowed)	-> 	accum spot unshadowed
	//		if (was_point_shadowed)		->	accum point shadowed
	//		if (was_spot_shadowed)		->	accum spot shadowed
	//	}
	//	if (left_some_lights_that_doesn't cast shadows)
	//		accumulate them
	while (LP.v_shadowed.size())
	{
		// if (has_spot_shadowed)
		static xr_vector<light*> L_spot_s;
		Stats.s_used++;

		// generate spot shadowmap
		Target->phase_smap_spot_clear();
		light* L = LP.v_shadowed.back();
		u16 sid = L->vis.smap_ID;
		do
		{
			if (LP.v_shadowed.empty())
				break;
			L = LP.v_shadowed.back();
			if (L->vis.smap_ID != sid)
				break;
			LP.v_shadowed.pop_back();

			// render
			phase = PHASE_SMAP;

			r_pmask(true, !!RImplementation.o.Tshadows);

			L->svis.begin();
			PIX_EVENT(SHADOWED_LIGHTS_RENDER_SUBSPACE);
			r_dsgraph_render_subspace(L->spatial.sector, L->X.S.combine, L->position, TRUE);
			bool bNormal = mapNormalPasses[0][0].size() || mapMatrixPasses[0][0].size();
			bool bSpecial = mapNormalPasses[1][0].size() || mapMatrixPasses[1][0].size() || mapSorted.size();
			if (bNormal || bSpecial)
			{
				Stats.s_merged++;
				L_spot_s.push_back(L);
				Target->phase_smap_spot(L);
				RCache.set_xform_world(Fidentity);
				RCache.set_xform_view(L->X.S.view);
				RCache.set_xform_project(L->X.S.project);
				r_dsgraph_render_graph(0);

				if (ps_r2_ls_flags.test(R2FLAG_SUN_DETAILS))
					Details->Render(CDetailManager::VisiblesType::SHADOW_LIGHT);

				L->X.S.transluent = FALSE;
				if (bSpecial)
				{
					L->X.S.transluent = TRUE;
					Target->phase_smap_spot_tsh(L);
					PIX_EVENT(SHADOWED_LIGHTS_RENDER_GRAPH);
					r_dsgraph_render_graph(1); // normal level, secondary priority
					PIX_EVENT(SHADOWED_LIGHTS_RENDER_SORTED);
					r_dsgraph_render_sorted(); // strict-sorted geoms
				}
			}
			else
			{
				Stats.s_finalclip++;
			}
			L->svis.end();
			r_pmask(true, false);
		} while (true);

		//		switch-to-accumulator
		Target->phase_accumulator();

		while(!LP.v_point.empty())
		{
			light* L2 = LP.v_point.back();
			LP.v_point.pop_back();
			if (L2->vis.visible)
				Target->accum_point(L2);
		}

		//		if (has_spot_unshadowed)	-> 	accum spot unshadowed
		while (!LP.v_point.empty())
		{
			light* L2 = LP.v_spot.back();
			LP.v_spot.pop_back();
			if (L2->vis.visible)
			{
				LR.compute_xf_spot(L2);
				Target->accum_spot(L2);
			}
		}

		//		if (was_spot_shadowed)		->	accum spot shadowed
		if (!L_spot_s.empty())
		{
			for (light* p_light : L_spot_s)
				Target->accum_spot(p_light);

			if (RImplementation.o.advancedpp && ps_r2_ls_flags.is(R2FLAG_VOLUMETRIC_LIGHTS))
				for (light* p_light : L_spot_s)
					Target->accum_volumetric(p_light);

			L_spot_s.clear();
		}
	}
}
