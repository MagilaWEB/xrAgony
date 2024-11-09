#include "stdafx.h"
#pragma hdrstop
#include "DetailManager.h"
#ifdef _EDITOR
#include "IGame_Persistent.h"
#include "Environment.h"
#else
#include "xrEngine/IGame_Persistent.h"
#include "xrEngine/Environment.h"
#endif
#include "Layers/xrRenderDX10/dx10BufferUtils.h"

const int quant = 16384;
const int c_size = 4;

static D3DVERTEXELEMENT9 dwDecl[] = {
	{0, 0, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0}, // pos
	{0, 12, D3DDECLTYPE_SHORT4, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0}, // uv
	D3DDECL_END()
};

#pragma pack(push,1)
struct vertHW
{
	float x, y, z;
	short u, v, t, mid;
};
#pragma pack(pop)

short QC(float v)
{
	int t = iFloor(v * float(quant));
	clamp(t, -32768, 32767);
	return short(t & 0xffff);
}

float CDetailManager::hw_timeDelta()
{
	float fDelta = IDevice->TimeGlobal_sec() - m_global_time_old;

	if ((fDelta < 0) || (fDelta > 1))
		fDelta = 0.03f;

	m_global_time_old = IDevice->TimeGlobal_sec();

	return fDelta;
}

void CDetailManager::hw_Load()
{
	hw_Load_Geom();
	hw_Load_Shaders();
}

void CDetailManager::hw_Load_Geom()
{
	// Analyze batch-size
	hw_BatchSize = (u32(HW.Caps.geometry.dwRegisters) - 10) / c_size;
	clamp(hw_BatchSize, (u32)0, (u32)64);
	Msg("* [DETAILS] VertexConsts(%d), Batch(%d)", u32(HW.Caps.geometry.dwRegisters), hw_BatchSize);

	// Pre-process objects
	u32 dwVerts = 0;
	u32 dwIndices = 0;
	for (CDetail*& D : objects)
	{
		dwVerts += D->number_vertices * hw_BatchSize;
		dwIndices += D->number_indices * hw_BatchSize;
	}
	u32 vSize = sizeof(vertHW);
	Msg("* [DETAILS] %d v(%d), %d p", dwVerts, vSize, dwIndices / 3);
	Msg("* [DETAILS] Batch(%d), VB(%dK), IB(%dK)", hw_BatchSize, (dwVerts * vSize) / 1024, (dwIndices * 2) / 1024);

	// Fill VB
	{
		vertHW* pV;
		vertHW* pVOriginal;
		pVOriginal = xr_alloc<vertHW>(dwVerts);
		pV = pVOriginal;
		for (CDetail*& D : objects)
		{
			for (u32 batch = 0; batch < hw_BatchSize; batch++)
			{
				u32 mid = batch * c_size;
				for (u32 v = 0; v < D->number_vertices; v++)
				{
					const Fvector& vP = D->vertices[v].P;
					pV->x = vP.x;
					pV->y = vP.y;
					pV->z = vP.z;
					pV->u = QC(D->vertices[v].u);
					pV->v = QC(D->vertices[v].v);
					pV->t = QC(vP.y / (D->bv_bb.vMax.y - D->bv_bb.vMin.y));
					pV->mid = short(mid);
					pV++;
				}
			}
		}
		R_CHK(dx10BufferUtils::CreateVertexBuffer(&hw_VB, pVOriginal, dwVerts * vSize));
		HW.stats_manager.increment_stats_vb(hw_VB);
		xr_free(pVOriginal);
	}

	// Fill IB
	{
		u16* pI;
		u16* pIOriginal;
		pIOriginal = xr_alloc<u16>(dwIndices);
		pI = pIOriginal;
		for (const CDetail* D : objects)
		{
			u16 offset = 0;
			for (u32 batch = 0; batch < hw_BatchSize; batch++)
			{
				for (u32 i = 0; i < u32(D->number_indices); i++)
					*pI++ = u16(u16(D->indices[i]) + u16(offset));
				offset = u16(offset + u16(D->number_vertices));
			}
		}
		R_CHK(dx10BufferUtils::CreateIndexBuffer(&hw_IB, pIOriginal, dwIndices * 2));
		HW.stats_manager.increment_stats_ib(hw_IB);
		xr_free(pIOriginal);
	}

	// Declare geometry
	hw_Geom.create(dwDecl, hw_VB, hw_IB);
}

void CDetailManager::hw_Unload()
{
	// Destroy VS/VB/IB
	hw_Geom.destroy();
	HW.stats_manager.decrement_stats_vb(hw_VB);
	HW.stats_manager.decrement_stats_ib(hw_IB);
	_RELEASE(hw_IB);
	_RELEASE(hw_VB);
}

void CDetailManager::hw_Load_Shaders()
{
	// Create shader to access constant storage
	ref_shader S;
	S.create("details\\set");
	R_constant_table& T0 = *(S->E[0]->passes[0]->constants);
	R_constant_table& T1 = *(S->E[1]->passes[0]->constants);
	hwc_consts = T0.get("consts");
	hwc_wave = T0.get("wave");
	hwc_wind = T0.get("dir2D");
	hwc_array = T0.get("array");
	hwc_s_consts = T1.get("consts");
	hwc_s_xform = T1.get("xform");
	hwc_s_array = T1.get("array");
}

