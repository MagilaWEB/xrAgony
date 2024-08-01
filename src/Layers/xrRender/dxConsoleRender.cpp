#include "stdafx.h"
#include "dxConsoleRender.h"
#include "FVF.h"

dxConsoleRender::dxConsoleRender()
{
	m_Shader.create("hud\\crosshair");
	m_Geom.create(FVF::F_TL, RCache.Vertex.Buffer(), RCache.QuadIB);
}

void dxConsoleRender::Copy(IConsoleRender& _in) { *this = *(dxConsoleRender*)&_in; }
void dxConsoleRender::OnRender(bool bGame)
{
	VERIFY(HW.pDevice);

	D3DRECT R = { 0, 0, static_cast<LONG>(Device.dwWidth), static_cast<LONG>(Device.dwHeight) };
	if (bGame)
		R.y2 /= 2;

	u32 vOffset = 0;
	//	TODO: DX10: Implement console background clearing for DX10
	FVF::TL* verts = (FVF::TL*)RCache.Vertex.Lock(4, m_Geom->vb_stride, vOffset);
	verts->set(float(R.x1), float(R.y2), color_xrgb(32, 32, 32), 0, 0);
	verts++;
	verts->set(float(R.x1), float(R.y1), color_xrgb(32, 32, 32), 0, 0);
	verts++;
	verts->set(float(R.x2), float(R.y2), color_xrgb(32, 32, 32), 0, 0);
	verts++;
	verts->set(float(R.x2), float(R.y1), color_xrgb(32, 32, 32), 0, 0);
	verts++;
	RCache.Vertex.Unlock(4, m_Geom->vb_stride);

	RCache.set_Element(m_Shader->E[0]);
	RCache.set_Geometry(m_Geom);

	RCache.Render(D3DPT_TRIANGLELIST, vOffset, 0, 4, 0, 2);
}
