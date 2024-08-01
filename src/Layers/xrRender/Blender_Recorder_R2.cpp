#include "stdafx.h"
#pragma hdrstop

#include "ResourceManager.h"
#include "blenders/Blender_Recorder.h"
#include "blenders/Blender.h"

void fix_texture_name(LPSTR fn);

void CBlender_Compile::r_Pass(LPCSTR _vs, LPCSTR _ps, bool bFog, BOOL bZtest, BOOL bZwrite, BOOL bABlend,
	D3DBLEND abSRC, D3DBLEND abDST, BOOL aTest, u32 aRef)
{
	RS.Invalidate();
	ctable.clear();
	passTextures.clear();
	passMatrices.clear();
	passConstants.clear();
	dwStage = 0;

	// Setup FF-units (Z-buffer, blender)
	PassSET_ZB(bZtest, bZwrite);
	PassSET_Blend(bABlend, abSRC, abDST, aTest, aRef);
	PassSET_LightFog(FALSE, bFog);

	// Create shaders
	SPS* ps = RImplementation.Resources->_CreatePS(_ps);
	SVS* vs = RImplementation.Resources->_CreateVS(_vs);
	dest.ps = ps;
	dest.vs = vs;

	SGS* gs = RImplementation.Resources->_CreateGS("null");
	dest.gs = gs;
	dest.hs = RImplementation.Resources->_CreateHS("null");
	dest.ds = RImplementation.Resources->_CreateDS("null");
	dest.cs = RImplementation.Resources->_CreateCS("null");

	ctable.merge(&ps->constants);
	ctable.merge(&vs->constants);

	// Last Stage - disable
	if (0 == xr_stricmp(_ps, "null"))
	{
		RS.SetTSS(0, D3DTSS_COLOROP, D3DTOP_DISABLE);
		RS.SetTSS(0, D3DTSS_ALPHAOP, D3DTOP_DISABLE);
	}
}

void CBlender_Compile::r_Constant(LPCSTR name, R_constant_setup* s)
{
	R_ASSERT(s);
	ref_constant C = ctable.get(name);
	if (C)
		C->handler = s;
}

void CBlender_Compile::r_ColorWriteEnable(bool cR, bool cG, bool cB, bool cA)
{
	BYTE Mask = 0;
	Mask |= cR ? D3DCOLORWRITEENABLE_RED : 0;
	Mask |= cG ? D3DCOLORWRITEENABLE_GREEN : 0;
	Mask |= cB ? D3DCOLORWRITEENABLE_BLUE : 0;
	Mask |= cA ? D3DCOLORWRITEENABLE_ALPHA : 0;

	RS.SetRS(D3DRS_COLORWRITEENABLE, Mask);
	RS.SetRS(D3DRS_COLORWRITEENABLE1, Mask);
	RS.SetRS(D3DRS_COLORWRITEENABLE2, Mask);
	RS.SetRS(D3DRS_COLORWRITEENABLE3, Mask);
}