void CDetailManager::hw_Render(VisiblesType type)
{
	// Render-prepare
	//	Update timer
	float fDelta = hw_timeDelta();

	m_time_rot_1 += (PI_MUL_2 * fDelta / swing_current.rot1);
	m_time_rot_2 += (PI_MUL_2 * fDelta / swing_current.rot2);
	m_time_pos += (swing_current.speed * fDelta);

	//float tm_rot1 = (PI_MUL_2*IDevice->TimeGlobal_sec()/swing_current.rot1);
	//float tm_rot2 = (PI_MUL_2*IDevice->TimeGlobal_sec()/swing_current.rot2);
	float tm_rot1 = m_time_rot_1;
	float tm_rot2 = m_time_rot_2;

	Fvector4 dir1, dir2;
	dir1.set(_sin(tm_rot1), 0, _cos(tm_rot1), 0).normalize().mul(swing_current.amp1);
	dir2.set(_sin(tm_rot2), 0, _cos(tm_rot2), 0).normalize().mul(swing_current.amp2);

	// Setup geometry and DMA
	RCache.set_Geometry(hw_Geom);

	auto v_list = [&](u32 var_id)
	{
		if (type == SHADOW_SUN)
			return m_visibles_shadow_sun[var_id];
		else if (type == SHADOW_LIGHT)
			return m_visibles_shadow_light[var_id];

		return m_visibles[var_id];
	};

	// Wave0
	float scale = 1.f / float(quant);
	Fvector4 wave;
	Fvector4 consts;
	consts.set(scale, scale, .25f, .9f);
	wave.set(1.f / 5.f, 1.f / 7.f, 1.f / 3.f, m_time_pos);
	hw_Render_dump(consts, wave.div(PI_MUL_2), dir1, 1, 0, v_list(1));

	// Wave1
	wave.set(1.f / 3.f, 1.f / 7.f, 1.f / 5.f, m_time_pos);
	hw_Render_dump(consts, wave.div(PI_MUL_2), dir2, 2, 0, v_list(2));

	// Still
	consts.set(scale, scale, scale, 1.f);
	hw_Render_dump(consts, wave.div(PI_MUL_2), dir2, 0, 1, v_list(0));
}

void CDetailManager::hw_Render_dump(const Fvector4& consts, const Fvector4& wave, const Fvector4& wind, u32 var_id, u32 lod_id, const vis_list v_list)
{
	//Device.Statistic->RenderDUMP_DT_Count = 0;
	static shared_str strConsts("consts");
	static shared_str strWave("wave");
	static shared_str strDir2D("dir2D");
	static shared_str strArray("array");
	static shared_str strXForm("xform");

	// Matrices and offsets
	size_t vOffset = 0;
	size_t iOffset = 0;

	CEnvDescriptor& desc = *g_pGamePersistent->Environment().CurrentEnv;
	Fvector
		c_sun{ desc.sun_color },
		c_ambient{ desc.ambient },
		c_hemi{ desc.hemi_color.x, desc.hemi_color.y, desc.hemi_color.z };

	c_sun.mul(.5f);

	// Iterate
	for (size_t O = 0; O < objects.size(); O++)
	{
		CDetail& Object = *objects[O];
		concurrent_vector <SlotItem*> vis = v_list[O];

		if (!vis.empty())
		{
			// Setup matrices + colors (and flush it as nesessary)
			RCache.set_Element(Object.shader->E[lod_id]);
			RImplementation.apply_lmaterial();

			//	This could be cached in the corresponding consatant buffer
			//	as it is done for DX9
			RCache.set_c(strConsts, consts);
			RCache.set_c(strWave, wave);
			RCache.set_c(strDir2D, wind);
			RCache.set_c(strXForm, Device.mFullTransform);

			//	Map constants to memory directly
			constexpr size_t size_vector4 = sizeof(Fvector4) * 4;
			Fvector4* c_storage{};
			RCache.get_ConstantDirect(strArray, hw_BatchSize * size_vector4, reinterpret_cast<void**>(&c_storage), nullptr, nullptr);

			size_t dwBatch = 0;
			for (SlotItem* Item : vis)
			{
				size_t base = dwBatch * 4;

				// matrix ( 3x4 matrix, last row - color )
				for(size_t i = 0; i < 4; i++)
					c_storage[base + i] = std::move(Item->build_matrix[i]);

				if (++dwBatch == hw_BatchSize)
				{
					// flush
					//Device.Statistic->RenderDUMP_DT_Count += dwBatch;
					size_t dwCNT_verts = dwBatch * Object.number_vertices;
					size_t dwCNT_prims = (dwBatch * Object.number_indices) / 3;
					RCache.Render(D3DPT_TRIANGLELIST, vOffset, 0, dwCNT_verts, iOffset, dwCNT_prims);
					//RCache.stat.r.s_details.add(dwCNT_verts);
					
					RCache.get_ConstantDirect(strArray, hw_BatchSize * size_vector4, reinterpret_cast<void**>(&c_storage), nullptr, nullptr);
					// restart
					dwBatch = 0;
				}
			}
			// flush if nessecary
			if (dwBatch)
			{
				//Device.Statistic->RenderDUMP_DT_Count += dwBatch;
				size_t dwCNT_verts = dwBatch * Object.number_vertices;
				size_t dwCNT_prims = (dwBatch * Object.number_indices) / 3;
				RCache.Render(D3DPT_TRIANGLELIST, vOffset, 0, dwCNT_verts, iOffset, dwCNT_prims);
				//RCache.stat.r.s_details.add(dwCNT_verts);
			}
		}
		vOffset += hw_BatchSize * Object.number_vertices;
		iOffset += hw_BatchSize * Object.number_indices;
	}
}
