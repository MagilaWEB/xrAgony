// TextureManager.cpp: implementation of the CResourceManager class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#pragma hdrstop
#include <tbb.h>

#include "ResourceManager.h"
#include "tss.h"
#include "blenders/blender.h"
#include "blenders/blender_recorder.h"
#include "Layers/xrRenderDX10/dx10BufferUtils.h"
#include "Layers/xrRenderDX10/dx10ConstantBuffer.h"
#include "Layers/xrRender/ShaderResourceTraits.h"

#include "xrEngine/x_ray.h"

#include <FlexibleVertexFormat.h>

SGS* CResourceManager::_CreateGS(LPCSTR Name) { return CreateShader<SGS>(Name); }
void CResourceManager::_DeleteGS(const SGS* GS) { DestroyShader(GS); }

SHS* CResourceManager::_CreateHS(LPCSTR Name) { return CreateShader<SHS>(Name); }
void CResourceManager::_DeleteHS(const SHS* HS) { DestroyShader(HS); }
SDS* CResourceManager::_CreateDS(LPCSTR Name) { return CreateShader<SDS>(Name); }
void CResourceManager::_DeleteDS(const SDS* DS) { DestroyShader(DS); }
SCS* CResourceManager::_CreateCS(LPCSTR Name) { return CreateShader<SCS>(Name); }
void CResourceManager::_DeleteCS(const SCS* CS) { DestroyShader(CS); }

void fix_texture_name(LPSTR fn);

template <class T>
bool reclaim(xr_vector<T*>& vec, const T* ptr)
{
	if (vec.empty())
		return false;

	for (size_t i = 0; i < vec.size(); i++)
	{
		if (vec[i] == ptr)
		{
			vec.erase(vec.begin() + i);
			return true;
		}
	}

	return false;
}

//--------------------------------------------------------------------------------------------------------------
SState* CResourceManager::_CreateState(SimulatorStates& state_code)
{
	// Search equal state-code
	for (u32 it = 0; it < v_states.size(); it++)
	{
		SState* C = v_states[it];
		;
		SimulatorStates& base = C->state_code;
		if (base.equal(state_code))
			return C;
	}

	// Create New
	v_states.push_back(new SState());
	v_states.back()->dwFlags |= xr_resource_flagged::RF_REGISTERED;
	v_states.back()->state = ID3DState::Create(state_code);
	v_states.back()->state_code = state_code;
	return v_states.back();
}

void CResourceManager::_DeleteState(const SState* state)
{
	if (0 == (state->dwFlags & xr_resource_flagged::RF_REGISTERED))
		return;
	if (reclaim(v_states, state))
		return;
	Msg("! ERROR: Failed to find compiled stateblock");
}

//--------------------------------------------------------------------------------------------------------------
SPass* CResourceManager::_CreatePass(const SPass& proto)
{
	for (u32 it = 0; it < v_passes.size(); it++)
		if (v_passes[it]->equal(proto))
			return v_passes[it];

	SPass* P = new SPass();
	P->dwFlags |= xr_resource_flagged::RF_REGISTERED;
	P->state = proto.state;
	P->ps = proto.ps;
	P->vs = proto.vs;
	P->gs = proto.gs;
	P->hs = proto.hs;
	P->ds = proto.ds;
	P->cs = proto.cs;
	P->constants = proto.constants;
	P->T = proto.T;
#ifdef _EDITOR
	P->M = proto.M;
#endif
	P->C = proto.C;

	v_passes.push_back(P);
	return v_passes.back();
}

void CResourceManager::_DeletePass(const SPass* P)
{
	if (0 == (P->dwFlags & xr_resource_flagged::RF_REGISTERED))
		return;
	if (reclaim(v_passes, P))
		return;
	Msg("! ERROR: Failed to find compiled pass");
}

//--------------------------------------------------------------------------------------------------------------
SVS* CResourceManager::_CreateVS(LPCSTR _name)
{
	string_path name;
	xr_strcpy(name, _name);
	if (0 == ::Render->m_skinning)
		xr_strcat(name, "_0");
	if (1 == ::Render->m_skinning)
		xr_strcat(name, "_1");
	if (2 == ::Render->m_skinning)
		xr_strcat(name, "_2");
	if (3 == ::Render->m_skinning)
		xr_strcat(name, "_3");
	if (4 == ::Render->m_skinning)
		xr_strcat(name, "_4");

	return CreateShader<SVS>(name, _name, false);
}

