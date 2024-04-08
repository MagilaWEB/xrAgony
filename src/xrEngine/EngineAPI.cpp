#include "stdafx.h"
#include "EngineAPI.h"
#include "XR_IOConsole.h"
#include "xrCore/ModuleLookup.hpp"
#include "xrCore/xr_token.h"

extern xr_vector<xr_token> vid_quality_token;

constexpr pcstr CHECK_FUNCTION = "CheckRendererSupport";
constexpr pcstr SETUP_FUNCTION = "SetupEnv";

constexpr pcstr DX9_LIBRARY = "xrRender_DX9";
constexpr pcstr DX11_LIBRARY = "xrRender_DX11";

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
	GEnv.CurrentRenderer = -1;

	const auto select = [&](pcstr library, u32 selected, int index, u32 fallback = 0)
	{
		if (psDeviceFlags.test(selected))
		{
			if (m_renderers[library]->IsLoaded())
			{
				GEnv.CurrentRenderer = index;
				m_setupSelectedRenderer = (SetupEnv)m_renderers[library]->GetProcAddress(SETUP_FUNCTION);
			}
			else // Selected is unavailable
			{
				psDeviceFlags.set(selected, false);
				if (fallback > 0) // try to use another
					psDeviceFlags.set(fallback, true);
			}
		}
	};

	select(DX11_LIBRARY, rsDX11, 1, rsDX9);
	select(DX9_LIBRARY, rsDX9, 0);
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
	m_renderers.clear();
	pCreate = nullptr;
	pDestroy = nullptr;
	Engine.Event._destroy();
	XRC.r_clear_compact();
}

void CEngineAPI::CloseUnusedLibraries()
{
	// Now unload unused renderers
	if (GEnv.CurrentRenderer != 1)
		m_renderers[DX11_LIBRARY]->Close();
	else
		m_renderers[DX9_LIBRARY]->Close();
}

void CEngineAPI::CreateRendererList()
{
	if (!vid_quality_token.empty())
		return;

	m_renderers[DX9_LIBRARY] = XRay::LoadModule(DX9_LIBRARY);
	m_renderers[DX11_LIBRARY] = XRay::LoadModule(DX11_LIBRARY);

	auto& modes = vid_quality_token;

	const auto checkRenderer = [&](pcstr library, pcstr mode, int index)
	{
		if (m_renderers[library]->IsLoaded())
		{
			// Load SupportCheck, SetupEnv and GetModeName functions from DLL
			const auto checkSupport = (SupportCheck)m_renderers[library]->GetProcAddress(CHECK_FUNCTION);

			// Test availability
			if (checkSupport && checkSupport())
				modes.emplace_back(mode, index);
			else // Close the handle if test is failed
				m_renderers[library]->Close();
		}
	};

	checkRenderer(DX9_LIBRARY, RENDERER_DX9Basic, 0);

	if (m_renderers[DX9_LIBRARY]->IsLoaded())
		modes.emplace_back(RENDERER_DX9Normal, 1);

	checkRenderer(DX9_LIBRARY, RENDERER_DX9Enhanced, 2);

	checkRenderer(DX11_LIBRARY, RENDERER_dx11, 3);

	modes.emplace_back(xr_token(nullptr, -1));

	Msg("Available render modes[%d]:", modes.size());
	for (const auto& mode : modes)
		if (mode.name)
			Log(mode.name);
}
