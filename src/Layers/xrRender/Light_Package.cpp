#include "stdafx.h"
#include "Light_Package.h"

void light_Package::clear()
{
	v_point.clear();
	v_spot.clear();
	v_shadowed.clear();
}

void light_Package::vis_prepare()
{
	for (light* L : v_point)
		L->vis_prepare();
	for (light* L : v_shadowed)
		L->vis_prepare();
	for (light* L : v_spot)
		L->vis_prepare();
}

void light_Package::vis_update()
{
	for (light* L : v_spot)
		L->vis_update();
	for (light* L : v_shadowed)
		L->vis_update();
	for (light* L : v_point)
		L->vis_update();
}