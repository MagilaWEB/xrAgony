#pragma once
#include "xr_collide_defs.h"
#include "Common/Noncopyable.hpp"
#include "Include/xrRender/FactoryPtr.h"
#include "Include/xrRender/ObjectSpaceRender.h"
#include "xrXRC.h"
#include "xrCDB.h"
#include "xrCore/_fbox.h"

// fwd. decl.
class ISpatial;
class ICollisionForm;
class IGameObject;

//-----------------------------------------------------------------------------------------------------------
// Space Area
//-----------------------------------------------------------------------------------------------------------
struct hdrCFORM;
class XRCDB_API CObjectSpace : Noncopyable
{
private:
	// Debug
	xrCriticalSection lock;
	CDB::MODEL Static;
	Fbox m_BoundingVolume;
	xrXRC xrc; // MT: dangerous
	collide::rq_results r_temp; // MT: dangerous
	xr_vector<ISpatial*> r_spatial; // MT: dangerous
public:
#ifdef DEBUG
	FactoryPtr<IObjectSpaceRender>* m_pRender;
// ref_shader							sh_debug;
// clQueryCollision					q_debug;			// MT: dangerous
// xr_vector<std::pair<Fsphere,u32> >	dbg_S;				// MT: dangerous
#endif

private:
	BOOL _RayTest(const Fvector& start, const Fvector& dir, float range, collide::rq_target tgt,
		collide::ray_cache* cache, IGameObject* ignore_object);
	BOOL _RayPick(const Fvector& start, const Fvector& dir, float range, collide::rq_target tgt, collide::rq_result& R,
		IGameObject* ignore_object);
	BOOL _RayQuery(collide::rq_results& dest, const collide::ray_defs& rq, collide::rq_callback cb, LPVOID user_data,
		collide::test_callback tb, IGameObject* ignore_object);
	BOOL _RayQuery2(collide::rq_results& dest, const collide::ray_defs& rq, collide::rq_callback cb, LPVOID user_data,
		collide::test_callback tb, IGameObject* ignore_object);
	BOOL _RayQuery3(collide::rq_results& dest, const collide::ray_defs& rq, collide::rq_callback cb, LPVOID user_data,
		collide::test_callback tb, IGameObject* ignore_object);

public:
	CObjectSpace();
	~CObjectSpace();

	void Load(CDB::build_callback build_callback);
	void Load(LPCSTR path, LPCSTR fname, CDB::build_callback build_callback);
	void Load(IReader* R, CDB::build_callback build_callback);
	void Create(Fvector* verts, CDB::TRI* tris, const hdrCFORM& H, CDB::build_callback build_callback);
	// Occluded/No
	BOOL RayTest(const Fvector& start, const Fvector& dir, float range, collide::rq_target tgt,
		collide::ray_cache* cache, IGameObject* ignore_object);

	// Game raypick (nearest) - returns object and addititional params
	BOOL RayPick(const Fvector& start, const Fvector& dir, float range, collide::rq_target tgt, collide::rq_result& R,
		IGameObject* ignore_object);

	// General collision query
	BOOL RayQuery(collide::rq_results& dest, const collide::ray_defs& rq, collide::rq_callback cb, LPVOID user_data,
		collide::test_callback tb, IGameObject* ignore_object);
	BOOL RayQuery(collide::rq_results& dest, ICollisionForm* target, const collide::ray_defs& rq);

	bool BoxQuery(Fvector const& box_center, Fvector const& box_z_axis, Fvector const& box_y_axis,
		Fvector const& box_sizes, xr_vector<Fvector>* out_tris);

	int GetNearest(xr_vector<IGameObject*>& q_nearest, ICollisionForm* obj, float range);
	int GetNearest(xr_vector<IGameObject*>& q_nearest, const Fvector& point, float range, IGameObject* ignore_object);
	int GetNearest(xr_vector<ISpatial*>& q_spatial, xr_vector<IGameObject*>& q_nearest, const Fvector& point,
		float range, IGameObject* ignore_object);

	CDB::TRI* GetStaticTris() { return Static.get_tris(); }
	Fvector* GetStaticVerts() { return Static.get_verts(); }
	CDB::MODEL* GetStaticModel() { return &Static; }
	const Fbox& GetBoundingVolume() { return m_BoundingVolume; }
// Debugging
#ifdef DEBUG
	void dbgRender();
// ref_shader							dbgGetShader		()	{ return sh_debug;	}
#endif
	void DumpStatistics(IGameFont& font, IPerformanceAlert* alert);
};