void CResourceManager::_DeleteVS(const SVS* vs)
{
	// XXX: try to use code below
	// DestroyShader(vs);

	if (0 == (vs->dwFlags & xr_resource_flagged::RF_REGISTERED))
		return;
	LPSTR N = LPSTR(*vs->cName);
	map_VS::iterator I = m_vs.find(N);
	if (I != m_vs.end())
	{
		m_vs.erase(I);
		xr_vector<SDeclaration*>::iterator iDecl;
		for (iDecl = v_declarations.begin(); iDecl != v_declarations.end(); ++iDecl)
		{
			xr_map<ID3DBlob*, ID3DInputLayout*>::iterator iLayout;
			iLayout = (*iDecl)->vs_to_layout.find(vs->signature->signature);
			if (iLayout != (*iDecl)->vs_to_layout.end())
			{
				//	Release vertex layout
				_RELEASE(iLayout->second);
				(*iDecl)->vs_to_layout.erase(iLayout);
			}
		}
		return;
	}
	Msg("! ERROR: Failed to find compiled vertex-shader '%s'", *vs->cName);
}

//--------------------------------------------------------------------------------------------------------------
SPS* CResourceManager::_CreatePS(LPCSTR _name)
{
	string_path name;
	xr_strcpy(name, _name);
	if (0 == ::Render->m_MSAASample)
		xr_strcat(name, "_0");
	if (1 == ::Render->m_MSAASample)
		xr_strcat(name, "_1");
	if (2 == ::Render->m_MSAASample)
		xr_strcat(name, "_2");
	if (3 == ::Render->m_MSAASample)
		xr_strcat(name, "_3");
	if (4 == ::Render->m_MSAASample)
		xr_strcat(name, "_4");
	if (5 == ::Render->m_MSAASample)
		xr_strcat(name, "_5");
	if (6 == ::Render->m_MSAASample)
		xr_strcat(name, "_6");
	if (7 == ::Render->m_MSAASample)
		xr_strcat(name, "_7");

	return CreateShader<SPS>(name, _name, false);
}
void CResourceManager::_DeletePS(const SPS* ps) { DestroyShader(ps); }
//--------------------------------------------------------------------------------------------------------------
static BOOL dcl_equal(D3DVERTEXELEMENT9* a, D3DVERTEXELEMENT9* b)
{
	// check sizes
	u32 a_size = dx10BufferUtils::GetDeclLength(a);
	u32 b_size = dx10BufferUtils::GetDeclLength(b);
	if (a_size != b_size)
		return FALSE;
	return 0 == memcmp(a, b, a_size * sizeof(D3DVERTEXELEMENT9));
}

SDeclaration* CResourceManager::_CreateDecl(D3DVERTEXELEMENT9* dcl)
{
	// Search equal code
	for (u32 it = 0; it < v_declarations.size(); it++)
	{
		SDeclaration* D = v_declarations[it];
		;
		if (dcl_equal(dcl, &*D->dcl_code.begin()))
			return D;
	}

	// Create _new
	SDeclaration* D = new SDeclaration();
	u32 dcl_size = dx10BufferUtils::GetDeclLength(dcl) + 1;
	//	Don't need it for DirectX 10 here
	// CHK_DX					(HW.pDevice->CreateVertexDeclaration(dcl,&D->dcl));
	D->dcl_code.assign(dcl, dcl + dcl_size);
	dx10BufferUtils::ConvertVertexDeclaration(D->dcl_code, D->dx10_dcl_code);
	D->dwFlags |= xr_resource_flagged::RF_REGISTERED;
	v_declarations.push_back(D);
	return D;
}

void CResourceManager::_DeleteDecl(const SDeclaration* dcl)
{
	if (0 == (dcl->dwFlags & xr_resource_flagged::RF_REGISTERED))
		return;
	if (reclaim(v_declarations, dcl))
		return;
	Msg("! ERROR: Failed to find compiled vertex-declarator");
}

