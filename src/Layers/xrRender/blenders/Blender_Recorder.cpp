// Blender_Recorder.cpp: implementation of the CBlender_Compile class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#pragma hdrstop

#include "Layers/xrRender/ResourceManager.h"
#include "Blender_Recorder.h"
#include "Blender.h"

void fix_texture_name(LPSTR fn);

static int ParseName(LPCSTR N)
{
	if (0 == xr_strcmp(N, "$null"))
		return -1;
	if (0 == xr_strcmp(N, "$base0"))
		return 0;
	if (0 == xr_strcmp(N, "$base1"))
		return 1;
	if (0 == xr_strcmp(N, "$base2"))
		return 2;
	if (0 == xr_strcmp(N, "$base3"))
		return 3;
	if (0 == xr_strcmp(N, "$base4"))
		return 4;
	if (0 == xr_strcmp(N, "$base5"))
		return 5;
	if (0 == xr_strcmp(N, "$base6"))
		return 6;
	if (0 == xr_strcmp(N, "$base7"))
		return 7;
	return -1;
}

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CBlender_Compile::CBlender_Compile() {}
CBlender_Compile::~CBlender_Compile() {}
void CBlender_Compile::_cpp_Compile(ShaderElement* _SH)
{
	SH = _SH;
	RS.Invalidate();

	//	TODO: Check if we need such wired system for
	//	base texture name detection. Perhapse it's done for
	//	optimization?

	// Analyze possibility to detail this shader
	detail_texture = nullptr;
	detail_scaler = nullptr;
	LPCSTR base = nullptr;
	if (bDetail && BT->canBeDetailed())
	{
		//
		sh_list& lst = L_textures;
		int id = ParseName(BT->oT_Name);
		base = BT->oT_Name;
		if (id >= 0)
		{
			if (id >= int(lst.size()))
				xrDebug::Fatal(DEBUG_INFO, "Not enought textures for shader. Base texture: '%s'.", *lst[0]);
			base = *lst[id];
		}
		if (!RImplementation.Resources->m_textures_description.GetDetailTexture(base, detail_texture, detail_scaler))
			bDetail = false;
	}
	else
	{
		////////////////////
		//	Igor
		//	Need this to correct base to detect steep parallax.
		if (BT->canUseSteepParallax())
		{
			sh_list& lst = L_textures;
			int id = ParseName(BT->oT_Name);
			base = BT->oT_Name;
			if (id >= 0)
			{
				if (id >= int(lst.size()))
					xrDebug::Fatal(DEBUG_INFO, "Not enought textures for shader. Base texture: '%s'.", *lst[0]);
				base = *lst[id];
			}
		}
		//	Igor
		////////////////////

		bDetail = false;
	}

	// Validate for R1 or R2
	bDetail_Diffuse = false;
	bDetail_Bump = false;

	if (bDetail)
	{
		RImplementation.Resources->m_textures_description.GetTextureUsage(base, bDetail_Diffuse, bDetail_Bump);

		//	Detect the alowance of detail bump usage here.
		if (!(RImplementation.o.advancedpp && ps_r2_ls_flags.test(R2FLAG_DETAIL_BUMP)))
		{
			bDetail_Diffuse |= bDetail_Bump;
			bDetail_Bump = false;
		}
	}

	bUseSteepParallax =
		RImplementation.Resources->m_textures_description.UseSteepParallax(base) && BT->canUseSteepParallax();
	/*
		if (DEV->m_textures_description.UseSteepParallax(base))
		{
			bool bSteep = BT->canUseSteepParallax();
			DEV->m_textures_description.UseSteepParallax(base);
			bUseSteepParallax = true;
		}
	*/
	TessMethod = 0;

	// Compile
	BT->Compile(*this);
}

void CBlender_Compile::SetParams(int iPriority, bool bStrictB2F)
{
	SH->flags.iPriority = iPriority;
	SH->flags.bStrictB2F = bStrictB2F;
	if (bStrictB2F)
	{
#ifdef _EDITOR
		if (1 != (SH->flags.iPriority / 2))
		{
			Log("!If StrictB2F true then Priority must div 2.");
			SH->flags.bStrictB2F = FALSE;
		}
#else
		VERIFY(1 == (SH->flags.iPriority / 2));
#endif
	}
	// SH->Flags.bLighting		= FALSE;
}

//
void CBlender_Compile::PassBegin()
{
	RS.Invalidate();
	passTextures.clear();
	passMatrices.clear();
	passConstants.clear();
	xr_strcpy(pass_ps, "null");
	xr_strcpy(pass_vs, "null");
	dwStage = 0;
}

