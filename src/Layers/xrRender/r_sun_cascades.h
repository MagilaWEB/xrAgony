#pragma once

namespace sun
{
struct ray
{
	ray() {}
	ray(Fvector3 const& _P, Fvector3 const& _D) : D(_D), P(_P) {}
	Fvector3 D;
	Fvector3 P;
};

struct cascade
{
	cascade() : reset_chain(false) {}
	Fmatrix xform;
	ray rays[4];
	float size;
	float bias;
	bool reset_chain;
};

} // namespace sun