//--------------------------------------------------------------------------------------------------------------
R_constant_table* CResourceManager::_CreateConstantTable(R_constant_table& C)
{
	if (C.empty())
		return nullptr;

	for (auto& constant : v_constant_tables)
		if (constant->equal(C))
			return constant;

	R_constant_table*& newElem = v_constant_tables.emplace_back(new R_constant_table());
	newElem->_copy(C);
	newElem->dwFlags |= xr_resource_flagged::RF_REGISTERED;
	return newElem;
}
void CResourceManager::_DeleteConstantTable(const R_constant_table* C)
{
	if (0 == (C->dwFlags & xr_resource_flagged::RF_REGISTERED))
		return;
	if (reclaim(v_constant_tables, C))
		return;
	Msg("! ERROR: Failed to find compiled constant-table");
}

//--------------------------------------------------------------------------------------------------------------
CRT* CResourceManager::_CreateRT(LPCSTR Name, u32 w, u32 h, D3DFORMAT f, u32 SampleCount, bool useUAV)
{
	R_ASSERT(Name && Name[0] && w && h);

	// ***** first pass - search already created RT
	LPSTR N = LPSTR(Name);
	map_RT::iterator I = m_rtargets.find(N);
	if (I != m_rtargets.end())
		return I->second;
	else
	{
		CRT* RT = new CRT();
		RT->dwFlags |= xr_resource_flagged::RF_REGISTERED;
		m_rtargets.emplace(RT->set_name(Name), RT);
		if (Device.b_is_Ready.load())
			RT->create(Name, w, h, f, SampleCount, useUAV);
		return RT;
	}
}
void CResourceManager::_DeleteRT(const CRT* RT)
{
	if (0 == (RT->dwFlags & xr_resource_flagged::RF_REGISTERED))
		return;
	LPSTR N = LPSTR(*RT->cName);
	map_RT::iterator I = m_rtargets.find(N);
	if (I != m_rtargets.end())
	{
		m_rtargets.erase(I);
		return;
	}
	Msg("! ERROR: Failed to find render-target '%s'", *RT->cName);
}
/*	//	DX10 cut
//--------------------------------------------------------------------------------------------------------------
CRTC*	CResourceManager::_CreateRTC		(LPCSTR Name, u32 size,	D3DFORMAT f)
{
	R_ASSERT(Name && Name[0] && size);

	// ***** first pass - search already created RTC
	LPSTR N = LPSTR(Name);
	map_RTC::iterator I = m_rtargets_c.find	(N);
	if (I!=m_rtargets_c.end())	return I->second;
	else
	{
		CRTC *RT				=	new CRTC();
		RT->dwFlags				|=	xr_resource_flagged::RF_REGISTERED;
		m_rtargets_c.insert		(std::make_pair(RT->set_name(Name),RT));
		if (Device.b_is_Ready)	RT->create	(Name,size,f);
		return					RT;
	}
}
void	CResourceManager::_DeleteRTC		(const CRTC* RT)
{
	if (0==(RT->dwFlags&xr_resource_flagged::RF_REGISTERED))	return;
	LPSTR N				= LPSTR		(*RT->cName);
	map_RTC::iterator I	= m_rtargets_c.find	(N);
	if (I!=m_rtargets_c.end())	{
		m_rtargets_c.erase(I);
		return;
	}
	Msg	("! ERROR: Failed to find render-target '%s'",*RT->cName);
}
*/
//--------------------------------------------------------------------------------------------------------------
//void CResourceManager::DBG_VerifyGeoms()
//{
//	for (u32 it = 0; it < v_geoms.size(); it++)
//	{
//		SGeometry* G = v_geoms[it];
//
//		D3DVERTEXELEMENT9		test[MAX_FVF_DECL_SIZE];
//		u32						size = 0;
//		G->dcl->GetDeclaration(test, (unsigned int*)&size);
//		u32 vb_stride = D3DXGetDeclVertexSize(test, 0);
//		u32 vb_stride_cached = G->vb_stride;
//		R_ASSERT(vb_stride == vb_stride_cached);
//	}
//}

