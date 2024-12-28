#include "StdAfx.h"
#include "Layers/xrRender/light.h"
#include "xrCDB/Intersect.hpp"

constexpr u64 cullfragments = 4;

void light::vis_prepare()
{
	//	. test is sheduled for future	= keep old result
	//	. test time comes :)
	//		. camera inside light volume	= visible,	shedule for 'small' interval
	//		. perform testing				= ???,		pending

	size_t ms = ::IDevice->TimeGlobal_ms();
	if (ms < vis.ms_test)
		return;

	const float a0 = deg2rad(::IDevice->cast()->fFOV * ::IDevice->cast()->fASPECT / 2.f);
	const float a1 = deg2rad(::IDevice->cast()->fFOV / 2.f);
	const float x0 = VIEWPORT_NEAR / _cos(a0);
	const float x1 = VIEWPORT_NEAR / _cos(a1);
	const float c = _sqrt(x0 * x0 + x1 * x1);
	const float safe_area = _max(_max(VIEWPORT_NEAR, _max(x0, x1)), c);

	// Msg	("sc[%f,%f,%f]/c[%f,%f,%f] - sr[%f]/r[%f]",VPUSH(spatial.center),VPUSH(position),spatial.radius,range);
	// Msg	("dist:%f, sa:%f",::IDevice->cast()->vCameraPosition.distance_to(spatial.center),safe_area);

	if (::IDevice->cast()->vCameraPosition.distance_to(spatial.sphere.P) <= (spatial.sphere.R * 1.01f + safe_area))
	{ // small error
		vis.visible = true;
		vis.pending = false;
		vis.ms_test = ms + 60;
		return;
	}

	// testing
	vis.pending = true;
	xform_calc();
	RCache.set_xform_world(m_xform);
	vis.query_order = RImplementation.occq_begin(vis.query_id);
	//	Hack: Igor. Light is visible if it's frutum is visible. (Only for volumetric)
	//	Hope it won't slow down too much since there's not too much volumetric lights
	//	TODO: sort for performance improvement if this technique hurts
	if ((flags.type == IRender_Light::SPOT) && flags.bShadow && flags.bVolumetric)
		RCache.set_Stencil(FALSE);
	else
		RCache.set_Stencil(TRUE, D3DCMP_LESSEQUAL, 0x01, 0xff, 0x00);
	RImplementation.Target->draw_volume(this);
	RImplementation.occq_end(vis.query_id);
}

void light::vis_update()
{
	//	. not pending	->>> return (early out)
	//	. test-result:	visible:
	//		. shedule for 'large' interval
	//	. test-result:	invisible:
	//		. shedule for 'next-frame' interval

	if (!vis.pending)
	{
		svis.flushoccq();
		return;
	}

	size_t ms = ::IDevice->TimeGlobal_ms();
	if (vis.ms_test > ms)
		return;

	u64 fragments = RImplementation.occq_get(vis.query_id);
	vis.pending = false;
	if (vis.visible = (fragments > cullfragments))
		vis.ms_test = ms + 200;
	else
		vis.ms_test = ms + 40;
}
