#include "stdafx.h"

#include "SoundRender_CoreA.h"

XRSOUND_API xr_token* snd_devices_token = nullptr;
XRSOUND_API u32 snd_device_id = u32(-1);

void ISoundManager::_create()
{
	SoundRenderA = new CSoundRender_CoreA();
	SoundRender = SoundRenderA;
	::Sound = SoundRender;
	SoundRender->bPresent = strstr(Core.Params, "-nosound") == nullptr;
	if (!SoundRender->bPresent)
		return;
	::Sound->_initialize();
}

void ISoundManager::_destroy()
{
	::Sound->_clear();
	xr_delete(SoundRender);
	::Sound = nullptr;
}