SGeometry* CResourceManager::CreateGeom(D3DVERTEXELEMENT9* decl, ID3DVertexBuffer* vb, ID3DIndexBuffer* ib)
{
	R_ASSERT(decl && vb);

	SDeclaration* dcl = _CreateDecl(decl);
	u32 vb_stride = dx10BufferUtils::GetDeclVertexSize(decl, 0);

	// ***** first pass - search already loaded shader
	for (u32 it = 0; it < v_geoms.size(); it++)
	{
		SGeometry& G = *(v_geoms[it]);
		if ((G.dcl == dcl) && (G.vb == vb) && (G.ib == ib) && (G.vb_stride == vb_stride))
			return v_geoms[it];
	}

	SGeometry* Geom = new SGeometry();
	Geom->dwFlags |= xr_resource_flagged::RF_REGISTERED;
	Geom->dcl = dcl;
	Geom->vb = vb;
	Geom->vb_stride = vb_stride;
	Geom->ib = ib;
	v_geoms.push_back(Geom);
	return Geom;
}
SGeometry* CResourceManager::CreateGeom(u32 FVF, ID3DVertexBuffer* vb, ID3DIndexBuffer* ib)
{
	thread_local xr_vector<D3DVERTEXELEMENT9> decl;
	[[maybe_unused]] const bool result = FVF::CreateDeclFromFVF(FVF, decl);
	VERIFY(result);
	SGeometry* g = CreateGeom(decl.data(), vb, ib);
	return g;
}

void CResourceManager::DeleteGeom(const SGeometry* Geom)
{
	if (0 == (Geom->dwFlags & xr_resource_flagged::RF_REGISTERED))
		return;
	if (reclaim(v_geoms, Geom))
		return;
	Msg("! ERROR: Failed to find compiled geometry-declaration");
}

//--------------------------------------------------------------------------------------------------------------
CTexture* CResourceManager::_CreateTexture(LPCSTR _Name)
{
#ifdef DEBUG
	DBG_VerifyTextures();
#endif

	if (0 == xr_strcmp(_Name, "null"))
		return 0;
	R_ASSERT(_Name && _Name[0]);
	string_path Name;
	xr_strcpy(Name, _Name); //. andy if (strext(Name)) *strext(Name)=0;
	fix_texture_name(Name);
	// ***** first pass - search already loaded texture
	auto I = m_textures.find(Name);
	if (I != m_textures.end())
		return I->second;

	CTexture* T = new CTexture();
	T->dwFlags |= xr_resource_flagged::RF_REGISTERED;
	m_textures.emplace(T->set_name(Name), T);

	T->Preload();

	task_louding_textures.run([this, Name]
	{
		while (!Device.b_is_Ready.load())
			std::this_thread::yield();

		size_t it{ 0 };
		do
		{
			auto texture_it = m_textures.find(Name);
			if (texture_it != m_textures.end())
			{
				(*texture_it).second->Load();
				break;
			}
			else
				std::this_thread::yield();
		} while (++it < 10);

		VERIFY3(it <= 10, "filed texture loading ", Name);
	});

	return T;
}
void CResourceManager::_DeleteTexture(const CTexture* T)
{
#ifdef DEBUG
	DBG_VerifyTextures();
#endif

	if (0 == (T->dwFlags & xr_resource_flagged::RF_REGISTERED))
		return;
	LPSTR N = LPSTR(*T->cName);
	map_Texture::iterator I = m_textures.find(N);
	if (I != m_textures.end())
	{
		m_textures.erase(I);
		return;
	}
	Msg("! ERROR: Failed to find texture surface '%s'", *T->cName);
}

#ifdef DEBUG
void CResourceManager::DBG_VerifyTextures()
{
	map_Texture::iterator I = m_textures.begin();
	map_Texture::iterator E = m_textures.end();
	for (; I != E; I++)
	{
		R_ASSERT(I->first);
		R_ASSERT(I->second);
		R_ASSERT(I->second->cName);
		R_ASSERT(0 == xr_strcmp(I->first, *I->second->cName));
	}
}
#endif

//--------------------------------------------------------------------------------------------------------------
CMatrix* CResourceManager::_CreateMatrix(LPCSTR Name)
{
	R_ASSERT(Name && Name[0]);
	if (0 == xr_stricmp(Name, "$null"))
		return nullptr;

	LPSTR N = LPSTR(Name);
	map_Matrix::iterator I = m_matrices.find(N);
	if (I != m_matrices.end())
		return I->second;
	else
	{
		CMatrix* M = new CMatrix();
		M->dwFlags |= xr_resource_flagged::RF_REGISTERED;
		M->dwReference.store(1);
		m_matrices.emplace(M->set_name(Name), M);
		return M;
	}
}
void CResourceManager::_DeleteMatrix(const CMatrix* M)
{
	if (0 == (M->dwFlags & xr_resource_flagged::RF_REGISTERED))
		return;
	LPSTR N = LPSTR(*M->cName);
	map_Matrix::iterator I = m_matrices.find(N);
	if (I != m_matrices.end())
	{
		m_matrices.erase(I);
		return;
	}
	Msg("! ERROR: Failed to find xform-def '%s'", *M->cName);
}

