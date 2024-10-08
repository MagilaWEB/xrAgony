#include "stdafx.h"
#include "xrSheduler.h"
#include "xr_object.h"
// XXX: rename this file to ScheduledBase.cpp
ScheduledBase::ScheduledBase()
{
	shedule.t_min = 20;
	shedule.t_max = 1000;
	shedule.b_locked = FALSE;
}

ScheduledBase::~ScheduledBase()
{
#ifdef DEBUG
	VERIFY2(!Engine.Sheduler.Registered(this), make_string("0x%08x : %s", this, *shedule_Name()));
#endif

// XXX: WTF???
// sad, but true
// we need this to become MASTER
#ifndef DEBUG
	Engine.Sheduler.Unregister(this);
#endif // DEBUG
}

void ScheduledBase::shedule_register() { Engine.Sheduler.Register(this); }
void ScheduledBase::shedule_unregister() { Engine.Sheduler.Unregister(this); }
void ScheduledBase::shedule_Update(u32 dt)
{

}
