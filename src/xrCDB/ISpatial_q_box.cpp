#include "stdafx.h"
#include "ISpatial.h"
#include "xrCore/_fbox.h"
extern Fvector c_spatial_offset[8];

template <bool b_first>
struct alignas(16) walker
{
	u32 mask;
	Fvector center;
	Fvector size;
	Fbox box;
	bool is_send_funcion{ false };

	walker(u32 _mask, const Fvector& _center, const Fvector& _size)
	{
		mask = _mask;
		center = _center;
		size = _size;
		box.setb(center, size);
	}

	template <typename TResult>
	void walk(TResult& q_result, ISpatial_NODE* N, Fvector& n_C, float n_R)
	{
		// box
		float n_vR = 2 * n_R;
		Fbox BB;
		BB.set(n_C.x - n_vR, n_C.y - n_vR, n_C.z - n_vR, n_C.x + n_vR, n_C.y + n_vR, n_C.z + n_vR);
		if (!BB.intersect(box))
			return;

		// test items
		for (ISpatial*& S : N->items)
		{
			if (0 == (S->GetSpatialData().type & mask))
				continue;

			Fvector& sC = S->GetSpatialData().sphere.P;
			float sR = S->GetSpatialData().sphere.R;
			Fbox sB;
			sB.set(sC.x - sR, sC.y - sR, sC.z - sR, sC.x + sR, sC.y + sR, sC.z + sR);
			if (!sB.intersect(box))
				continue;

			if constexpr (std::is_same_v<TResult, std::function<void(ISpatial*)>>)
			{
				q_result(S);
				if (is_send_funcion == false)
					is_send_funcion = true;
			}
			else
				q_result.push_back(S);

			if (b_first)
				break;
		}

		// recurse
		float c_R = n_R / 2;
		for (u32 octant = 0; octant < 8; octant++)
		{
			if (0 == N->children[octant])
				continue;
			Fvector c_C;
			c_C.mad(n_C, c_spatial_offset[octant], c_R);
			walk(q_result, N->children[octant], c_C, c_R);

			if constexpr (std::is_same_v<TResult, std::function<void(ISpatial*)>>)
			{
				if (b_first && is_send_funcion)
					break;
			}
			else
			{
				if (b_first && !q_result.empty())
					break;
			}
		}
	}
};

void ISpatial_DB::q_box(xr_vector<ISpatial*>& R, u32 _o, u32 _mask, const Fvector& _center, const Fvector& _size)
{
	Stats.Query.Begin();
	R.clear();
	if (_o & O_ONLYFIRST)
	{
		walker<true> W(_mask, _center, _size);
		W.walk(R, m_root, m_center, m_bounds);
	}
	else
	{
		walker<false> W(_mask, _center, _size);
		W.walk(R, m_root, m_center, m_bounds);
	}
	Stats.Query.End();
}

void ISpatial_DB::q_box_it(std::function<void(ISpatial*)> q_func, u32 _o, u32 _mask, const Fvector& _center, const Fvector& _size)
{
	if (_o & O_ONLYFIRST)
	{
		walker<true> W(_mask, _center, _size);
		W.walk(q_func, m_root, m_center, m_bounds);
	}
	else
	{
		walker<false> W(_mask, _center, _size);
		W.walk(q_func, m_root, m_center, m_bounds);
	}
}

void ISpatial_DB::q_sphere(xr_vector<ISpatial*>& R, u32 _o, u32 _mask, const Fvector& _center, const float _radius)
{
	Fvector _size = {_radius, _radius, _radius};
	q_box(R, _o, _mask, _center, _size);
}

void ISpatial_DB::q_sphere_it(std::function<void(ISpatial*)> q_func, u32 _o, u32 _mask, const Fvector& _center, const float _radius)
{
	Fvector _size = { _radius, _radius, _radius };
	q_box_it(q_func, _o, _mask, _center, _size);
}