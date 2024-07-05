#include "stdafx.h"
#include "ParticlesObject.h"
#include "xrEngine/defines.h"
#include "Include/xrRender/RenderVisual.h"
#include "Include/xrRender/ParticleCustom.h"
#include "xrEngine/render.h"
#include "xrEngine/IGame_Persistent.h"
#include "xrEngine/Environment.h"


const Fvector zero_vel = { 0.f,0.f,0.f };

CParticlesObject::CParticlesObject(LPCSTR p_name, BOOL bAutoRemove, bool destroy_on_game_load) :
	inherited(destroy_on_game_load)
{
	Init(p_name, 0, bAutoRemove);
}

void CParticlesObject::Init(LPCSTR p_name, IRender_Sector* S, BOOL bAutoRemove)
{
	xrCriticalSection::raii mt{ po_lock };
	m_bLooped = false;
	m_bStopping = false;
	m_bAutoRemove = bAutoRemove;
	float time_limit = 0.0f;

	// create visual
	renderable.visual = ::Render->model_CreateParticles(p_name);
	VERIFY(renderable.visual);
	IParticleCustom* V = smart_cast<IParticleCustom*>(renderable.visual);  VERIFY(V);
	time_limit = V->GetTimeLimit();

	if (time_limit > 0.f)
	{
		m_iLifeTime = iFloor(time_limit * 1000.f);
	}
	else
	{
		if (bAutoRemove)
		{
			R_ASSERT3(!m_bAutoRemove, "Can't set auto-remove flag for looped particle system.", p_name);
		}
		else
		{
			m_iLifeTime = 0;
			m_bLooped = true;
		}
	}


	// spatial
	spatial.type = 0;
	spatial.sector = S;

	// sheduled
	shedule.t_min = 20;
	shedule.t_max = 50;
	shedule_register();

	dwLastTime = Device.dwTimeGlobal;
}

//----------------------------------------------------
CParticlesObject::~CParticlesObject()
{
	Device.remove_parallel2(this, &CParticlesObject::PerformAllTheWork);
	xrCriticalSection::raii mt{ po_lock };

	//	we do not need this since CPS_Instance does it
	//	shedule_unregister		();
}

void CParticlesObject::UpdateSpatial()
{
	// spatial	(+ workaround occasional bug inside particle-system)
	vis_data& vis = renderable.visual->getVisData();
	if (_valid(vis.sphere))
	{
		Fvector	P;	float	R;
		renderable.xform.transform_tiny(P, vis.sphere.P);
		R = vis.sphere.R;
		if (0 == spatial.type) {
			// First 'valid' update - register
			spatial.type = STYPE_PARTICLE;
			spatial.sphere.set(P, R);
			spatial_register();
		}
		else {
			BOOL	bMove = FALSE;
			if (!P.similar(spatial.sphere.P, EPS_L * 10.f))		bMove = TRUE;
			if (!fsimilar(R, spatial.sphere.R, 0.15f))			bMove = TRUE;
			if (bMove)
			{
				spatial.sphere.set(P, R);
				spatial_move();
			}
		}
	}
}

const shared_str CParticlesObject::Name()
{
	IParticleCustom* V = smart_cast<IParticleCustom*>(renderable.visual); VERIFY(V);
	return (V) ? V->Name() : "";
}

//----------------------------------------------------
void CParticlesObject::Play(bool bHudMode)
{
	xrCriticalSection::raii mt{ po_lock };
	IParticleCustom* V = smart_cast<IParticleCustom*>(renderable.visual); VERIFY(V);
	if (bHudMode)
		V->SetHudMode(bHudMode);

	V->Play();
	u32 dt = 1000 / Device.FPS;
	clamp<u32>(dt, 1, 32);
	dwLastTime = Device.dwTimeGlobal - dt;
	m_bStopping = false;
	PerformAllTheWork();
}

void CParticlesObject::play_at_pos(const Fvector& pos, BOOL xform)
{
	xrCriticalSection::raii mt{ po_lock };
	IParticleCustom* V = smart_cast<IParticleCustom*>(renderable.visual); VERIFY(V);
	Fmatrix m; m.translate(pos);
	V->UpdateParent(m, zero_vel, xform);
	V->Play();
	u32 dt = 1000 / Device.FPS;
	clamp<u32>(dt, 1, 32);
	dwLastTime = Device.dwTimeGlobal - dt;
	m_bStopping = false;
	PerformAllTheWork();
}

void CParticlesObject::Stop(BOOL bDefferedStop)
{
	xrCriticalSection::raii mt{ po_lock };

	IParticleCustom* V = smart_cast<IParticleCustom*>(renderable.visual); VERIFY(V);
	V->Stop(bDefferedStop);
	m_bStopping = true;
}

void CParticlesObject::shedule_Update(u32 _dt)
{
	inherited::shedule_Update(_dt);

	if (m_bDead)
		return;

	if (psDeviceFlags.test(mtParticles))
		Device.add_parallel2(this, &CParticlesObject::PerformAllTheWork);
	else
		PerformAllTheWork();
}

void CParticlesObject::PerformAllTheWork()
{
	if (m_bDead)
		return;

	xrCriticalSection::raii mt{ po_lock };
	// Update
	u32 dt = Device.dwTimeGlobal - dwLastTime;
	if (dt)
	{
		IParticleCustom* V = smart_cast<IParticleCustom*>(renderable.visual); VERIFY(V);
		V->OnFrame(dt);
		dwLastTime = Device.dwTimeGlobal;
	}
	UpdateSpatial();
}

void CParticlesObject::SetXFORM(const Fmatrix& m)
{
	xrCriticalSection::raii mt{ po_lock };
	IParticleCustom* V = smart_cast<IParticleCustom*>(renderable.visual); VERIFY(V);
	V->UpdateParent(m, zero_vel, TRUE);
	renderable.xform.set(m);
	UpdateSpatial();
}

void CParticlesObject::UpdateParent(const Fmatrix& m, const Fvector& vel)
{
	xrCriticalSection::raii mt{ po_lock };
	IParticleCustom* V = smart_cast<IParticleCustom*>(renderable.visual); VERIFY(V);
	V->UpdateParent(m, vel, FALSE);
	UpdateSpatial();
}

Fvector& CParticlesObject::Position()
{
	vis_data& vis = renderable.visual->getVisData();
	return vis.sphere.P;
}

float CParticlesObject::shedule_Scale()
{
	return Device.vCameraPosition.distance_to(Position()) / 200.f;
}

void CParticlesObject::renderable_Render()
{
	xrCriticalSection::raii mt{ po_lock };
	VERIFY(renderable.visual);
	u32 dt = Device.dwTimeGlobal - dwLastTime;
	if (dt)
	{
		IParticleCustom* V = smart_cast<IParticleCustom*>(renderable.visual); VERIFY(V);
		V->OnFrame(dt);
		dwLastTime = Device.dwTimeGlobal;
	}

	::Render->set_Transform(&renderable.xform);
	::Render->add_Visual(renderable.visual);
}

bool CParticlesObject::IsAutoRemove()
{
	if (m_bAutoRemove) return true;
	else return false;
}
void CParticlesObject::SetAutoRemove(bool auto_remove)
{
	VERIFY(m_bStopping || !IsLooped());
	m_bAutoRemove = auto_remove;
}

//играются ли партиклы, отличается от PSI_Alive, тем что после
//остановки Stop партиклы могут еще доигрывать анимацию IsPlaying = true
bool CParticlesObject::IsPlaying()
{
	IParticleCustom* V = smart_cast<IParticleCustom*>(renderable.visual);
	VERIFY(V);
	return !!V->IsPlaying();
}