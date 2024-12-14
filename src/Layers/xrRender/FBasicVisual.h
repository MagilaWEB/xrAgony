#pragma once

#include "xrEngine/vis_common.h"

#include "Include/xrRender/RenderVisual.h"

#define VLOAD_NOVERTICES (1 << 0)

// The class itself
class CKinematicsAnimated;
class CKinematics;
class IParticleCustom;

struct IRender_Mesh
{
	// format
	ref_geom rm_geom;

	// verts
	ID3DVertexBuffer* p_rm_Vertices;

	u32 vBase;
	u32 vCount;

	// indices
	ID3DIndexBuffer* p_rm_Indices;

	u32 iBase;
	u32 iCount;
	u32 dwPrimitives;

	IRender_Mesh()
	{
		p_rm_Vertices = 0;
		p_rm_Indices = 0;
	}
	virtual ~IRender_Mesh();

private:
	IRender_Mesh(const IRender_Mesh& other);
	void operator=(const IRender_Mesh& other);
};

// The class itself
class ECORE_API dxRender_Visual : public IRenderVisual
{
#ifdef DEBUG
public:
	shared_str dbg_name;
	virtual shared_str getDebugName() { return dbg_name; }
#endif
public:
	// Common data for rendering
	u32 Type; // visual's type
	vis_data vis; // visibility-data
	ref_shader shader; // pipe state, shared

	dxRender_Visual();
	virtual ~dxRender_Visual();

	virtual void Render(float /*LOD*/) {} // LOD - Level Of Detail  [0..1], Ignored
	virtual void Load(const char* N, IReader* data, u32 dwFlags);
	virtual void Release(); // Shared memory release
	virtual void Copy(dxRender_Visual* from);
	virtual void Spawn() {};
	virtual void Depart() {};

	//	virtual	CKinematics*		dcast_PKinematics			()				{ return 0;	}
	//	virtual	CKinematicsAnimated*dcast_PKinematicsAnimated	()				{ return 0;	}
	//	virtual IParticleCustom*	dcast_ParticleCustom		()				{ return 0;	}

	vis_data& getVisData() override { return vis; };
	u32 getType() override { return Type; };

	
	fastdelegate::FastDelegate<void(float dist)>& get_callback_dist()
	{
		return callback_check_dist;
	};

	void update_distance_to_camera(Fmatrix* transform_matrix = nullptr) override;
	float get_distance_to_camera_base() const override;
	float getDistanceToCamera() const override;
private:
	fastdelegate::FastDelegate<void(float dist)> callback_check_dist;
	float m_distance{ 0.f };
	size_t m_next_distance_update_time{ 0 };
	size_t m_random_period_dist_update_time{ 0 };
};