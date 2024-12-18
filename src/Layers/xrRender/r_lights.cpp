#include "stdafx.h"

void CRender::render_lights(light_Package& LP)
{
	//////////////////////////////////////////////////////////////////////////
	// 1. calculate area + sort in descending order
	tbb::parallel_for_each(LP.v_shadowed, [&](light* L)
	{
		if (L->vis.visible)
			LR.compute_xf_spot(L);
	});

	// 2. infact we could go from the backside and sort in ascending order
	size_t total = LP.v_shadowed.size();

	for (size_t smap = 0; smap != total;)
	{
		LP_smap_pool.initialize(RImplementation.o.smapsize);
		// generate spot shadowmap
		Target->phase_smap_spot_clear();

		std::erase_if(LP.v_shadowed, [&](light*& _light) {
			if (_light->vis.visible)
			{
				SMAP_Rect R{};
				if (LP_smap_pool.push(R, _light->X.S.size))
				{
					// OK
					_light->X.S.posX = R.min.x;
					_light->X.S.posY = R.min.y;

					//////////////////////////////////////////////////////////////////////////
					//		if (has_point_shadowed)		->	generate point shadowmap
					//		if (has_spot_shadowed)		->	generate spot shadowmap
					//		switch-to-accumulator
					//		if (has_point_unshadowed)	-> 	accum point unshadowed
					//		if (has_spot_unshadowed)	-> 	accum spot unshadowed
					//		if (was_point_shadowed)		->	accum point shadowed
					//		if (was_spot_shadowed)		->	accum spot shadowed
					//	if (left_some_lights_that_doesn't cast shadows)
					//		accumulate them
					// render
					phase = PHASE_SMAP;

					r_pmask(true, !!RImplementation.o.Tshadows);

					PIX_EVENT(SHADOWED_LIGHTS_RENDER_SUBSPACE);
					r_dsgraph_render_subspace(_light->spatial.sector, _light->X.S.combine, _light->position, TRUE);
					_light->svis.begin();

					bool bNormal = mapNormalPasses[0][0].size() || mapMatrixPasses[0][0].size();
					bool bSpecial = mapNormalPasses[1][0].size() || mapMatrixPasses[1][0].size() || mapSorted.size();
					if (bNormal || bSpecial)
					{
						Stats.s_merged++;

						Target->phase_smap_spot(_light);
						RCache.set_xform_world(Fidentity);
						RCache.set_xform_view(_light->X.S.view);
						RCache.set_xform_project(_light->X.S.project);
						r_dsgraph_render_graph(0);

						if (ps_r2_ls_flags.test(R2FLAG_SUN_DETAILS))
							Details->Render(CDetailManager::VisiblesType::SHADOW_LIGHT);

						_light->X.S.transluent = false;
						if (bSpecial)
						{
							_light->X.S.transluent = true;
							Target->phase_smap_spot_tsh(_light);
							PIX_EVENT(SHADOWED_LIGHTS_RENDER_GRAPH);
							r_dsgraph_render_graph(1); // normal level, secondary priority
							PIX_EVENT(SHADOWED_LIGHTS_RENDER_SORTED);
							r_dsgraph_render_sorted(); // strict-sorted geoms
						}

						Target->accum_spot(_light);
						if (RImplementation.o.advancedpp && ps_r2_ls_flags.is(R2FLAG_VOLUMETRIC_LIGHTS))
							Target->accum_volumetric(_light);
					}
					else
					{
						Stats.s_finalclip++;
					}
					_light->svis.end();
					r_pmask(true, false);

					smap++;

					return true;
				}
				return false;
			}

			total--;
			return true;
		});

		//		switch-to-accumulator
		Target->phase_accumulator();
	}

	// Point lighting (unshadowed, if left)
	if (!LP.v_point.empty())
	{
		for(light* L2 : LP.v_point)
			if (L2->vis.visible)
				Target->accum_point(L2);
	}

	// Spot lighting (unshadowed, if left)
	if (!LP.v_spot.empty())
	{
		for (light* L2 : LP.v_spot)
		{
			if (L2->vis.visible)
			{
				LR.compute_xf_spot(L2);
				Target->accum_spot(L2);
			}
		}
	}
}
