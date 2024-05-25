#include "stdafx.h"
#include "Light_Package.h"

void light_Package::clear()
{
	v_point.clear();
	v_spot.clear();
	v_shadowed.clear();
}

void light_Package::sort()
{
	const auto pred_light_cmp = [](const light* l1, const light* l2)
	{
		return l1->range > l2->range;
	};

	// resort lights (pending -> at the end), maintain stable order
	std::stable_sort(v_point.begin(), v_point.end(), pred_light_cmp);
	std::stable_sort(v_spot.begin(), v_spot.end(), pred_light_cmp);
	std::stable_sort(v_shadowed.begin(), v_shadowed.end(), pred_light_cmp);
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

// Получаем ответы от запросов к окклюдеру в обратном порядке, от
// самого свежего запроса, к самому старому. См. комментарии выше.
void light_Package::vis_update()
{
	for (auto it = v_spot.cbegin(); it != v_spot.cend(); it++)
		(*it)->vis_update();

	for (auto it = v_shadowed.cbegin(); it != v_shadowed.cend(); it++)
		(*it)->vis_update();

	for (auto it = v_point.cbegin(); it != v_point.cend(); it++)
		(*it)->vis_update();
}