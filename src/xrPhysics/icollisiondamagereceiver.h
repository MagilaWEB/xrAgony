#pragma once
#include "xrPhysics.h"
#include "xrCore/_types.h"

// fwd. decl.
template <class T> struct _vector3;
using Fvector = _vector3<float>;
struct SGameMtl;

class ICollisionDamageReceiver
{
public:
	virtual void CollisionHit(u16 source_id, u16 bone_id, float power, const Fvector& dir, Fvector& pos) = 0;

protected:
	virtual ~ICollisionDamageReceiver() = 0 {};
};

struct dContact;
struct SGameMtl;
XRPHYSICS_API void DamageReceiverCollisionCallback(
	bool& do_colide, bool bo1, dContact& c, SGameMtl* material_1, SGameMtl* material_2);
XRPHYSICS_API void BreakableObjectCollisionCallback(
	bool& do_colide, bool bo1, dContact& c, SGameMtl* material_1, SGameMtl* material_2);
