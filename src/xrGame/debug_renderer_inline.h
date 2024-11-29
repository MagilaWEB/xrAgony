////////////////////////////////////////////////////////////////////////////
//	Module 		: debug_renderer_inline.h
//	Created 	: 19.06.2006
//  Modified 	: 19.06.2006
//	Author		: Dmitriy Iassenev
//	Description : debug renderer inline functions
////////////////////////////////////////////////////////////////////////////

#pragma once

IC void CDebugRenderer::render()
{
	::DRender->Render();
	for (auto & [pos, color] : dbg_position_render_ui)
	{
		UI().Font().pFontArial14->SetAligment(CGameFont::alCenter);
		UI().Font().pFontArial14->SetColor(color);
		UI().Font().pFontArial14->Out(pos.x, pos.y, "+");
		UI().Font().pFontArial14->OnRender();
	}

	dbg_position_render_ui.clear();
}
IC void CDebugRenderer::draw_line(
	const Fmatrix& matrix, const Fvector& vertex0, const Fvector& vertex1, const u32& color)
{
	Fvector vertices[2] = {vertex0, vertex1};
	u16 indices[2] = {0, 1};
	add_lines(
		&vertices[0], sizeof(vertices) / sizeof(Fvector), &indices[0], sizeof(indices) / (2 * sizeof(u16)), color);
}

IC void CDebugRenderer::draw_aabb(const Fvector& center, const float& half_radius_x, const float& half_radius_y,
	const float& half_radius_z, const u32& color)
{
	Fvector half_radius;
	half_radius.set(half_radius_x, half_radius_y, half_radius_z);

	Fmatrix matrix = ::IDevice->cast()->mInvView;
	matrix.translate_over(center);

	draw_obb(matrix, half_radius, color);
}