void CBlender_Compile::PassEnd()
{
	// Last Stage - disable
	RS.SetTSS(Stage(), D3DTSS_COLOROP, D3DTOP_DISABLE);
	RS.SetTSS(Stage(), D3DTSS_ALPHAOP, D3DTOP_DISABLE);

	SPass proto;
	// Create pass
	proto.state = RImplementation.Resources->_CreateState(RS.GetContainer());
	proto.ps = RImplementation.Resources->_CreatePS(pass_ps);
	proto.vs = RImplementation.Resources->_CreateVS(pass_vs);
	ctable.merge(&proto.ps->constants);
	ctable.merge(&proto.vs->constants);

	proto.gs = RImplementation.Resources->_CreateGS(pass_gs);
	ctable.merge(&proto.gs->constants);
	proto.hs = RImplementation.Resources->_CreateHS(pass_hs);
	ctable.merge(&proto.hs->constants);
	proto.ds = RImplementation.Resources->_CreateDS(pass_ds);
	ctable.merge(&proto.ds->constants);
	proto.cs = RImplementation.Resources->_CreateCS(pass_cs);
	ctable.merge(&proto.cs->constants);

	SetMapping();
	proto.constants = RImplementation.Resources->_CreateConstantTable(ctable);
	proto.T = RImplementation.Resources->_CreateTextureList(passTextures);
	proto.C = RImplementation.Resources->_CreateConstantList(passConstants);

	ref_pass _pass_ = RImplementation.Resources->_CreatePass(proto);
	SH->passes.push_back(_pass_);
}

void CBlender_Compile::PassSET_PS(LPCSTR name)
{
	xr_strcpy(pass_ps, name);
	xr_strlwr(pass_ps);
}

void CBlender_Compile::PassSET_VS(LPCSTR name)
{
	xr_strcpy(pass_vs, name);
	xr_strlwr(pass_vs);
}

void CBlender_Compile::PassSET_ZB(BOOL bZTest, BOOL bZWrite, BOOL bInvertZTest)
{
	if (Pass())
		bZWrite = FALSE;
	RS.SetRS(D3DRS_ZFUNC, bZTest ? (bInvertZTest ? D3DCMP_GREATER : D3DCMP_LESSEQUAL) : D3DCMP_ALWAYS);
	RS.SetRS(D3DRS_ZWRITEENABLE, BC(bZWrite));
	/*
	if (bZWrite || bZTest)				RS.SetRS	(D3DRS_ZENABLE,	D3DZB_TRUE);
	else								RS.SetRS	(D3DRS_ZENABLE,	D3DZB_FALSE);
	*/
}

void CBlender_Compile::PassSET_ablend_mode(BOOL bABlend, u32 abSRC, u32 abDST)
{
	if (bABlend && D3DBLEND_ONE == abSRC && D3DBLEND_ZERO == abDST)
		bABlend = FALSE;
	RS.SetRS(D3DRS_ALPHABLENDENABLE, BC(bABlend));
	RS.SetRS(D3DRS_SRCBLEND, bABlend ? abSRC : D3DBLEND_ONE);
	RS.SetRS(D3DRS_DESTBLEND, bABlend ? abDST : D3DBLEND_ZERO);

	//	Since in our engine D3DRS_SEPARATEALPHABLENDENABLE state is
	//	always set to false and in DirectX 10 blend functions for
	//	color and alpha are always independent, assign blend options for
	//	alpha in DX10 identical to color.
	RS.SetRS(D3DRS_SRCBLENDALPHA, bABlend ? abSRC : D3DBLEND_ONE);
	RS.SetRS(D3DRS_DESTBLENDALPHA, bABlend ? abDST : D3DBLEND_ZERO);
}
void CBlender_Compile::PassSET_ablend_aref(BOOL bATest, u32 aRef)
{
	clamp(aRef, 0u, 255u);
	RS.SetRS(D3DRS_ALPHATESTENABLE, BC(bATest));
	if (bATest)
		RS.SetRS(D3DRS_ALPHAREF, u32(aRef));
}

void CBlender_Compile::PassSET_Blend(BOOL bABlend, u32 abSRC, u32 abDST, BOOL bATest, u32 aRef)
{
	PassSET_ablend_mode(bABlend, abSRC, abDST);
#ifdef DEBUG
	if (strstr(Core.Params, "-noaref"))
	{
		bATest = FALSE;
		aRef = 0;
	}
#endif
	PassSET_ablend_aref(bATest, aRef);
}