//--------------------------------------------------------------------------------------------------------------
CConstant* CResourceManager::_CreateConstant(LPCSTR Name)
{
	R_ASSERT(Name && Name[0]);
	if (0 == xr_stricmp(Name, "$null"))
		return nullptr;

	LPSTR N = LPSTR(Name);
	map_Constant::iterator I = m_constants.find(N);
	if (I != m_constants.end())
		return I->second;
	else
	{
		CConstant* C = new CConstant();
		C->dwFlags |= xr_resource_flagged::RF_REGISTERED;
		C->dwReference.store(1);
		m_constants.emplace(C->set_name(Name), C);
		return C;
	}
}
void CResourceManager::_DeleteConstant(const CConstant* C)
{
	if (0 == (C->dwFlags & xr_resource_flagged::RF_REGISTERED))
		return;
	LPSTR N = LPSTR(*C->cName);
	map_Constant::iterator I = m_constants.find(N);
	if (I != m_constants.end())
	{
		m_constants.erase(I);
		return;
	}
	Msg("! ERROR: Failed to find R1-constant-def '%s'", *C->cName);
}

//--------------------------------------------------------------------------------------------------------------
bool cmp_tl(const std::pair<u32, ref_texture>& _1, const std::pair<u32, ref_texture>& _2)
{
	return _1.first < _2.first;
}
STextureList* CResourceManager::_CreateTextureList(STextureList& L)
{
	L.sort(cmp_tl);
	for (auto& texture_list : lst_textures)
		if (L.equal(*texture_list))
			return texture_list;

	STextureList*& newTextureL = lst_textures.emplace_back(new STextureList());
	newTextureL->_copy(L);
	newTextureL->dwFlags |= xr_resource_flagged::RF_REGISTERED;
	return newTextureL;
}
void CResourceManager::_DeleteTextureList(const STextureList* L)
{
	if (0 == (L->dwFlags & xr_resource_flagged::RF_REGISTERED))
		return;
	if (reclaim(lst_textures, L))
		return;
	Msg("! ERROR: Failed to find compiled list of textures");
}
//--------------------------------------------------------------------------------------------------------------
SMatrixList* CResourceManager::_CreateMatrixList(SMatrixList& L)
{
	BOOL bEmpty = TRUE;
	for (auto& matrix : L)
		if (matrix)
		{
			bEmpty = FALSE;
			break;
		}

	if (bEmpty)
		return nullptr;

	for (auto& matrix : lst_matrices)
		if (L.equal(*matrix))
			return matrix;

	SMatrixList*& newMatrixL = lst_matrices.emplace_back(new SMatrixList());
	newMatrixL->_copy(L);
	newMatrixL->dwFlags |= xr_resource_flagged::RF_REGISTERED;
	return newMatrixL;
}
void CResourceManager::_DeleteMatrixList(const SMatrixList* L)
{
	if (0 == (L->dwFlags & xr_resource_flagged::RF_REGISTERED))
		return;
	if (reclaim(lst_matrices, L))
		return;
	Msg("! ERROR: Failed to find compiled list of xform-defs");
}
//--------------------------------------------------------------------------------------------------------------
SConstantList* CResourceManager::_CreateConstantList(SConstantList& L)
{
	BOOL bEmpty = TRUE;
	for (auto& lst_constant : L)
		if (lst_constant)
		{
			bEmpty = FALSE;
			break;
		}

	if (bEmpty)
		return nullptr;

	for (auto& lst_constant : lst_constants)
		if (L.equal(*lst_constant))
			return lst_constant;

	SConstantList*& newConstantL = lst_constants.emplace_back(new SConstantList());
	newConstantL->_copy(L);
	newConstantL->dwFlags |= xr_resource_flagged::RF_REGISTERED;
	return newConstantL;
}

