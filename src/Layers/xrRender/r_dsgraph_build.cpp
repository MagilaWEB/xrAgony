#include "stdafx.h"

#include "FHierrarhyVisual.h"
#include "SkeletonCustom.h"
#include "xrCore/FMesh.hpp"
#include "xrEngine/IRenderable.h"

#include "FLOD.h"
#include "particlegroup.h"
#include "FTreeVisual.h"
#include "xrEngine/GameFont.h"
#include "xrEngine/PerformanceAlert.hpp"

using namespace R_dsgraph;

////////////////////////////////////////////////////////////////////////////////////////////////////
// Scene graph actual insertion and sorting ////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
float r_ssaDISCARD;
float r_ssaDONTSORT;
float r_ssaLOD_A, r_ssaLOD_B;
float r_ssaGLOD_start, r_ssaGLOD_end;
float r_ssaHZBvsTEX;

ICF float CalcSSA(float& distSQ, Fvector& C, dxRender_Visual* V)
{
	float R = V->vis.sphere.R + 0;
	distSQ = Device.vCameraPosition.distance_to_sqr(C) + EPS;
	return R / distSQ;
}
ICF float CalcSSA(float& distSQ, Fvector& C, float R)
{
	distSQ = Device.vCameraPosition.distance_to_sqr(C) + EPS;
	return R / distSQ;
}

void D3DXRenderBase::r_dsgraph_insert_dynamic(dxRender_Visual* pVisual, Fvector& Center)
{
	CRender& RI = RImplementation;

	if (pVisual->vis.marker == RI.marker)
		return;
	pVisual->vis.marker = RI.marker;

	float distSQ;
	float SSA = CalcSSA(distSQ, Center, pVisual);
	if (SSA <= r_ssaDISCARD)
		return;

	// Distortive geometry should be marked and R2 special-cases it
	// a) Allow to optimize RT order
	// b) Should be rendered to special distort buffer in another pass
	VERIFY(pVisual->shader._get());
	ShaderElement* sh_d = &*pVisual->shader->E[4]; // 4=L_special

	if (RImplementation.o.distortion && sh_d && sh_d->flags.bDistort && pmask[sh_d->flags.iPriority / 2])
	{
		mapSorted_Node* N = mapDistort.insertInAnyWay(distSQ);
		N->second.ssa = SSA;
		N->second.pObject = RI.val_pObject;
		N->second.pVisual = pVisual;
		N->second.Matrix = *RI.val_pTransform;
		N->second.se = sh_d; // 4=L_special
	}

	// Select shader
	ShaderElement* sh = RImplementation.rimp_select_sh_dynamic(pVisual, distSQ);
	if (nullptr == sh)
		return;
	if (!pmask[sh->flags.iPriority / 2])
		return;

	// HUD rendering
	if (RI.val_bHUD)
	{
		if (sh->flags.bStrictB2F)
		{
			if (sh->flags.bEmissive)
			{
				mapSorted_Node* N = mapHUDEmissive.insertInAnyWay(distSQ);
				N->second.ssa = SSA;
				N->second.pObject = RI.val_pObject;
				N->second.pVisual = pVisual;
				N->second.Matrix = *RI.val_pTransform;
				N->second.se = &*pVisual->shader->E[4]; // 4=L_special
			}

			mapSorted_Node* N = mapHUDSorted.insertInAnyWay(distSQ);
			N->second.ssa = SSA;
			N->second.pObject = RI.val_pObject;
			N->second.pVisual = pVisual;
			N->second.Matrix = *RI.val_pTransform;
			N->second.se = sh;
			return;
		}
		else
		{
			mapHUD_Node* N = mapHUD.insertInAnyWay(distSQ);
			N->second.ssa = SSA;
			N->second.pObject = RI.val_pObject;
			N->second.pVisual = pVisual;
			N->second.Matrix = *RI.val_pTransform;
			N->second.se = sh;

			if (sh->flags.bEmissive)
			{
				mapSorted_Node* N = mapHUDEmissive.insertInAnyWay(distSQ);
				N->second.ssa = SSA;
				N->second.pObject = RI.val_pObject;
				N->second.pVisual = pVisual;
				N->second.Matrix = *RI.val_pTransform;
				N->second.se = &*pVisual->shader->E[4]; // 4=L_special
			}
			return;
		}
	}

	// Shadows registering
	if (RI.val_bInvisible)
		return;

	// strict-sorting selection
	if (sh->flags.bStrictB2F)
	{
		mapSorted_Node* N = mapSorted.insertInAnyWay(distSQ);
		N->second.ssa = SSA;
		N->second.pObject = RI.val_pObject;
		N->second.pVisual = pVisual;
		N->second.Matrix = *RI.val_pTransform;
		N->second.se = sh;
		return;
	}

	// Emissive geometry should be marked and R2 special-cases it
	// a) Allow to skeep already lit pixels
	// b) Allow to make them 100% lit and really bright
	// c) Should not cast shadows
	// d) Should be rendered to accumulation buffer in the second pass
	if (sh->flags.bEmissive)
	{
		mapSorted_Node* N = mapEmissive.insertInAnyWay(distSQ);
		N->second.ssa = SSA;
		N->second.pObject = RI.val_pObject;
		N->second.pVisual = pVisual;
		N->second.Matrix = *RI.val_pTransform;
		N->second.se = &*pVisual->shader->E[4]; // 4=L_special
	}

	if (sh->flags.bWmark && pmask_wmark)
	{
		mapSorted_Node* N = mapWmark.insertInAnyWay(distSQ);
		N->second.ssa = SSA;
		N->second.pObject = RI.val_pObject;
		N->second.pVisual = pVisual;
		N->second.Matrix = *RI.val_pTransform;
		N->second.se = sh;
		return;
	}

	// Create common node
	// NOTE: Invisible elements exist only in R1
	_MatrixItem item = { SSA, RI.val_pObject, pVisual, *RI.val_pTransform };

	for (u32 iPass = 0; iPass < sh->passes.size(); ++iPass)
	{
		SPass& pass = *sh->passes[iPass];
		mapMatrix_T& map = mapMatrixPasses[sh->flags.iPriority / 2][iPass];

		mapMatrixVS::value_type* Nvs = map.insert(&*pass.vs);
		mapMatrixGS::value_type* Ngs = Nvs->second.insert(pass.gs->sh);
		mapMatrixPS::value_type* Nps = Ngs->second.insert(pass.ps->sh);

		Nps->second.hs = pass.hs->sh;
		Nps->second.ds = pass.ds->sh;
		mapMatrixCS::value_type* Ncs = Nps->second.mapCS.insert(pass.constants._get());

		mapMatrixStates::value_type* Nstate = Ncs->second.insert(pass.state->state);
		mapMatrixTextures::value_type* Ntex = Nstate->second.insert(pass.T._get());
		mapMatrixItems& items = Ntex->second;
		items.push_back(item);

		// Need to sort for HZB efficient use
		if (SSA > Ntex->second.ssa)
		{
			Ntex->second.ssa = SSA;
			if (SSA > Nstate->second.ssa)
			{
				Nstate->second.ssa = SSA;
				if (SSA > Ncs->second.ssa)
				{
					Ncs->second.ssa = SSA;
					if (SSA > Nps->second.mapCS.ssa)
					{
						Nps->second.mapCS.ssa = SSA;
						if (SSA > Ngs->second.ssa)
						{
							Ngs->second.ssa = SSA;
							if (SSA > Nvs->second.ssa)
							{
								Nvs->second.ssa = SSA;
							}
						}
					}
				}
			}
		}
	}

	if (val_recorder)
	{
		Fbox3 temp;
		Fmatrix& xf = *RI.val_pTransform;
		temp.xform(pVisual->vis.box, xf);
		val_recorder->push_back(temp);
	}
}