void CBlender_Compile::PassSET_LightFog(BOOL bLight, BOOL bFog)
{
	RS.SetRS(D3DRS_LIGHTING, BC(bLight));
	RS.SetRS(D3DRS_FOGENABLE, BC(bFog));
	// SH->Flags.bLighting				|= !!bLight;
}

//
void CBlender_Compile::StageBegin()
{
	StageSET_Address(D3DTADDRESS_WRAP); // Wrapping enabled by default
}
void CBlender_Compile::StageEnd() { dwStage++; }
void CBlender_Compile::StageSET_Address(u32 adr)
{
	RS.SetSAMP(Stage(), D3DSAMP_ADDRESSU, adr);
	RS.SetSAMP(Stage(), D3DSAMP_ADDRESSV, adr);
}
void CBlender_Compile::StageSET_XForm(u32 tf, u32 tc)
{
#ifdef _EDITOR
	RS.SetTSS(Stage(), D3DTSS_TEXTURETRANSFORMFLAGS, tf);
	RS.SetTSS(Stage(), D3DTSS_TEXCOORDINDEX, tc);
#else
	UNUSED(tf);
	UNUSED(tc);
#endif
}
void CBlender_Compile::StageSET_Color(u32 a1, u32 op, u32 a2) { RS.SetColor(Stage(), a1, op, a2); }
void CBlender_Compile::StageSET_Color3(u32 a1, u32 op, u32 a2, u32 a3) { RS.SetColor3(Stage(), a1, op, a2, a3); }
void CBlender_Compile::StageSET_Alpha(u32 a1, u32 op, u32 a2) { RS.SetAlpha(Stage(), a1, op, a2); }

void CBlender_Compile::Stage_Matrix(LPCSTR name, int iChannel)
{
	sh_list& lst = L_matrices;
	int id = ParseName(name);
	CMatrix* M = RImplementation.Resources->_CreateMatrix((id >= 0) ? *lst[id] : name);
	passMatrices.push_back(M);

	// Setup transform pipeline
	u32 ID = Stage();
	if (M)
	{
		switch (M->dwMode)
		{
		case CMatrix::modeProgrammable: StageSET_XForm(D3DTTFF_COUNT3, D3DTSS_TCI_CAMERASPACEPOSITION | ID); break;
		case CMatrix::modeTCM: StageSET_XForm(D3DTTFF_COUNT2, D3DTSS_TCI_PASSTHRU | iChannel); break;
		case CMatrix::modeC_refl: StageSET_XForm(D3DTTFF_COUNT3, D3DTSS_TCI_CAMERASPACEREFLECTIONVECTOR | ID); break;
		case CMatrix::modeS_refl: StageSET_XForm(D3DTTFF_COUNT2, D3DTSS_TCI_CAMERASPACENORMAL | ID); break;
		default: StageSET_XForm(D3DTTFF_DISABLE, D3DTSS_TCI_PASSTHRU | iChannel); break;
		}
	}
	else
	{
		// No XForm at all
		StageSET_XForm(D3DTTFF_DISABLE, D3DTSS_TCI_PASSTHRU | iChannel);
	}
}

void CBlender_Compile::Stage_Constant(LPCSTR name)
{
	sh_list& lst = L_constants;
	int id = ParseName(name);
	passConstants.push_back(RImplementation.Resources->_CreateConstant((id >= 0) ? *lst[id] : name));
}

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

void CBlender_Compile::r_Stencil(BOOL Enable, u32 Func, u32 Mask, u32 WriteMask, u32 Fail, u32 Pass, u32 ZFail)
{
	RS.SetRS(D3DRS_STENCILENABLE, BC(Enable));
	if (!Enable)
		return;
	RS.SetRS(D3DRS_STENCILFUNC, Func);
	RS.SetRS(D3DRS_STENCILMASK, Mask);
	RS.SetRS(D3DRS_STENCILWRITEMASK, WriteMask);
	RS.SetRS(D3DRS_STENCILFAIL, Fail);
	RS.SetRS(D3DRS_STENCILPASS, Pass);
	RS.SetRS(D3DRS_STENCILZFAIL, ZFail);
	//	Since we never really support different options for
	//	CW/CCW stencil use it to mimic DX9 behaviour for
	//	single-sided stencil
	RS.SetRS(D3DRS_CCW_STENCILFUNC, Func);
	RS.SetRS(D3DRS_CCW_STENCILFAIL, Fail);
	RS.SetRS(D3DRS_CCW_STENCILPASS, Pass);
	RS.SetRS(D3DRS_CCW_STENCILZFAIL, ZFail);
}

