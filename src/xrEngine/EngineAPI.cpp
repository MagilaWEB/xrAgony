#include "stdafx.h"
#include "EngineAPI.h"
#include "XR_IOConsole.h"
#include "xrCore/ModuleLookup.hpp"
#include "xrCore/xr_token.h"

extern xr_vector<xr_token> vid_quality_token;

constexpr pcstr CHECK_FUNCTION = "CheckRendererSupport";
constexpr pcstr SETUP_FUNCTION = "SetupEnv";

constexpr pcstr RENDERER_DX9Basic = "renderer_DX9Basic";
constexpr pcstr RENDERER_DX9Normal = "renderer_DX9Normal";
constexpr pcstr RENDERER_DX9Enhanced = "renderer_DX9Enhanced";
constexpr pcstr RENDERER_dx11 = "renderer_DX11";

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

CEngineAPI::~CEngineAPI() { vid_quality_token.clear(); }

bool is_enough_address_space_available()
{
	SYSTEM_INFO system_info;
	GetSystemInfo(&system_info);
	return (*(u32*)&system_info.lpMaximumApplicationAddress) > 0x90000000;
}

void CEngineAPI::SelectRenderer()
{
	::CurrentRenderer = -1;

	const auto select = [&](int index, u32 selected,  u32 fallback = 0)
	{
		if (psDeviceFlags.test(selected))
		{
			if (m_renderers[index]->IsLoaded())
			{
				::CurrentRenderer = index;
				m_setupSelectedRenderer = (SetupEnv)m_renderers[index]->GetProcAddress(SETUP_FUNCTION);
			}
			else // Selected is unavailable
			{
				psDeviceFlags.set(selected, false);
				if (fallback > 0) // try to use another
					psDeviceFlags.set(fallback, true);
			}
		}
	};

	select(1, rsDX11, rsDX9);
	select(0, rsDX9);
}

void CEngineAPI::InitializeRenderers()
{
	SelectRenderer();

	if (m_setupSelectedRenderer == nullptr && vid_quality_token[0].id != -1)
	{
		// if engine failed to load renderer
		// but there is at least one available
		// then try again
		string32 buf;
		xr_sprintf(buf, "renderer %s", vid_quality_token[0].name);
		Console->Execute(buf);

		// Second attempt
		SelectRenderer();
	}

	// ask current renderer to setup GEnv
	R_ASSERT2(m_setupSelectedRenderer, "Can't setup renderer");
	m_setupSelectedRenderer();

	Log("Selected renderer:", Console->GetString("renderer"));
}

void CEngineAPI::Initialize(void)
{
	InitializeRenderers();

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

	// Close only AFTER other libraries are loaded!!
	CloseUnusedLibraries();
}

void CEngineAPI::Destroy(void)
{
	hGame = nullptr;
	hTuner = nullptr;
	pCreate = nullptr;
	pDestroy = nullptr;
	Engine.Event._destroy();
	XRC.r_clear_compact();
	Log("Engine Destroy!");
}

void CEngineAPI::CloseUnusedLibraries()
{
	// Now unload unused renderers
	for (u32 it = 0; it < m_renderers.size(); it++)
		if (::CurrentRenderer != it)
			m_renderers[it]->Close();
}

void CEngineAPI::CreateRendererList()
{
	if (!vid_quality_token.empty())
		return;

	m_renderers.push_back(XRay::LoadModule("xrRender_DX9"));
	m_renderers.push_back(XRay::LoadModule("xrRender_DX11"));

	auto& modes = vid_quality_token;

	const auto checkRenderer = [&](int index, pcstr mode, int r_index)
	{
		if (m_renderers[index]->IsLoaded())
		{
			// Load SupportCheck, SetupEnv and GetModeName functions from DLL
			const auto checkSupport = (SupportCheck)m_renderers[index]->GetProcAddress(CHECK_FUNCTION);

			// Test availability
			if (checkSupport && checkSupport())
				modes.emplace_back(mode, r_index);
			else // Close the handle if test is failed
				m_renderers[index]->Close();
		}
	};

	checkRenderer(0, RENDERER_DX9Basic, 0);

	if (m_renderers[0]->IsLoaded())
		modes.emplace_back(RENDERER_DX9Normal, 1);

	checkRenderer(0, RENDERER_DX9Enhanced, 2);

	checkRenderer(1, RENDERER_dx11, 3);

	modes.emplace_back(xr_token(nullptr, -1));

	Msg("Available render modes[%d]:", modes.size());
	for (const auto& mode : modes)
		if (mode.name)
			Log(mode.name);
}