void D3DXRenderBase::r_dsgraph_insert_static(dxRender_Visual* pVisual)
{
	CRender& RI = RImplementation;

	if (pVisual->vis.marker == RI.marker)
		return;
	pVisual->vis.marker = RI.marker;

	float distSQ;
	float SSA = CalcSSA(distSQ, pVisual->vis.sphere.P, pVisual);
	if (SSA <= r_ssaDISCARD)
		return;

	// Distortive geometry should be marked and R2 special-cases it
	// a) Allow to optimize RT order
	// b) Should be rendered to special distort buffer in another pass
	VERIFY(pVisual->shader._get());
	ShaderElement* sh_d = &*pVisual->shader->E[4]; // 4=L_special
	if (RImplementation.o.distortion && sh_d && sh_d->flags.bDistort && pmask[sh_d->flags.iPriority / 2])
	{
		mapSorted_Node* N = mapDistort.insertInAnyWay(distSQ);
		N->second.ssa = SSA;
		N->second.pObject = nullptr;
		N->second.pVisual = pVisual;
		N->second.Matrix = Fidentity;
		N->second.se = &*pVisual->shader->E[4]; // 4=L_special
	}

	// Select shader
	ShaderElement* sh = RImplementation.rimp_select_sh_static(pVisual, distSQ);
	if (nullptr == sh)
		return;
	if (!pmask[sh->flags.iPriority / 2])
		return;

	// strict-sorting selection
	if (sh->flags.bStrictB2F)
	{
		mapSorted_Node* N = mapSorted.insertInAnyWay(distSQ);
		N->second.pObject = nullptr;
		N->second.pVisual = pVisual;
		N->second.Matrix = Fidentity;
		N->second.se = sh;
		return;
	}


	// Emissive geometry should be marked and R2 special-cases it
	// a) Allow to skeep already lit pixels
	// b) Allow to make them 100% lit and really bright
	// c) Should not cast shadows
	// d) Should be rendered to accumulation buffer in the second pass
	if (sh->flags.bEmissive)
	{
		mapSorted_Node* N = mapEmissive.insertInAnyWay(distSQ);
		N->second.ssa = SSA;
		N->second.pObject = nullptr;
		N->second.pVisual = pVisual;
		N->second.Matrix = Fidentity;
		N->second.se = &*pVisual->shader->E[4]; // 4=L_special
	}

	if (sh->flags.bWmark && pmask_wmark)
	{
		mapSorted_Node* N = mapWmark.insertInAnyWay(distSQ);
		N->second.ssa = SSA;
		N->second.pObject = nullptr;
		N->second.pVisual = pVisual;
		N->second.Matrix = Fidentity;
		N->second.se = sh;
		return;
	}

	if (val_feedback && counter_S == val_feedback_breakp)
		val_feedback->rfeedback_static(pVisual);

	counter_S++;

	_NormalItem item = { SSA, pVisual };

	for (u32 iPass = 0; iPass < sh->passes.size(); ++iPass)
	{
		SPass& pass = *sh->passes[iPass];
		mapNormal_T& map = mapNormalPasses[sh->flags.iPriority / 2][iPass];

		mapNormalVS::value_type* Nvs = map.insert(&*pass.vs);
		mapNormalGS::value_type* Ngs = Nvs->second.insert(pass.gs->sh);
		mapNormalPS::value_type* Nps = Ngs->second.insert(pass.ps->sh);

		Nps->second.hs = pass.hs->sh;
		Nps->second.ds = pass.ds->sh;
		mapNormalCS::value_type* Ncs = Nps->second.mapCS.insert(pass.constants._get());
		mapNormalStates::value_type* Nstate = Ncs->second.insert(pass.state->state);
		mapNormalTextures::value_type* Ntex = Nstate->second.insert(pass.T._get());
		mapNormalItems& items = Ntex->second;
		_NormalItem item = { SSA, pVisual };
		items.push_back(item);

		// Need to sort for HZB efficient use
		if (SSA > Ntex->second.ssa)
		{
			Ntex->second.ssa = SSA;
			if (SSA > Nstate->second.ssa)
			{
				Nstate->second.ssa = SSA;
				if (SSA > Ncs->second.ssa)
				{
					Ncs->second.ssa = SSA;
					if (SSA > Nps->second.mapCS.ssa)
					{
						Nps->second.mapCS.ssa = SSA;
						if (SSA > Ngs->second.ssa)
						{
							Ngs->second.ssa = SSA;
							if (SSA > Nvs->second.ssa)
								Nvs->second.ssa = SSA;
						}
					}
				}
			}
		}
	}

	if (val_recorder)
		val_recorder->push_back(pVisual->vis.box);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

// Static geometry optimization
#define O_S_L1_S_LOW    10.f // geometry 3d volume size
#define O_S_L1_D_LOW    150.f // distance, after which it is not rendered
#define O_S_L2_S_LOW    100.f
#define O_S_L2_D_LOW    200.f
#define O_S_L3_S_LOW    500.f
#define O_S_L3_D_LOW    250.f
#define O_S_L4_S_LOW    2500.f
#define O_S_L4_D_LOW    350.f
#define O_S_L5_S_LOW    7000.f
#define O_S_L5_D_LOW    400.f

#define O_S_L1_S_MED    25.f
#define O_S_L1_D_MED    50.f
#define O_S_L2_S_MED    200.f
#define O_S_L2_D_MED    150.f
#define O_S_L3_S_MED    1000.f
#define O_S_L3_D_MED    200.f
#define O_S_L4_S_MED    2500.f
#define O_S_L4_D_MED    300.f
#define O_S_L5_S_MED    7000.f
#define O_S_L5_D_MED    400.f

#define O_S_L1_S_HII    50.f
#define O_S_L1_D_HII    50.f
#define O_S_L2_S_HII    400.f
#define O_S_L2_D_HII    150.f
#define O_S_L3_S_HII    1500.f
#define O_S_L3_D_HII    200.f
#define O_S_L4_S_HII    5000.f
#define O_S_L4_D_HII    300.f
#define O_S_L5_S_HII    20000.f
#define O_S_L5_D_HII    350.f

#define O_S_L1_S_ULT    50.f
#define O_S_L1_D_ULT    35.f
#define O_S_L2_S_ULT    500.f
#define O_S_L2_D_ULT    125.f
#define O_S_L3_S_ULT    1750.f
#define O_S_L3_D_ULT    175.f
#define O_S_L4_S_ULT    5250.f
#define O_S_L4_D_ULT    250.f
#define O_S_L5_S_ULT    25000.f
#define O_S_L5_D_ULT    300.f

// Dyn geometry optimization

#define O_D_L1_S_LOW    1.f // geometry 3d volume size
#define O_D_L1_D_LOW    80.f // distance, after which it is not rendered
#define O_D_L2_S_LOW    3.f
#define O_D_L2_D_LOW    150.f
#define O_D_L3_S_LOW    4000.f
#define O_D_L3_D_LOW    250.f

#define O_D_L1_S_MED    1.f
#define O_D_L1_D_MED    40.f
#define O_D_L2_S_MED    4.f
#define O_D_L2_D_MED    100.f
#define O_D_L3_S_MED    4000.f
#define O_D_L3_D_MED    200.f

#define O_D_L1_S_HII    1.4f
#define O_D_L1_D_HII    30.f
#define O_D_L2_S_HII    4.f
#define O_D_L2_D_HII    80.f
#define O_D_L3_S_HII    4000.f
#define O_D_L3_D_HII    150.f

#define O_D_L1_S_ULT    2.0f
#define O_D_L1_D_ULT    30.f
#define O_D_L2_S_ULT    8.f
#define O_D_L2_D_ULT    50.f
#define O_D_L3_S_ULT    4000.f
#define O_D_L3_D_ULT    110.f

Fvector4 o_optimize_static_l1_dist = { O_S_L1_D_LOW, O_S_L1_D_MED, O_S_L1_D_HII, O_S_L1_D_ULT };
Fvector4 o_optimize_static_l1_size = { O_S_L1_S_LOW, O_S_L1_S_MED, O_S_L1_S_HII, O_S_L1_S_ULT };
Fvector4 o_optimize_static_l2_dist = { O_S_L2_D_LOW, O_S_L2_D_MED, O_S_L2_D_HII, O_S_L2_D_ULT };
Fvector4 o_optimize_static_l2_size = { O_S_L2_S_LOW, O_S_L2_S_MED, O_S_L2_S_HII, O_S_L2_S_ULT };
Fvector4 o_optimize_static_l3_dist = { O_S_L3_D_LOW, O_S_L3_D_MED, O_S_L3_D_HII, O_S_L3_D_ULT };
Fvector4 o_optimize_static_l3_size = { O_S_L3_S_LOW, O_S_L3_S_MED, O_S_L3_S_HII, O_S_L3_S_ULT };
Fvector4 o_optimize_static_l4_dist = { O_S_L4_D_LOW, O_S_L4_D_MED, O_S_L4_D_HII, O_S_L4_D_ULT };
Fvector4 o_optimize_static_l4_size = { O_S_L4_S_LOW, O_S_L4_S_MED, O_S_L4_S_HII, O_S_L4_S_ULT };
Fvector4 o_optimize_static_l5_dist = { O_S_L5_D_LOW, O_S_L5_D_MED, O_S_L5_D_HII, O_S_L5_D_ULT };
Fvector4 o_optimize_static_l5_size = { O_S_L5_S_LOW, O_S_L5_S_MED, O_S_L5_S_HII, O_S_L5_S_ULT };

Fvector4 o_optimize_dynamic_l1_dist = { O_D_L1_D_LOW, O_D_L1_D_MED, O_D_L1_D_HII, O_D_L1_D_ULT };
Fvector4 o_optimize_dynamic_l1_size = { O_D_L1_S_LOW, O_D_L1_S_MED, O_D_L1_S_HII, O_D_L1_S_ULT };
Fvector4 o_optimize_dynamic_l2_dist = { O_D_L2_D_LOW, O_D_L2_D_MED, O_D_L2_D_HII, O_D_L2_D_ULT };
Fvector4 o_optimize_dynamic_l2_size = { O_D_L2_S_LOW, O_D_L2_S_MED, O_D_L2_S_HII, O_D_L2_S_ULT };
Fvector4 o_optimize_dynamic_l3_dist = { O_D_L3_D_LOW, O_D_L3_D_MED, O_D_L3_D_HII, O_D_L3_D_ULT };
Fvector4 o_optimize_dynamic_l3_size = { O_D_L3_S_LOW, O_D_L3_S_MED, O_D_L3_S_HII, O_D_L3_S_ULT };

IC float GetDistFromCamera(const Fvector& from_position)
// Aproximate, adjusted by fov, distance from camera to position (For right work when looking though binoculars and scopes)
{
	float distance = Device.vCameraPosition.distance_to(from_position);
	float fov_K = 67.f / Device.fFOV;
	float adjusted_distane = distance / fov_K;

	return adjusted_distane;
}

IC bool IsValuableToRender(dxRender_Visual* pVisual, bool isStatic, bool sm, Fmatrix& transform_matrix, bool ignore_optimize = false)
{
	if (ignore_optimize)
		return true;

	if ((isStatic && opt_static >= 1) || (!isStatic && opt_dynamic >= 1) || opt_shadow >= 1)
	{
		float sphere_volume = pVisual->getVisData().sphere.volume();

		float adjusted_dist = 0;

		if (isStatic)
			adjusted_dist = GetDistFromCamera(pVisual->vis.sphere.P);
		else
			// dynamic geometry position needs to be transformed by transform matrix, to get world coordinates, dont forget ;)
		{
			Fvector pos;
			transform_matrix.transform_tiny(pos, pVisual->vis.sphere.P);

			adjusted_dist = GetDistFromCamera(pos);
		}

		if (sm && opt_shadow >= 1) // Highest cut off for shadow map
		{
			float factor_dist = float(5 - opt_shadow) * 0.8f;
			if (sphere_volume < 50000.f && adjusted_dist >(100.f * factor_dist))
				// don't need geometry behind the farest sun shadow cascade
				return false;

			if ((sphere_volume < o_optimize_static_l1_size.z) && (adjusted_dist > o_optimize_static_l1_dist.z * factor_dist))
				return false;
			else if ((sphere_volume < o_optimize_static_l2_size.z) && (adjusted_dist > o_optimize_static_l2_dist.z * factor_dist))
				return false;
			else if ((sphere_volume < o_optimize_static_l3_size.z) && (adjusted_dist > o_optimize_static_l3_dist.z * factor_dist))
				return false;
			else if ((sphere_volume < o_optimize_static_l4_size.z) && (adjusted_dist > o_optimize_static_l4_dist.z * factor_dist))
				return false;
			else if ((sphere_volume < o_optimize_static_l5_size.z) && (adjusted_dist > o_optimize_static_l5_dist.z * factor_dist))
				return false;
		}

		if (isStatic)
		{
			switch (opt_static)
			{
			case 2:
				if ((sphere_volume < o_optimize_static_l1_size.y) && (adjusted_dist > o_optimize_static_l1_dist.y))
					return false;
				else if ((sphere_volume < o_optimize_static_l2_size.y) && (adjusted_dist > o_optimize_static_l2_dist.y))
					return false;
				else if ((sphere_volume < o_optimize_static_l3_size.y) && (adjusted_dist > o_optimize_static_l3_dist.y))
					return false;
				else if ((sphere_volume < o_optimize_static_l4_size.y) && (adjusted_dist > o_optimize_static_l4_dist.y))
					return false;
				else if ((sphere_volume < o_optimize_static_l5_size.y) && (adjusted_dist > o_optimize_static_l5_dist.y))
					return false;
				break;
			case 3:
				if ((sphere_volume < o_optimize_static_l1_size.z) && (adjusted_dist > o_optimize_static_l1_dist.z))
					return false;
				else if ((sphere_volume < o_optimize_static_l2_size.z) && (adjusted_dist > o_optimize_static_l2_dist.z))
					return false;
				else if ((sphere_volume < o_optimize_static_l3_size.z) && (adjusted_dist > o_optimize_static_l3_dist.z))
					return false;
				else if ((sphere_volume < o_optimize_static_l4_size.z) && (adjusted_dist > o_optimize_static_l4_dist.z))
					return false;
				else if ((sphere_volume < o_optimize_static_l5_size.z) && (adjusted_dist > o_optimize_static_l5_dist.z))
					return false;
				break;
			case 4:
				if ((sphere_volume < o_optimize_static_l1_size.w) && (adjusted_dist > o_optimize_static_l1_dist.w))
					return false;
				else if ((sphere_volume < o_optimize_static_l2_size.w) && (adjusted_dist > o_optimize_static_l2_dist.w))
					return false;
				else if ((sphere_volume < o_optimize_static_l3_size.w) && (adjusted_dist > o_optimize_static_l3_dist.w))
					return false;
				else if ((sphere_volume < o_optimize_static_l4_size.w) && (adjusted_dist > o_optimize_static_l4_dist.w))
					return false;
				else if ((sphere_volume < o_optimize_static_l5_size.w) && (adjusted_dist > o_optimize_static_l5_dist.w))
					return false;
				break;
			default:
				if ((sphere_volume < o_optimize_static_l1_size.x) && (adjusted_dist > o_optimize_static_l1_dist.x))
					return false;
				else if ((sphere_volume < o_optimize_static_l2_size.x) && (adjusted_dist > o_optimize_static_l2_dist.x))
					return false;
				else if ((sphere_volume < o_optimize_static_l3_size.x) && (adjusted_dist > o_optimize_static_l3_dist.x))
					return false;
				else if ((sphere_volume < o_optimize_static_l4_size.x) && (adjusted_dist > o_optimize_static_l4_dist.x))
					return false;
				else if ((sphere_volume < o_optimize_static_l5_size.x) && (adjusted_dist > o_optimize_static_l5_dist.x))
					return false;
			}
		}
		else
		{
			switch (opt_dynamic)
			{
			case 2:
				if ((sphere_volume < o_optimize_dynamic_l1_size.y) && (adjusted_dist > o_optimize_dynamic_l1_dist.y))
					return false;
				else if ((sphere_volume < o_optimize_dynamic_l2_size.y) && (adjusted_dist > o_optimize_dynamic_l2_dist.y))
					return false;
				else if ((sphere_volume < o_optimize_dynamic_l3_size.y) && (adjusted_dist > o_optimize_dynamic_l3_dist.y))
					return false;
				break;
			case 3:
				if ((sphere_volume < o_optimize_dynamic_l1_size.z) && (adjusted_dist > o_optimize_dynamic_l1_dist.z))
					return false;
				else if ((sphere_volume < o_optimize_dynamic_l2_size.z) && (adjusted_dist > o_optimize_dynamic_l2_dist.z))
					return false;
				else if ((sphere_volume < o_optimize_dynamic_l3_size.z) && (adjusted_dist > o_optimize_dynamic_l3_dist.z))
					return false;
				break;
			case 4:
				if ((sphere_volume < o_optimize_dynamic_l1_size.w) && (adjusted_dist > o_optimize_dynamic_l1_dist.w))
					return false;
				else if ((sphere_volume < o_optimize_dynamic_l2_size.w) && (adjusted_dist > o_optimize_dynamic_l2_dist.w))
					return false;
				else if ((sphere_volume < o_optimize_dynamic_l3_size.w) && (adjusted_dist > o_optimize_dynamic_l3_dist.w))
					return false;
				break;
			default:
				if ((sphere_volume < o_optimize_dynamic_l1_size.x) && (adjusted_dist > o_optimize_dynamic_l1_dist.x))
					return false;
				else if ((sphere_volume < o_optimize_dynamic_l2_size.x) && (adjusted_dist > o_optimize_dynamic_l2_dist.x))
					return false;
				else if ((sphere_volume < o_optimize_dynamic_l3_size.x) && (adjusted_dist > o_optimize_dynamic_l3_dist.x))
					return false;
			}
		}
	}

	return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
void CRender::add_leafs_Dynamic(dxRender_Visual* pVisual, bool bIgnoreOpt)
{
	if (!pVisual)
		return;

	if (!IsValuableToRender(pVisual, false, phase == 1, *val_pTransform, bIgnoreOpt))
		return;

	switch (pVisual->Type)
	{
	case MT_PARTICLE_GROUP:
	{
		// Add all children, doesn't perform any tests
		for (PS::CParticleGroup::SItem& I : reinterpret_cast<PS::CParticleGroup*>(pVisual)->items)
		{
			if (I._effect)
				add_leafs_Dynamic(I._effect, bIgnoreOpt);

			for (dxRender_Visual* pit : I._children_related)
				add_leafs_Dynamic(pit, bIgnoreOpt);

			for (dxRender_Visual* pit : I._children_free)
				add_leafs_Dynamic(pit, bIgnoreOpt);
		}
	}
	return;
	case MT_HIERRARHY:
	{
		FHierrarhyVisual* pV = reinterpret_cast<FHierrarhyVisual*>(pVisual);
		for (dxRender_Visual*& Vis : pV->children)
		{
			Vis->vis.obj_data = pV->getVisData().obj_data; // Наследники используют шейдерные данные от родительского визуала
			add_leafs_Dynamic(Vis);
		}
	}
	return;
	case MT_SKELETON_ANIM:
	case MT_SKELETON_RIGID:
	{
		// Add all children, doesn't perform any tests
		CKinematics* pV = reinterpret_cast<CKinematics*>(pVisual);
		bool use_lod = false;
		if (pV->m_lod)
		{
			Fvector Tpos;
			float D;
			val_pTransform->transform_tiny(Tpos, pV->vis.sphere.P);
			float ssa = CalcSSA(D, Tpos, pV->vis.sphere.R / 2.f); // assume dynamics never consume full sphere
			if (ssa < r_ssaLOD_A)
				use_lod = true;
		}

		if (use_lod)
			add_leafs_Dynamic(pV->m_lod, bIgnoreOpt);
		else
		{
			//pV->CalculateBones(TRUE);
			//pV->CalculateWallmarks(); //. bug?
			for (dxRender_Visual* I : pV->children)
			{
				I->vis.obj_data = pV->getVisData().obj_data; // Наследники используют шейдерные данные от родительского визуала
				// [use shader data from parent model, rather than it childrens]
				add_leafs_Dynamic(I);
			}
		}
	}
	return;
	default:
	{
		// General type of visual
		// Calculate distance to it's center
		Fvector Tpos;
		val_pTransform->transform_tiny(Tpos, pVisual->vis.sphere.P);
		r_dsgraph_insert_dynamic(pVisual, Tpos);
	}
	return;
	}
}

void CRender::add_leafs_Static(dxRender_Visual* pVisual)
{
	if (!HOM.visible(pVisual->vis))
		return;

	if (!pVisual->bIgnoreOpt && !IsValuableToRender(pVisual, true, phase == 1, *val_pTransform))
		return;

	switch (pVisual->Type)
	{
	case MT_PARTICLE_GROUP:
	{
		// Add all children, doesn't perform any tests
		for (PS::CParticleGroup::SItem& I : reinterpret_cast<PS::CParticleGroup*>(pVisual)->items)
		{
			if (I._effect)
				add_leafs_Dynamic(I._effect);

			for (dxRender_Visual* pit : I._children_related)
				add_leafs_Dynamic(pit);

			for (dxRender_Visual* pit : I._children_free)
				add_leafs_Dynamic(pit);
		}
	}
	return;
	case MT_HIERRARHY:
	{
		// Add all children, doesn't perform any tests
		FHierrarhyVisual* pV = reinterpret_cast<FHierrarhyVisual*>(pVisual);
		for (dxRender_Visual*& I : pV->children)
		{
			I->vis.obj_data = pV->getVisData().obj_data;// Наследники используют шейдерные данные от родительского визуала
			add_leafs_Static(I);
		}
	}
	return;
	case MT_SKELETON_ANIM:
	case MT_SKELETON_RIGID:
	{
		// Add all children, doesn't perform any tests
		CKinematics* pV = reinterpret_cast<CKinematics*>(pVisual);
		//pV->CalculateBones(TRUE);
		for (dxRender_Visual* I : pV->children)
			add_leafs_Static(I);
	}
	return;
	case MT_LOD:
	{
		FLOD* pV = reinterpret_cast<FLOD*>(pVisual);
		float D;
		float ssa = CalcSSA(D, pV->vis.sphere.P, pV);
		ssa *= pV->lod_factor;
		if (ssa < r_ssaLOD_A)
		{
			if (ssa < r_ssaDISCARD) return;
			mapLOD_Node* N = mapLOD.insertInAnyWay(D);
			N->second.ssa = ssa;
			N->second.pVisual = pVisual;
		}

		if (ssa > r_ssaLOD_B || phase == PHASE_SMAP)
		{
			// Add all children, doesn't perform any tests
			for (dxRender_Visual*& I : pV->children)
			{
				I->vis.obj_data = pV->getVisData().obj_data; // Наследники используют шейдерные данные от родительского визуала
				add_leafs_Static(I);
			}
		}
	}
	return;
	case MT_TREE_PM:
	case MT_TREE_ST:
	{
		// General type of visual
		r_dsgraph_insert_static(pVisual);
	}
	return;
	default:
	{
		// General type of visual
		r_dsgraph_insert_static(pVisual);
	}
	return;
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL CRender::add_Dynamic(dxRender_Visual* pVisual, u32 planes)
{
	if (!pVisual->bIgnoreOpt && !IsValuableToRender(pVisual, false, phase == 1, *val_pTransform))
		return FALSE;

	// Check frustum visibility and calculate distance to visual's center
	Fvector Tpos; // transformed position
	EFC_Visible VIS;

	val_pTransform->transform_tiny(Tpos, pVisual->vis.sphere.P);
	VIS = View->testSphere(Tpos, pVisual->vis.sphere.R, planes);
	if (fcvNone == VIS) return FALSE;

	// If we get here visual is visible or partially visible

	switch (pVisual->Type)
	{
	case MT_PARTICLE_GROUP:
	{
		// Add all children, doesn't perform any tests
		if (fcvPartial == VIS)
		{
			for (PS::CParticleGroup::SItem& I : reinterpret_cast<PS::CParticleGroup*>(pVisual)->items)
			{
				if (I._effect)
					add_Dynamic(I._effect, planes);

				for (dxRender_Visual* pit : I._children_related)
					add_Dynamic(pit, planes);
				for (dxRender_Visual* pit : I._children_free)
					add_Dynamic(pit, planes);
			}
		}
		else
		{
			for (PS::CParticleGroup::SItem& I : reinterpret_cast<PS::CParticleGroup*>(pVisual)->items)
			{
				if (I._effect)
					add_leafs_Dynamic(I._effect);

				for (dxRender_Visual* pit : I._children_related)
					add_leafs_Dynamic(pit);
				for (dxRender_Visual* pit : I._children_free)
					add_leafs_Dynamic(pit);
			}
		}
	}
	break;
	case MT_HIERRARHY:
	{
		// Add all children
		if (fcvPartial == VIS)
			for (dxRender_Visual* I : reinterpret_cast<FHierrarhyVisual*>(pVisual)->children)
				add_Dynamic(I, planes);
		else
			for (dxRender_Visual* I : reinterpret_cast<FHierrarhyVisual*>(pVisual)->children)
				add_leafs_Dynamic(I);
	}
	break;
	case MT_SKELETON_ANIM:
	case MT_SKELETON_RIGID:
	{
		// Add all children, doesn't perform any tests
		CKinematics* pV = reinterpret_cast<CKinematics*>(pVisual);
		bool use_lod = false;
		if (pV->m_lod)
		{
			Fvector Tpos;
			float D;
			val_pTransform->transform_tiny(Tpos, pV->vis.sphere.P);
			float ssa = CalcSSA(D, Tpos, pV->vis.sphere.R / 2.f); // assume dynamics never consume full sphere
			if (ssa < r_ssaLOD_A)
				use_lod = true;
		}

		if (use_lod)
			add_leafs_Dynamic(pV->m_lod);
		else
		{
			//pV->CalculateBones(TRUE);
			//pV->CalculateWallmarks(); //. bug?
			for (dxRender_Visual* I : reinterpret_cast<FHierrarhyVisual*>(pVisual)->children)
				add_leafs_Dynamic(I);
		}
	}
	break;
	default:
	{
		// General type of visual
		r_dsgraph_insert_dynamic(pVisual, Tpos);
	}
	break;
	}
	return TRUE;
}

void CRender::add_Static(dxRender_Visual* pVisual, u32 planes)
{
	if (!pVisual->bIgnoreOpt && !IsValuableToRender(pVisual, true, phase == 1, *val_pTransform))
		return;
	// Check frustum visibility and calculate distance to visual's center
	EFC_Visible VIS;
	vis_data& vis = pVisual->vis;
	VIS = View->testSAABB(vis.sphere.P, vis.sphere.R, vis.box.data(), planes);
	if (VIS == fcvNone)
		return;

	if (!HOM.visible(vis))
		return;

	// If we get here visual is visible or partially visible

	switch (pVisual->Type)
	{
	case MT_PARTICLE_GROUP:
	{
		// Add all children, doesn't perform any tests
		if (fcvPartial == VIS)
		{
			for (PS::CParticleGroup::SItem& I : reinterpret_cast<PS::CParticleGroup*>(pVisual)->items)
			{
				if (I._effect)
					add_Dynamic(I._effect, planes);

				for (dxRender_Visual* childRelated : I._children_related)
					add_Dynamic(childRelated, planes);
				for (dxRender_Visual* childFree : I._children_free)
					add_Dynamic(childFree, planes);
			}
		}
		else
		{
			for (PS::CParticleGroup::SItem& I : reinterpret_cast<PS::CParticleGroup*>(pVisual)->items)
			{
				if (I._effect)
					add_leafs_Dynamic(I._effect);

				for (dxRender_Visual* childRelated : I._children_related)
					add_leafs_Dynamic(childRelated);
				for (dxRender_Visual* childFree : I._children_free)
					add_leafs_Dynamic(childFree);
			}
		}
	}
	break;
	case MT_HIERRARHY:
	{
		// Add all children
		if (VIS == fcvPartial)
			for (dxRender_Visual* childRenderable : reinterpret_cast<FHierrarhyVisual*>(pVisual)->children)
				add_Static(childRenderable, planes);
		else
			for (dxRender_Visual* childRenderable : reinterpret_cast<FHierrarhyVisual*>(pVisual)->children)
				add_leafs_Static(childRenderable);
	}
	break;
	case MT_SKELETON_ANIM:
	case MT_SKELETON_RIGID:
	{
		// Add all children, doesn't perform any tests
		CKinematics* pV = reinterpret_cast<CKinematics*>(pVisual);
		//pV->CalculateBones(TRUE);
		if (VIS == fcvPartial)
			for (dxRender_Visual* childRenderable : pV->children)
				add_Static(childRenderable, planes);
		else
			for (dxRender_Visual* childRenderable : pV->children)
				add_leafs_Static(childRenderable);
	}
	break;
	case MT_LOD:
	{
		FLOD* pV = reinterpret_cast<FLOD*>(pVisual);
		float D;
		float ssa = CalcSSA(D, pV->vis.sphere.P, pV);
		ssa *= pV->lod_factor;
		if (ssa < r_ssaLOD_A)
		{
			if (ssa < r_ssaDISCARD) return;
			mapLOD_Node* N = mapLOD.insertInAnyWay(D);
			N->second.ssa = ssa;
			N->second.pVisual = pVisual;
		}

		if (ssa > r_ssaLOD_B || phase == PHASE_SMAP)
		{
			// Add all children, perform tests
			for (dxRender_Visual* childRenderable : pV->children)
				add_leafs_Static(childRenderable);
		}
	}
	break;
	case MT_TREE_ST:
	case MT_TREE_PM:
	{
		// General type of visual
		r_dsgraph_insert_static(pVisual);
	}
	return;
	default:
	{
		// General type of visual
		r_dsgraph_insert_static(pVisual);
	}
	break;
	}
}

// ============================= NEW MEMBERS ===========================

D3DXRenderBase::D3DXRenderBase()
{
	val_pObject = nullptr;
	val_pTransform = nullptr;
	val_bHUD = FALSE;
	val_bInvisible = FALSE;
	val_bRecordMP = FALSE;
	val_feedback = nullptr;
	val_feedback_breakp = 0;
	val_recorder = nullptr;
	marker = 0;
	r_pmask(true, true);
	b_loaded = FALSE;
	Resources = nullptr;
}

void D3DXRenderBase::Copy(IRender& _in) { *this = *(D3DXRenderBase*)&_in; }
void D3DXRenderBase::setGamma(float fGamma)
{
	m_Gamma.Gamma(fGamma);
}

void D3DXRenderBase::setBrightness(float fGamma)
{
	m_Gamma.Brightness(fGamma);
}

void D3DXRenderBase::setContrast(float fGamma)
{
	m_Gamma.Contrast(fGamma);
}

void D3DXRenderBase::updateGamma()
{
	m_Gamma.Update();
}

void D3DXRenderBase::OnDeviceDestroy(bool bKeepTextures)
{
	m_WireShader.destroy();
	m_SelectionShader.destroy();
	Resources->OnDeviceDestroy(bKeepTextures);
	RCache.OnDeviceDestroy();
}

void D3DXRenderBase::ValidateHW() { HW.Validate(); }
void D3DXRenderBase::DestroyHW()
{
	xr_delete(Resources);
	HW.DestroyDevice();
}

void D3DXRenderBase::Reset(HWND hWnd, u32& dwWidth, u32& dwHeight, float& fWidth_2, float& fHeight_2)
{
#if defined(DEBUG)
	_SHOW_REF("*ref -CRenderDevice::ResetTotal: DeviceREF:", HW.pDevice);
#endif // DEBUG

	Resources->reset_begin();
	Memory.mem_compact();

	ResourcesDeferredUnload();

	HW.Reset(hWnd);

	ResourcesDeferredUpload();

	dwWidth = HW.m_ChainDesc.BufferDesc.Width;
	dwHeight = HW.m_ChainDesc.BufferDesc.Height;

	Device.aspect_ratio = (Device.UI_BASE_WIDTH / Device.UI_BASE_HEIGHT) / (float(dwWidth) / float(dwHeight));

	fWidth_2 = float(dwWidth / 2);
	fHeight_2 = float(dwHeight / 2);
	Resources->reset_end();

#if defined(DEBUG)
	_SHOW_REF("*ref +CRenderDevice::ResetTotal: DeviceREF:", HW.pDevice);
#endif
}

void D3DXRenderBase::SetupStates()
{
	HW.Caps.Update();
	SSManager.SetMaxAnisotropy(4);
	//  TODO: DX10: Implement Resetting of render states into default mode
	// VERIFY(!"D3DXRenderBase::SetupStates not implemented.");
}

void D3DXRenderBase::OnDeviceCreate(const char* shName)
{
	// Signal everyone - device created
	RCache.OnDeviceCreate();
	m_Gamma.Update();
	Resources->OnDeviceCreate(shName);
	create();

	m_WireShader.create("editor\\wire");
	m_SelectionShader.create("editor\\selection");
	DUImpl.OnDeviceCreate();
}

void D3DXRenderBase::Create(HWND hWnd, u32& dwWidth, u32& dwHeight, float& fWidth_2, float& fHeight_2, bool move_window)
{
	HW.CreateDevice(hWnd, move_window);
	dwWidth = HW.m_ChainDesc.BufferDesc.Width;
	dwHeight = HW.m_ChainDesc.BufferDesc.Height;
	Device.aspect_ratio = (Device.UI_BASE_WIDTH / Device.UI_BASE_HEIGHT) / (float(dwWidth) / float(dwHeight));
	fWidth_2 = float(dwWidth / 2);
	fHeight_2 = float(dwHeight / 2);
	Resources = new CResourceManager();
}

void D3DXRenderBase::SetupGPU(bool bForceGPU_SW, bool bForceGPU_NonPure, bool bForceGPU_REF)
{
	HW.Caps.bForceGPU_SW = bForceGPU_SW;
	HW.Caps.bForceGPU_NonPure = bForceGPU_NonPure;
	HW.Caps.bForceGPU_REF = bForceGPU_REF;
}

void D3DXRenderBase::overdrawBegin()
{
	//  TODO: DX10: Implement overdrawBegin
	VERIFY(!"D3DXRenderBase::overdrawBegin not implemented.");
}

void D3DXRenderBase::overdrawEnd()
{
	// TODO: DX10: Implement overdrawEnd
	VERIFY(!"D3DXRenderBase::overdrawBegin not implemented.");
}

void D3DXRenderBase::DeferredLoad(bool E) { Resources->DeferredLoad(E); }
void D3DXRenderBase::ResourcesDeferredUpload() { Resources->DeferredUpload(); }
void D3DXRenderBase::ResourcesDeferredUnload() { Resources->DeferredUnload(); }
void D3DXRenderBase::ResourcesGetMemoryUsage(u32& m_base, u32& c_base, u32& m_lmaps, u32& c_lmaps)
{
	if (Resources)
		Resources->_GetMemoryUsage(m_base, c_base, m_lmaps, c_lmaps);
}

void D3DXRenderBase::ResourcesStoreNecessaryTextures() { Resources->StoreNecessaryTextures(); }
void D3DXRenderBase::ResourcesDumpMemoryUsage() { Resources->_DumpMemoryUsage(); }
DeviceState D3DXRenderBase::GetDeviceState()
{
	HW.Validate();
	//  TODO: DX10: Implement GetDeviceState
	//  TODO: DX10: Implement DXGI_PRESENT_TEST testing
	// VERIFY(!"D3DXRenderBase::overdrawBegin not implemented.");
	return DeviceState::Normal;
}

bool D3DXRenderBase::GetForceGPU_REF() { return HW.Caps.bForceGPU_REF; }
u32 D3DXRenderBase::GetCacheStatPolys() { return RCache.stat.polys; }
void D3DXRenderBase::Begin()
{
	RCache.OnFrameBegin();
	RCache.set_CullMode(CULL_CW);
	RCache.set_CullMode(CULL_CCW);
	if (HW.Caps.SceneMode)
		overdrawBegin();
}

void D3DXRenderBase::Clear()
{
	HW.pContext->ClearDepthStencilView(RCache.get_ZB(), D3D_CLEAR_DEPTH | D3D_CLEAR_STENCIL, 1.0f, 0);
	if (psDeviceFlags.test(rsClearBB))
	{
		FLOAT ColorRGBA[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
		HW.pContext->ClearRenderTargetView(RCache.get_RT(), ColorRGBA);
	}
}

void DoAsyncScreenshot();

void D3DXRenderBase::End()
{
	VERIFY(HW.pDevice);
	if (HW.Caps.SceneMode)
		overdrawEnd();

	RCache.OnFrameEnd();

	if (Device.m_ScopeVP.IsSVPRender())
		return;

	DoAsyncScreenshot();
	extern ENGINE_API u32 state_screen_mode;
	HW.m_pSwapChain->Present((state_screen_mode == 1) && psDeviceFlags.test(rsVSync) ? 1 : 0, 0);
}

void D3DXRenderBase::ResourcesDestroyNecessaryTextures() { Resources->DestroyNecessaryTextures(); }
void D3DXRenderBase::ClearTarget()
{
	FLOAT ColorRGBA[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
	HW.pContext->ClearRenderTargetView(RCache.get_RT(), ColorRGBA);
}

void D3DXRenderBase::SetCacheXform(Fmatrix& mView, Fmatrix& mProject)
{
	RCache.set_xform_view(mView);
	RCache.set_xform_project(mProject);
}

bool D3DXRenderBase::HWSupportsShaderYUV2RGB()
{
	u32 v_dev = CAP_VERSION(HW.Caps.raster_major, HW.Caps.raster_minor);
	u32 v_need = CAP_VERSION(2, 0);
	return v_dev >= v_need;
}

void D3DXRenderBase::OnAssetsChanged()
{
	Resources->m_textures_description.UnLoad();
	Resources->m_textures_description.Load();
}

void D3DXRenderBase::DumpStatistics(IGameFont& font, IPerformanceAlert* alert)
{
	BasicStats.FrameEnd();
	auto renderTotal = Device.GetStats().RenderTotal.result;
#define PPP(a) (100.f * float(a) / renderTotal)
	font.OutNext("*** RENDER:	%2.2fms", renderTotal);
	font.OutNext("Calc:		 %2.2fms, %2.1f%%", BasicStats.Culling.result, PPP(BasicStats.Culling.result));
	font.OutNext("Skeletons:	%2.2fms, %d", BasicStats.Animation.result, BasicStats.Animation.count);
	font.OutNext("Primitives:	%2.2fms, %2.1f%%", BasicStats.Primitives.result, PPP(BasicStats.Primitives.result));
	font.OutNext("Wait-L:		%2.2fms", BasicStats.Wait.result);
	font.OutNext("Skinning:	 %2.2fms", BasicStats.Skinning.result);
	font.OutNext("DT_Vis/Cnt:	%2.2fms/%d", BasicStats.DetailVisibility.result, BasicStats.DetailCount);
	font.OutNext("DT_Render:	%2.2fms", BasicStats.DetailRender.result);
	font.OutNext("DT_Cache:	 %2.2fms", BasicStats.DetailCache.result);
	font.OutNext("Wallmarks:	%2.2fms, %d/%d - %d", BasicStats.Wallmarks.result, BasicStats.StaticWMCount,
		BasicStats.DynamicWMCount, BasicStats.WMTriCount);
	font.OutNext("Glows:		%2.2fms", BasicStats.Glows.result);
	font.OutNext("Lights:		%2.2fms, %d", BasicStats.Lights.result, BasicStats.Lights.count);
	font.OutNext("RT:			%2.2fms, %d", BasicStats.RenderTargets.result, BasicStats.RenderTargets.count);
	font.OutNext("HUD:		  %2.2fms", BasicStats.HUD.result);
	font.OutNext("P_calc:		%2.2fms", BasicStats.Projectors.result);
	font.OutNext("S_calc:		%2.2fms", BasicStats.ShadowsCalc.result);
	font.OutNext("S_render:	 %2.2fms, %d", BasicStats.ShadowsRender.result, BasicStats.ShadowsRender.count);
	u32 occQs = BasicStats.OcclusionQueries ? BasicStats.OcclusionQueries : 1;
	font.OutNext("Occ-query:	%03.1f", 100.f * f32(BasicStats.OcclusionCulled) / occQs);
	font.OutNext("- queries:	%u", BasicStats.OcclusionQueries);
	font.OutNext("- culled:	 %u", BasicStats.OcclusionCulled);
#undef PPP
	font.OutSkip();
	const auto& rcstats = RCache.stat;
	font.OutNext("Vertices:	 %d/%d", rcstats.verts, rcstats.calls ? rcstats.verts / rcstats.calls : 0);
	font.OutNext("Polygons:	 %d/%d", rcstats.polys, rcstats.calls ? rcstats.polys / rcstats.calls : 0);
	font.OutNext("DIP/DP:		%d", rcstats.calls);
#ifdef DEBUG
	font.OutNext("SH/T/M/C:	 %d/%d/%d/%d", rcstats.states, rcstats.textures, rcstats.matrices, rcstats.constants);
	font.OutNext("RT/PS/VS:	 %d/%d/%d", rcstats.target_rt, rcstats.ps, rcstats.vs);
	font.OutNext("DECL/VB/IB:	%d/%d/%d", rcstats.decl, rcstats.vb, rcstats.ib);
#endif
	font.OutNext("XForms:		%d", rcstats.xforms);
	font.OutNext("Static:		%3.1f/%d", rcstats.r.s_static.verts / 1024.f, rcstats.r.s_static.dips);
	font.OutNext("Flora:		%3.1f/%d", rcstats.r.s_flora.verts / 1024.f, rcstats.r.s_flora.dips);
	font.OutNext("- lods:		%3.1f/%d", rcstats.r.s_flora_lods.verts / 1024.f, rcstats.r.s_flora_lods.dips);
	font.OutNext("Dynamic:	  %3.1f/%d", rcstats.r.s_dynamic.verts / 1024.f, rcstats.r.s_dynamic.dips);
	font.OutNext("- sw:		 %3.1f/%d", rcstats.r.s_dynamic_sw.verts / 1024.f, rcstats.r.s_dynamic_sw.dips);
	font.OutNext("- inst:		%3.1f/%d", rcstats.r.s_dynamic_inst.verts / 1024.f, rcstats.r.s_dynamic_inst.dips);
	font.OutNext("- 1B:		 %3.1f/%d", rcstats.r.s_dynamic_1B.verts / 1024.f, rcstats.r.s_dynamic_1B.dips);
	font.OutNext("- 2B:		 %3.1f/%d", rcstats.r.s_dynamic_2B.verts / 1024.f, rcstats.r.s_dynamic_2B.dips);
	font.OutNext("- 3B:		 %3.1f/%d", rcstats.r.s_dynamic_3B.verts / 1024.f, rcstats.r.s_dynamic_3B.dips);
	font.OutNext("- 4B:		 %3.1f/%d", rcstats.r.s_dynamic_4B.verts / 1024.f, rcstats.r.s_dynamic_4B.dips);
	font.OutNext("Details:	  %3.1f/%d", rcstats.r.s_details.verts / 1024.f, rcstats.r.s_details.dips);
	if (alert)
	{
		if (rcstats.verts > 500000)
			alert->Print(font, "Verts	 > 500k: %d", rcstats.verts);
		if (rcstats.calls > 1000)
			alert->Print(font, "DIP/DP	> 1k:	%d", rcstats.calls);
		if (BasicStats.DetailCount > 1000)
			alert->Print(font, "DT_count  > 1000: %u", BasicStats.DetailCount);
	}
	BasicStats.FrameStart();
}
