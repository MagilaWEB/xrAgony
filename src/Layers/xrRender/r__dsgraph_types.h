#pragma once
#include "xrCore/Containers/FixedMap.h"

class dxRender_Visual;

namespace R_dsgraph
{
// Elementary types
struct _NormalItem
{
	float ssa;
	dxRender_Visual* pVisual;
};

struct _MatrixItem
{
	float				ssa;
	IRenderable* pObject;
	dxRender_Visual* pVisual;
	Fmatrix				Matrix;				// matrix (copy)
};

struct _MatrixItemS : public _MatrixItem
{
	ShaderElement* se;
};

struct _LodItem
{
	float ssa;
	dxRender_Visual* pVisual;
};

using ps_type = ID3DPixelShader*;
using vs_type = SVS*;
using gs_type = ID3DGeometryShader*;
using hs_type = ID3D11HullShader*;
using ds_type = ID3D11DomainShader*;

// NORMAL
using mapNormalDirect = xr_vector<_NormalItem>;

struct mapNormalItems : public mapNormalDirect
{
	float ssa;
};

struct mapNormalTextures : public xr_fixed_map<STextureList*, mapNormalItems>
{
	float ssa;
};

struct mapNormalStates : public xr_fixed_map<ID3DState*, mapNormalTextures>
{
	float ssa;
};

struct mapNormalCS : public xr_fixed_map<R_constant_table*, mapNormalStates>
{
	float ssa;
};

struct mapNormalAdvStages
{
	hs_type hs;
	ds_type ds;
	mapNormalCS mapCS;
};

struct mapNormalPS : public xr_fixed_map<ps_type, mapNormalAdvStages>
{
	float ssa;
};

struct mapNormalGS : public xr_fixed_map<gs_type, mapNormalPS>
{
	float ssa;
};

struct mapNormalVS : public xr_fixed_map<vs_type, mapNormalGS> {};

using mapNormal_T = mapNormalVS;
using mapNormalPasses_T = mapNormal_T[SHADER_PASSES_MAX];

// MATRIX
using mapMatrixDirect = xr_vector<_MatrixItem>;

struct mapMatrixItems : public mapMatrixDirect
{
	float ssa;
};

struct mapMatrixTextures : public xr_fixed_map<STextureList*, mapMatrixItems>
{
	float ssa;
};

struct mapMatrixStates : public xr_fixed_map<ID3DState*, mapMatrixTextures>
{
	float ssa;
};

struct mapMatrixCS : public xr_fixed_map<R_constant_table*, mapMatrixStates>
{
	float ssa;
};

struct mapMatrixAdvStages
{
	hs_type hs;
	ds_type ds;
	mapMatrixCS mapCS;
};

struct mapMatrixPS : public xr_fixed_map<ps_type, mapMatrixAdvStages>
{
	float ssa;
};

struct mapMatrixGS : public xr_fixed_map<gs_type, mapMatrixPS>
{
	float ssa;
};

struct mapMatrixVS : public xr_fixed_map<vs_type, mapMatrixGS> {};

using mapMatrix_T = mapMatrixVS;
using mapMatrixPasses_T = mapMatrix_T[SHADER_PASSES_MAX];

// Top level
using mapSorted_T = xr_fixed_map<float, _MatrixItemS>;
using mapSorted_Node = mapSorted_T::value_type;

using mapHUD_T	= xr_fixed_map<float, _MatrixItemS>;
using mapHUD_Node = mapHUD_T::value_type;

using mapLOD_T	= xr_fixed_map<float, _LodItem>;
using mapLOD_Node = mapLOD_T::value_type;
}