void CResourceManager::_DeleteConstantList(const SConstantList* L)
{
	if (0 == (L->dwFlags & xr_resource_flagged::RF_REGISTERED))
		return;
	if (reclaim(lst_constants, L))
		return;
	Msg("! ERROR: Failed to find compiled list of r1-constant-defs");
}
//--------------------------------------------------------------------------------------------------------------
dx10ConstantBuffer* CResourceManager::_CreateConstantBuffer(ID3DShaderReflectionConstantBuffer* pTable)
{
	VERIFY(pTable);
	dx10ConstantBuffer* pTempBuffer = new dx10ConstantBuffer(pTable);

	for (u32 it = 0; it < v_constant_buffer.size(); it++)
	{
		dx10ConstantBuffer* buf = v_constant_buffer[it];
		if (pTempBuffer->Similar(*buf))
		{
			xr_delete(pTempBuffer);
			return buf;
		}
	}

	pTempBuffer->dwFlags |= xr_resource_flagged::RF_REGISTERED;
	v_constant_buffer.push_back(pTempBuffer);
	return pTempBuffer;
}
//--------------------------------------------------------------------------------------------------------------
void CResourceManager::_DeleteConstantBuffer(const dx10ConstantBuffer* pBuffer)
{
	if (0 == (pBuffer->dwFlags & xr_resource_flagged::RF_REGISTERED))
		return;
	if (reclaim(v_constant_buffer, pBuffer))
		return;
	Msg("! ERROR: Failed to find compiled constant buffer");
}

//--------------------------------------------------------------------------------------------------------------
SInputSignature* CResourceManager::_CreateInputSignature(ID3DBlob* pBlob)
{
	VERIFY(pBlob);

	for (u32 it = 0; it < v_input_signature.size(); it++)
	{
		SInputSignature* sign = v_input_signature[it];
		if ((pBlob->GetBufferSize() == sign->signature->GetBufferSize()) &&
			(!(memcmp(pBlob->GetBufferPointer(), sign->signature->GetBufferPointer(), pBlob->GetBufferSize()))))
		{
			return sign;
		}
	}

	SInputSignature* pSign = new SInputSignature(pBlob);

	pSign->dwFlags |= xr_resource_flagged::RF_REGISTERED;
	v_input_signature.push_back(pSign);

	return pSign;
}
//--------------------------------------------------------------------------------------------------------------
void CResourceManager::_DeleteInputSignature(const SInputSignature* pSignature)
{
	if (0 == (pSignature->dwFlags & xr_resource_flagged::RF_REGISTERED))
		return;
	if (reclaim(v_input_signature, pSignature))
		return;
	Msg("! ERROR: Failed to find compiled constant buffer");
}

//--------------------------------------------------------------------------------------------------------------
IBlender* CResourceManager::_GetBlender(LPCSTR Name)
{
	R_ASSERT(Name && Name[0]);

	LPSTR N = LPSTR(Name);
	map_Blender::iterator I = m_blenders.find(N);
#ifdef _EDITOR
	if (I == m_blenders.end())
		return 0;
#else
	//	TODO: DX10: When all shaders are ready switch to common path
	if (I == m_blenders.end())
	{
		Msg("DX10: Shader '%s' not found in library.", Name);
		return 0;
	}
	if (I == m_blenders.end())
	{
		xrDebug::Fatal(DEBUG_INFO, "Shader '%s' not found in library.", Name);
		return nullptr;
	}
#endif
	else
		return I->second;
}

IBlender* CResourceManager::_FindBlender(LPCSTR Name)
{
	if (!(Name && Name[0]))
		return nullptr;

	LPSTR N = LPSTR(Name);
	map_Blender::iterator I = m_blenders.find(N);
	if (I == m_blenders.end())
		return nullptr;
	else
		return I->second;
}

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
void CResourceManager::_ParseList(sh_list& dest, LPCSTR names)
{
	if (nullptr == names || 0 == names[0])
		names = "$null";

	dest.clear();
	char* P = (char*)names;
	svector<char, 128> N;

	while (*P)
	{
		if (*P == ',')
		{
			// flush
			N.push_back(0);
			xr_strlwr(N.begin());

			fix_texture_name(N.begin());
			//. andy			if (strext(N.begin())) *strext(N.begin())=0;
			dest.push_back(N.begin());
			N.clear();
		}
		else
		{
			N.push_back(*P);
		}
		P++;
	}
	if (N.size())
	{
		// flush
		N.push_back(0);
		xr_strlwr(N.begin());

		fix_texture_name(N.begin());
		//. andy		if (strext(N.begin())) *strext(N.begin())=0;
		dest.push_back(N.begin());
	}
}

