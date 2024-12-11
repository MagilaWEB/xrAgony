#include "stdafx.h"
#include "ISpatial.h"
#include "Frustum.h"
#include "xrCore/_fbox.h"

extern Fvector c_spatial_offset[8];

struct alignas(16) walker
{
	u32 mask;
	CFrustum* F;

	walker(u32 _mask, const CFrustum* _F)
	{
		mask = _mask;
		F = (CFrustum*)_F;
	}
	template <typename TResult>
	void walk(TResult& q_result, ISpatial_NODE* N, Fvector& n_C, float n_R, u32 fmask)
	{
		// box
		float n_vR = 2 * n_R;
		Fbox BB;
		BB.set(n_C.x - n_vR, n_C.y - n_vR, n_C.z - n_vR, n_C.x + n_vR, n_C.y + n_vR, n_C.z + n_vR);
		if (fcvNone == F->testAABB(BB.data(), fmask))
			return;

		// test items
		for (auto& S : N->items)
		{
			if (0 == (S->GetSpatialData().type & mask))
				continue;

			Fvector& sC = S->GetSpatialData().sphere.P;
			float sR = S->GetSpatialData().sphere.R;
			u32 tmask = fmask;
			if (fcvNone == F->testSphere(sC, sR, tmask))
				continue;

			if constexpr (std::is_same_v<TResult, std::function<void(ISpatial*)>>)
				q_result(S);
			else
				q_result.push_back(S);
		}

		// recurse
		float c_R = n_R / 2;
		for (u32 octant = 0; octant < 8; octant++)
		{
			if (0 == N->children[octant])
				continue;
			Fvector c_C;
			c_C.mad(n_C, c_spatial_offset[octant], c_R);
			walk(q_result, N->children[octant], c_C, c_R, fmask);
		}
	}
};

void ISpatial_DB::q_frustum(xr_vector<ISpatial*>& R, u32 _o, u32 _mask, const CFrustum& _frustum)
{
	Stats.Query.Begin();
	R.clear();
	walker W(_mask, &_frustum);
	W.walk(R, m_root, m_center, m_bounds, _frustum.getMask());
	Stats.Query.End();
}

void ISpatial_DB::q_frustum_it(std::function<void(ISpatial*)> q_func, u32 _o, u32 _mask, const CFrustum& _frustum)
{
	walker W(_mask, &_frustum);
	W.walk(q_func, m_root, m_center, m_bounds, _frustum.getMask());
}