void CBlender_Compile::r_StencilRef(u32 Ref) { RS.SetRS(D3DRS_STENCILREF, Ref); }
void CBlender_Compile::r_CullMode(D3DCULL Mode) { RS.SetRS(D3DRS_CULLMODE, (u32)Mode); }
void CBlender_Compile::r_dx10Texture(LPCSTR ResourceName, LPCSTR texture)
{
	VERIFY(ResourceName);
	if (!texture)
		return;
	//
	string256 TexName;
	xr_strcpy(TexName, texture);
	fix_texture_name(TexName);

	// Find index
	ref_constant C = ctable.get(ResourceName);
	// VERIFY(C);
	if (!C)
		return;

	R_ASSERT(C->type == RC_dx10texture);
	u32 stage = C->samp.index;

	passTextures.emplace_back(stage, ref_texture(RImplementation.Resources->_CreateTexture(TexName)));
}

void CBlender_Compile::i_dx10Address(u32 s, u32 address)
{
	// VERIFY(s!=u32(-1));
	if (s == u32(-1))
	{
		Msg("s != u32(-1)");
	}
	RS.SetSAMP(s, D3DSAMP_ADDRESSU, address);
	RS.SetSAMP(s, D3DSAMP_ADDRESSV, address);
	RS.SetSAMP(s, D3DSAMP_ADDRESSW, address);
}

void CBlender_Compile::i_dx10BorderColor(u32 s, u32 color) { RS.SetSAMP(s, D3DSAMP_BORDERCOLOR, color); }
void CBlender_Compile::i_dx10Filter_Min(u32 s, u32 f)
{
	VERIFY(s != u32(-1));
	RS.SetSAMP(s, D3DSAMP_MINFILTER, f);
}

void CBlender_Compile::i_dx10Filter_Mip(u32 s, u32 f)
{
	VERIFY(s != u32(-1));
	RS.SetSAMP(s, D3DSAMP_MIPFILTER, f);
}

void CBlender_Compile::i_dx10Filter_Mag(u32 s, u32 f)
{
	VERIFY(s != u32(-1));
	RS.SetSAMP(s, D3DSAMP_MAGFILTER, f);
}

void CBlender_Compile::i_dx10FilterAnizo(u32 s, BOOL value)
{
	VERIFY(s != u32(-1));
	RS.SetSAMP(s, XRDX10SAMP_ANISOTROPICFILTER, value);
}

void CBlender_Compile::i_dx10Filter(u32 s, u32 _min, u32 _mip, u32 _mag)
{
	VERIFY(s != u32(-1));
	i_dx10Filter_Min(s, _min);
	i_dx10Filter_Mip(s, _mip);
	i_dx10Filter_Mag(s, _mag);
}

