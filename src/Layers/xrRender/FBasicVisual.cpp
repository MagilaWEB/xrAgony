// dxRender_Visual.cpp: implementation of the dxRender_Visual class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#pragma hdrstop

#ifndef _EDITOR
#include "xrEngine/Render.h"
#endif // #ifndef _EDITOR

#include "FBasicVisual.h"
#include "xrCore/FMesh.hpp"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

IRender_Mesh::~IRender_Mesh()
{
	_RELEASE(p_rm_Vertices);
	_RELEASE(p_rm_Indices);
}

dxRender_Visual::dxRender_Visual()
{
	m_random_period_dist_update_time = ::Random.randI(300, 900);
	Type = 0;
	shader = nullptr;
	vis.clear();
}

dxRender_Visual::~dxRender_Visual() {}
void dxRender_Visual::Release() {}
// CStatTimer						tscreate;

void dxRender_Visual::Load(const char* N, IReader* data, u32)
{
#ifdef DEBUG
	dbg_name = N;
#endif

	// header
	VERIFY(data);
	ogf_header hdr;
	if (data->r_chunk_safe(OGF_HEADER, &hdr, sizeof(hdr)))
	{
		R_ASSERT2(hdr.format_version == xrOGF_FormatVersion, "Invalid visual version");
		Type = hdr.type;
		// if (hdr.shader_id)	shader	= ::Render->getShader	(hdr.shader_id);
		if (hdr.shader_id)
			shader = ::RImplementation.getShader(hdr.shader_id);
		vis.box.set(hdr.bb.min, hdr.bb.max);
		vis.sphere.set(hdr.bs.c, hdr.bs.r);
	}
	else
	{
		FATAL("Invalid visual");
	}

	// Shader
	if (data->find_chunk(OGF_TEXTURE))
	{
		string256 fnT, fnS;
		data->r_stringZ(fnT, sizeof(fnT));
		data->r_stringZ(fnS, sizeof(fnS));
		shader.create(fnS, fnT);
	}

	// desc
#ifdef _EDITOR
	if (data->find_chunk(OGF_S_DESC))
		desc.Load(*data);
#endif
}

#define PCOPY(a) a = pFrom->a
void dxRender_Visual::Copy(dxRender_Visual* pFrom)
{
	PCOPY(Type);
	PCOPY(shader);
	PCOPY(vis);
#ifdef _EDITOR
	PCOPY(desc);
#endif
#ifdef DEBUG
	PCOPY(dbg_name);
#endif
}

void dxRender_Visual::update_distance_to_camera(Fmatrix* transform_matrix)
{
	if (m_next_distance_update_time < ::IDevice->TimeGlobal_ms())
	{
		if (transform_matrix)
		{
			Fvector pos;
			transform_matrix->transform_tiny(pos, getVisData().sphere.P);
			m_distance = ::IDevice->cast()->vCameraPosition.distance_to_sqr(pos);
		}
		else
			m_distance = ::IDevice->cast()->vCameraPosition.distance_to_sqr(getVisData().sphere.P);

		m_next_distance_update_time = ::IDevice->TimeGlobal_ms() + m_random_period_dist_update_time;

		if (!callback_check_dist.empty())
		{
			callback_check_dist(m_distance);
			callback_check_dist.clear();
		}
	}
}

float dxRender_Visual::get_distance_to_camera_base() const
{
	return m_distance;
}

float dxRender_Visual::getDistanceToCamera() const
{
	return get_distance_to_camera_base() * ::IDevice->cast()->iZoomSqr;
}