ShaderElement* CResourceManager::_CreateElement(ShaderElement& S)
{
	if (S.passes.empty())
		return nullptr;

	// Search equal in shaders array
	for (u32 it = 0; it < v_elements.size(); it++)
		if (S.equal(*(v_elements[it])))
			return v_elements[it];

	// Create _new_ entry
	ShaderElement* N = new ShaderElement();
	N->_copy(S);
	N->dwFlags |= xr_resource_flagged::RF_REGISTERED;
	v_elements.push_back(N);
	return N;
}

void CResourceManager::_DeleteElement(const ShaderElement* S)
{
	if (0 == (S->dwFlags & xr_resource_flagged::RF_REGISTERED))
		return;
	if (reclaim(v_elements, S))
		return;
	Msg("! ERROR: Failed to find compiled 'shader-element'");
}

Shader* CResourceManager::_cpp_Create(
	IBlender* B, LPCSTR s_shader, LPCSTR s_textures, LPCSTR s_constants, LPCSTR s_matrices)
{
	CBlender_Compile C;
	Shader* N = new Shader();

	//.
	// if (strstr(s_shader,"transparent"))	__asm int 3;

	// Access to template
	C.BT = B;
	C.bEditor = FALSE;
	C.bDetail = FALSE;
#ifdef _EDITOR
	<<<<<< < HEAD
		if (!C.BT)
		{
			ELog.Msg(mtError, "Can't find shader '%s'", s_shader);
			return 0;
		}
	C.bEditor = TRUE;
#else
	UNUSED(s_shader);
#endif

	// Parse names
	_ParseList(C.L_textures, s_textures);
	_ParseList(C.L_constants, s_constants);
	_ParseList(C.L_matrices, s_matrices);

	// Compile element	(LOD0 - HQ)
	{
		C.iElement = 0;
		C.bDetail = m_textures_description.GetDetailTexture(C.L_textures[0], C.detail_texture, C.detail_scaler);
		//.		C.bDetail			= _GetDetailTexture(*C.L_textures[0],C.detail_texture,C.detail_scaler);
		ShaderElement E;
		C._cpp_Compile(&E);
		N->E[0] = _CreateElement(E);
	}

	// Compile element	(LOD1)
	{
		C.iElement = 1;
		//.		C.bDetail			= _GetDetailTexture(*C.L_textures[0],C.detail_texture,C.detail_scaler);
		C.bDetail = m_textures_description.GetDetailTexture(C.L_textures[0], C.detail_texture, C.detail_scaler);
		ShaderElement E;
		C._cpp_Compile(&E);
		N->E[1] = _CreateElement(E);
	}

	// Compile element
	{
		C.iElement = 2;
		C.bDetail = FALSE;
		ShaderElement E;
		C._cpp_Compile(&E);
		N->E[2] = _CreateElement(E);
	}

	// Compile element
	{
		C.iElement = 3;
		C.bDetail = FALSE;
		ShaderElement E;
		C._cpp_Compile(&E);
		N->E[3] = _CreateElement(E);
	}

	// Compile element
	{
		C.iElement = 4;
		C.bDetail = TRUE; //.$$$ HACK :)
		ShaderElement E;
		C._cpp_Compile(&E);
		N->E[4] = _CreateElement(E);
	}

	// Compile element
	{
		C.iElement = 5;
		C.bDetail = FALSE;
		ShaderElement E;
		C._cpp_Compile(&E);
		N->E[5] = _CreateElement(E);
	}

	// Search equal in shaders array
	for (auto & sheder : v_shaders)
	{
		if (N->equal(sheder))
		{
			xr_delete(N);
			return sheder;
		}
	}

	N->dwFlags |= xr_resource_flagged::RF_REGISTERED;
	v_shaders.push_back(N);
	return N;
}

Shader* CResourceManager::_cpp_Create(LPCSTR s_shader, LPCSTR s_textures, LPCSTR s_constants, LPCSTR s_matrices)
{
	//	TODO: DX10: When all shaders are ready switch to common path
	IBlender* pBlender = _GetBlender(s_shader ? s_shader : "null");
	if (!pBlender)
		return nullptr;
	return _cpp_Create(pBlender, s_shader, s_textures, s_constants, s_matrices);
}

