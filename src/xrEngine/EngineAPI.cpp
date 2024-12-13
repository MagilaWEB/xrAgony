#include "stdafx.h"
#include "EngineAPI.h"
#include "XR_IOConsole.h"
#include "xrCore/ModuleLookup.hpp"
#include "xrCore/xr_token.h"

constexpr pcstr CHECK_FUNCTION = "CheckRendererSupport";
constexpr pcstr SETUP_FUNCTION = "SetupEnv";

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

void __cdecl dummy(void) {}

CEngineAPI::CEngineAPI()
{
	hGame = nullptr;
	hTuner = nullptr;
	pCreate = nullptr;
	pDestroy = nullptr;
	tune_enabled = false;
	tune_pause = dummy;
	tune_resume = dummy;
}
bool is_enough_address_space_available()
{
	SYSTEM_INFO system_info;
	GetSystemInfo(&system_info);
	return (*(u32*)&system_info.lpMaximumApplicationAddress) > 0x90000000;
}

void CEngineAPI::Initialize(void)
{
	hRender = XRay::LoadModule("xrRenderDX");

	m_setupSelectedRenderer = (SetupEnv)hRender->GetProcAddress(SETUP_FUNCTION);

	// ask current renderer to setup GEnv
	R_ASSERT2(m_setupSelectedRenderer, "Can't setup renderer");
	m_setupSelectedRenderer();

	hGame = XRay::LoadModule("xrGame");
	R_ASSERT2(hGame->IsLoaded(), "Game DLL raised exception during loading or there is no game DLL at all");

	pCreate = (Factory_Create*)hGame->GetProcAddress("xrFactory_Create");
	R_ASSERT(pCreate);

	pDestroy = (Factory_Destroy*)hGame->GetProcAddress("xrFactory_Destroy");
	R_ASSERT(pDestroy);

	pCreateGamePersisten = (CreateGamePersistent*)hGame->GetProcAddress("xrCreateGamePersistent");
	R_ASSERT(pCreateGamePersisten);

	pDestroyGamePersistent = (DestroyGamePersistent*)hGame->GetProcAddress("xrDestroyGamePersistent");
	R_ASSERT(pDestroyGamePersistent);

	//////////////////////////////////////////////////////////////////////////
	// vTune
	tune_enabled = false;
	if (strstr(Core.Params, "-tune"))
	{
		hTuner = XRay::LoadModule("vTuneAPI");
		tune_pause = (VTPause*)hTuner->GetProcAddress("VTPause");
		tune_resume = (VTResume*)hTuner->GetProcAddress("VTResume");

		if (!tune_pause || !tune_resume)
		{
			Log("Can't initialize Intel vTune");
			tune_pause = dummy;
			tune_resume = dummy;
			return;
		}

		tune_enabled = true;
	}
}

void CEngineAPI::Destroy(void)
{
	hGame = nullptr;
	hTuner = nullptr;
	pCreate = nullptr;
	pDestroy = nullptr;
	Engine.Event._destroy();
	XRC.r_clear_compact();
	hRender->Close();
	::IDevice = nullptr;
	Log("Engine Destroy!");
}