u32 CBlender_Compile::r_dx10Sampler(LPCSTR ResourceName)
{
	//	TEST
	// return ((u32)-1);
	VERIFY(ResourceName);
	string256 name;
	xr_strcpy(name, ResourceName);
	fix_texture_name(name);

	// Find index
	// ref_constant C			= ctable.get(ResourceName);
	ref_constant C = ctable.get(name);
	// VERIFY(C);
	if (!C)
		return u32(-1);

	R_ASSERT(C->type == RC_sampler);
	u32 stage = C->samp.index;

	//	init defaults here

	//	Use D3DTADDRESS_CLAMP,	D3DTEXF_POINT,			D3DTEXF_NONE,	D3DTEXF_POINT
	if (0 == xr_strcmp(ResourceName, "smp_nofilter"))
	{
		i_dx10Address(stage, D3DTADDRESS_CLAMP);
		i_dx10Filter(stage, D3DTEXF_POINT, D3DTEXF_NONE, D3DTEXF_POINT);
	}

	//	Use D3DTADDRESS_CLAMP,	D3DTEXF_LINEAR,			D3DTEXF_NONE,	D3DTEXF_LINEAR
	if (0 == xr_strcmp(ResourceName, "smp_rtlinear"))
	{
		i_dx10Address(stage, D3DTADDRESS_CLAMP);
		i_dx10Filter(stage, D3DTEXF_LINEAR, D3DTEXF_NONE, D3DTEXF_LINEAR);
	}

	//	Use	D3DTADDRESS_WRAP,	D3DTEXF_LINEAR,			D3DTEXF_LINEAR,	D3DTEXF_LINEAR
	if (0 == xr_strcmp(ResourceName, "smp_linear"))
	{
		i_dx10Address(stage, D3DTADDRESS_WRAP);
		i_dx10Filter(stage, D3DTEXF_LINEAR, D3DTEXF_LINEAR, D3DTEXF_LINEAR);
	}

	//	Use D3DTADDRESS_WRAP,	D3DTEXF_ANISOTROPIC, 	D3DTEXF_LINEAR,	D3DTEXF_ANISOTROPIC
	if (0 == xr_strcmp(ResourceName, "smp_base"))
	{
		i_dx10Address(stage, D3DTADDRESS_WRAP);
		i_dx10FilterAnizo(stage, TRUE);
		// i_dx10Filter(stage, D3DTEXF_LINEAR, D3DTEXF_LINEAR, D3DTEXF_LINEAR);
	}

	//	Use D3DTADDRESS_CLAMP,	D3DTEXF_LINEAR,			D3DTEXF_NONE,	D3DTEXF_LINEAR
	if (0 == xr_strcmp(ResourceName, "smp_material"))
	{
		i_dx10Address(stage, D3DTADDRESS_CLAMP);
		i_dx10Filter(stage, D3DTEXF_LINEAR, D3DTEXF_NONE, D3DTEXF_LINEAR);
		RS.SetSAMP(stage, D3DSAMP_ADDRESSW, D3DTADDRESS_WRAP);
	}

	if (0 == xr_strcmp(ResourceName, "smp_smap"))
	{
		i_dx10Address(stage, D3DTADDRESS_CLAMP);
		i_dx10Filter(stage, D3DTEXF_LINEAR, D3DTEXF_NONE, D3DTEXF_LINEAR);
		RS.SetSAMP(stage, XRDX10SAMP_COMPARISONFILTER, TRUE);
		RS.SetSAMP(stage, XRDX10SAMP_COMPARISONFUNC, (u32)D3D_COMPARISON_LESS_EQUAL);
	}

	if (0 == xr_strcmp(ResourceName, "smp_jitter"))
	{
		i_dx10Address(stage, D3DTADDRESS_WRAP);
		i_dx10Filter(stage, D3DTEXF_POINT, D3DTEXF_NONE, D3DTEXF_POINT);
	}

	return stage;
}

void CBlender_Compile::r_Pass(LPCSTR _vs, LPCSTR _gs, LPCSTR _ps, bool bFog, BOOL bZtest, BOOL bZwrite, BOOL bABlend,
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
	SGS* gs = RImplementation.Resources->_CreateGS(_gs);
	dest.ps = ps;
	dest.vs = vs;
	dest.gs = gs;
	dest.hs = RImplementation.Resources->_CreateHS("null");
	dest.ds = RImplementation.Resources->_CreateDS("null");
	dest.cs = RImplementation.Resources->_CreateCS("null");
	ctable.merge(&ps->constants);
	ctable.merge(&vs->constants);
	ctable.merge(&gs->constants);

	// Last Stage - disable
	if (0 == xr_stricmp(_ps, "null"))
	{
		RS.SetTSS(0, D3DTSS_COLOROP, D3DTOP_DISABLE);
		RS.SetTSS(0, D3DTSS_ALPHAOP, D3DTOP_DISABLE);
	}
}

void CBlender_Compile::r_TessPass(LPCSTR vs, LPCSTR hs, LPCSTR ds, LPCSTR gs, LPCSTR ps, bool bFog, BOOL bZtest,
	BOOL bZwrite, BOOL bABlend, D3DBLEND abSRC, D3DBLEND abDST, BOOL aTest, u32 aRef)
{
	r_Pass(vs, gs, ps, bFog, bZtest, bZwrite, bABlend, abSRC, abDST, aTest, aRef);

	dest.hs = RImplementation.Resources->_CreateHS(hs);
	dest.ds = RImplementation.Resources->_CreateDS(ds);

	ctable.merge(&dest.hs->constants);
	ctable.merge(&dest.ds->constants);
}

void CBlender_Compile::r_ComputePass(LPCSTR cs)
{
	dest.cs = RImplementation.Resources->_CreateCS(cs);

	ctable.merge(&dest.cs->constants);
}

void CBlender_Compile::r_End()
{
	SetMapping();
	dest.constants = RImplementation.Resources->_CreateConstantTable(ctable);
	dest.state = RImplementation.Resources->_CreateState(RS.GetContainer());
	dest.T = RImplementation.Resources->_CreateTextureList(passTextures);
	dest.C = 0;
	ref_matrix_list temp(0);
	SH->passes.push_back(RImplementation.Resources->_CreatePass(dest));
}