Shader* CResourceManager::Create(IBlender* B, LPCSTR s_shader, LPCSTR s_textures, LPCSTR s_constants, LPCSTR s_matrices)
{
	return _cpp_Create(B, s_shader, s_textures, s_constants, s_matrices);
}

Shader* CResourceManager::Create(LPCSTR s_shader, LPCSTR s_textures, LPCSTR s_constants, LPCSTR s_matrices)
{
	//	TODO: DX10: When all shaders are ready switch to common path
	if (_lua_HasShader(s_shader))
		return _lua_Create(s_shader, s_textures);
	else
	{
		Shader* pShader = _cpp_Create(s_shader, s_textures, s_constants, s_matrices);
		if (pShader)
			return pShader;
		else
		{
			if (_lua_HasShader("stub_default"))
				return _lua_Create("stub_default", s_textures);
			else
			{
				FATAL("Can't find stub_default.s");
				return 0;
			}
		}
	}
}

void CResourceManager::Delete(const Shader* S)
{
	if (0 == (S->dwFlags & xr_resource_flagged::RF_REGISTERED))
		return;
	if (reclaim(v_shaders, S))
		return;
	Msg("! ERROR: Failed to find complete shader");
}

//void CResourceManager::DeferredUpload()
//{
//	if (!Device.b_is_Ready.load() || Device.IsReset())
//		return;
//	
//	//static tbb::task_group parallel;
//	//std::atomic_uint texture_it = 0;
//	//size_t texture_send = 0;
//	//size_t size_texture = m_textures.size();
//	//if (pApp->IsLoadingScreen())
//	//{
//	//	parallel.run([&]() {
//	//		while (texture_send < 40)
//	//		{
//	//			const size_t result = size_t((float(texture_it) / size_texture) * 41);
//	//			if (result != texture_send)
//	//			{
//	//				pApp->SetLoadStageTitle("st_loading_textures");
//	//				texture_send = result;
//	//			}
//	//		}
//	//	});
//	//}
//
//	CHECK_TIME("Resource Manager DeferredUpload",
//		tbb::parallel_for_each(m_textures, [](auto && m_tex)
//		{
//			m_tex.second->Load();
//			//texture_it++;
//		});
//	)
//
//	//parallel.wait();
//}
//
//void CResourceManager::DeferredUnload()
//{
//	if (!Device.b_is_Ready.load() || Device.IsReset())
//		return;
//
//	CHECK_TIME("Resource Manager DeferredUnload",
//		tbb::parallel_for_each(m_textures, [](auto && m_tex)
//		{
//			m_tex.second->Unload();
//		});
//	)
//}

void CResourceManager::_GetMemoryUsage(u32& m_base, u32& c_base, u32& m_lmaps, u32& c_lmaps)
{
	m_base = c_base = m_lmaps = c_lmaps = 0;

	map_Texture::iterator I = m_textures.begin();
	map_Texture::iterator E = m_textures.end();
	for (; I != E; I++)
	{
		u32 m = I->second->flags.MemoryUsage.load();
		if (strstr(I->first, "lmap"))
		{
			c_lmaps++;
			m_lmaps += m;
		}
		else
		{
			c_base++;
			m_base += m;
		}
	}
}

void CResourceManager::_DumpMemoryUsage()
{
	xr_multimap<u32, std::pair<u32, shared_str>> mtex;

	// sort
	{
		map_Texture::iterator I = m_textures.begin();
		map_Texture::iterator E = m_textures.end();
		for (; I != E; I++)
		{
			u32 m = I->second->flags.MemoryUsage.load();
			shared_str n = I->second->cName;
			mtex.insert(std::make_pair(m, std::make_pair(I->second->dwReference.load(), n)));
		}
	}

	// dump
	if (Core.ParamFlags.test(Core.verboselog))
	{
		xr_multimap<u32, std::pair<u32, shared_str>>::iterator I = mtex.begin();
		xr_multimap<u32, std::pair<u32, shared_str>>::iterator E = mtex.end();
		for (; I != E; I++)
			Msg("* %4.1f : [%4d] %s", float(I->first) / 1024.f, I->second.first, I->second.second.c_str());
	}
}

//void CResourceManager::Evict()
//{
//	//	TODO: DX10: check if we really need this method
//#if !defined(USE_DX11)
//	CHK_DX(HW.pDevice->EvictManagedResources());
//#endif
//}