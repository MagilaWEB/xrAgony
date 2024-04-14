#pragma once

#include "particle_actions.h"

namespace PAPI
{
	class CParticleManager : public IParticleManager
	{
		// These are static because all threads access the same effects.
		// All accesses to these should be locked.
		DEFINE_VECTOR(ParticleEffect*, ParticleEffectVec, ParticleEffectVecIt);
		DEFINE_VECTOR(ParticleActions*, ParticleActionsVec, ParticleActionsVecIt);
		ParticleEffectVec			effect_vec;
		ParticleActionsVec			m_alist_vec;
		xrCriticalSection			pm_lock;
	public:
		CParticleManager();
		~CParticleManager();
		// Return an index into the list of particle effects where
		ParticleEffect*		GetEffectPtr(int effect_id);
		ParticleActions*	GetActionListPtr(int alist_id);

		// create&destroy
		int					CreateEffect(u32 max_particles);
		void				DestroyEffect(int effect_id);
		int					CreateActionList();
		void				DestroyActionList(int alist_id);

		// control
		void				PlayEffect(int effect_id, int alist_id);
		void				StopEffect(int effect_id, int alist_id, bool deffered = true);

		// update&render
		void				Update(int effect_id, int alist_id, float dt);
		void				Render(int effect_id);
		void				Transform(int alist_id, const Fmatrix& m, const Fvector& velocity);

		// effect
		void				RemoveParticle(int effect_id, u32 p_id);
		void				SetMaxParticles(int effect_id, u32 max_particles);
		void				SetCallback(int effect_id, OnBirthParticleCB b, OnDeadParticleCB d, void* owner, u32 param);
		void				GetParticles(int effect_id, Particle*& particles, u32& cnt);
		u32					GetParticlesCount(int effect_id);

		// action
		ParticleAction*		CreateAction(PActionEnum action_id);
		u32					LoadActions(int alist_id, IReader& R);
		void				SaveActions(int alist_id, IWriter& W);
	};
} // namespace PAPI
