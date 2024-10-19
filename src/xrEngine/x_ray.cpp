//-----------------------------------------------------------------------------
// File: x_ray.cpp
//
// Programmers:
// Oles - Oles Shishkovtsov
// AlexMX - Alexander Maksimchuk
//-----------------------------------------------------------------------------
#include "stdafx.h"
#include "IGame_Level.h"
#include "IGame_Persistent.h"

#include "XR_IOConsole.h"
#include "x_ray.h"
#include "std_classes.h"
#include "GameFont.h"
#include "xrCDB/ISpatial.h"
#include "xrSASH.h"
#include "xrServerEntities/smart_cast.h"
#include "xr_input.h"

//---------------------------------------------------------------------

ENGINE_API CApplication* pApp = nullptr;
extern CRenderDevice Device;

#ifdef MASTER
#define NO_MULTI_INSTANCES
#endif // #ifdef MASTER

//////////////////////////////////////////////////////////////////////////
LPCSTR _GetFontTexName(LPCSTR section)
{
	static const char* tex_names[] = { "texture800", "texture", "texture1600" };
	int def_idx = 1; // default 1024x768
	int idx = def_idx;

#if 0
	u32 w = Device.dwWidth;

	if (w <= 800) idx = 0;
	else if (w <= 1280)idx = 1;
	else idx = 2;
#else
	u32 h = Device.dwHeight;

	if (h <= 600)
		idx = 0;
	else if (h < 1024)
		idx = 1;
	else
		idx = 2;
#endif

	while (idx >= 0)
	{
		if (pSettings->line_exist(section, tex_names[idx]))
			return pSettings->r_string(section, tex_names[idx]);
		--idx;
	}
	return pSettings->r_string(section, tex_names[def_idx]);
}

CApplication::CApplication()
{
	loaded = false;

	max_load_stage = 0;

	// events
	eQuit = Engine.Event.Handler_Attach("KERNEL:quit", this);
	eStart = Engine.Event.Handler_Attach("KERNEL:start", this);
	//eStartLoad = Engine.Event.Handler_Attach("KERNEL:load", this);
	eDisconnect = Engine.Event.Handler_Attach("KERNEL:disconnect", this);
	eConsole = Engine.Event.Handler_Attach("KERNEL:console", this);
	eQuickLoad = Engine.Event.Handler_Attach("Game:QuickLoad", this);

	// levels
	Level_Current = u32(-1);
	Level_Scan();

	// Font
	pFontSystem = nullptr;

	// Register us
	Device.seqFrame.Add(this, REG_PRIORITY_HIGH + 1000);

	Console->Show();

	// App Title
	loadingScreen = nullptr;
	//SetLoadStateMax(load_stage_limit);
}

CApplication::~CApplication()
{
	Console->Hide();

	// font
	xr_delete(pFontSystem);

	Device.seqFrame.Remove(this);

	DestroyLoadingScreen();

	// events
	Engine.Event.Handler_Detach(eConsole, this);
	Engine.Event.Handler_Detach(eDisconnect, this);
	//Engine.Event.Handler_Detach(eStartLoad, this);
	Engine.Event.Handler_Detach(eStart, this);
	Engine.Event.Handler_Detach(eQuit, this);
	Engine.Event.Handler_Detach(eQuickLoad, this);
}

void CApplication::OnEvent(EVENT E, u64 P1, u64 P2)
{
	if (E == eQuit)
	{

		g_SASH.EndBenchmark();
		Device.EventMessage(WM_QUIT, NULL, NULL);

		for (u32 i = 0; i < Levels.size(); i++)
		{
			xr_free(Levels[i].folder);
			xr_free(Levels[i].name);
		}
		Levels.clear();
	}
	else if (E == eStart)
	{
		LPSTR op_server = LPSTR(P1);
		LPSTR op_client = LPSTR(P2);
		Level_Current = u32(-1);
		R_ASSERT(nullptr == g_pGameLevel);
		R_ASSERT(nullptr != g_pGamePersistent);
		Console->Execute("main_menu off");
		Console->Hide();
		//! this line is commented by Dima
		//! because I don't see any reason to reset device here
		//! Device.Reset (false);
		//-----------------------------------------------------------
		g_pGamePersistent->PreStart(op_server);
		//-----------------------------------------------------------
		g_pGameLevel = g_pGamePersistent->CreateLevel();
		R_ASSERT(g_pGameLevel);
		LoadBegin();
		g_pGamePersistent->Start(op_server);
		g_pGameLevel->net_Start(op_server, op_client);
		xr_free(op_server);
		xr_free(op_client);
	}
	else if (E == eDisconnect)
	{
		if (pInput != nullptr && TRUE == Engine.Event.Peek("KERNEL:quit"))
			pInput->ClipCursor(false);

		if (g_pGameLevel)
		{
			Console->Hide();
			g_pGameLevel->net_Stop();
			g_pGamePersistent->DestroyLevel(g_pGameLevel);
			Console->Show();

			if ((FALSE == Engine.Event.Peek("KERNEL:quit")) && (FALSE == Engine.Event.Peek("KERNEL:start")))
			{
				Console->Execute("main_menu off");
				Console->Execute("main_menu on");
			}
		}
		R_ASSERT(nullptr != g_pGamePersistent);
		g_pGamePersistent->Disconnect();
	}
	else if (E == eConsole)
	{
		LPSTR command = (LPSTR)P1;
		Console->ExecuteCommand(command, false);
		xr_free(command);
	}
}


void CApplication::CheckMaxLoad()
{
	if (!loaded)
		SetLoadStateMax(elementary_load_stage_limit);
	else if (!g_pGameLevel->bReady)
		SetLoadStateMax(reset_load_stage_limit);
	else
		SetLoadStateMax(restart_load_stage_limit);
}

void CApplication::LoadBegin()
{
	CheckMaxLoad();

	loaded = false;

	pFontSystem = new CGameFont("font_letterica");

	load_stage = 0;

	if(loadingScreen && !loadingScreen->IsVisibility())
		loadingScreen->ChangeVisibility(true);
}

void CApplication::LoadEnd()
{
	//Msg("* phase time: %d ms", phase_timer.GetElapsed_ms());
	//Msg("* phase cmem: %d K", Memory.mem_usage() / 1024);
	//Console->Execute("stat_memory");
#if Debug
	Msg("------ !load_stage %d", load_stage);
#endif
	load_stage = 0;
	loaded = true;

	//SetLoadStateMax(load_stage_limit);
}

void CApplication::SetLoadingScreen(std::function<void(ILoadingScreen*&)> func)
{
	if (!loadingScreen)
		func(loadingScreen);
}

void CApplication::DestroyLoadingScreen()
{
	xr_delete(loadingScreen);
}

bool CApplication::IsLoadingScreen()
{
	return loadingScreen != nullptr && loadingScreen->IsVisibility();
}

void CApplication::SetLoadStageTitle(pcstr ls_title)
{
	if (loadingScreen && ls_title)
		loadingScreen->SetStageTitle(ls_title);
	else
		loadingScreen->SetStageTitle(nullptr);

	++load_stage;

	Msg("`load_stage[%d], max_load_stage[%d] ", load_stage, max_load_stage);
	//Msg("- load_stage [%d]", load_stage);
}

// Sequential
void CApplication::OnFrame()
{
	Engine.Event.OnFrame();
	g_SpatialSpace->update();
	g_SpatialSpacePhysic->update();
	if (g_pGameLevel)
		g_pGameLevel->SoundEvent_Dispatch();
}

void CApplication::Level_Append(LPCSTR folder)
{
	string_path N1, N2, N3, N4;
	strconcat(sizeof(N1), N1, folder, "level");
	strconcat(sizeof(N2), N2, folder, "level.ltx");
	strconcat(sizeof(N3), N3, folder, "level.geom");
	strconcat(sizeof(N4), N4, folder, "level.cform");
	if (FS.exist("$game_levels$", N1) && FS.exist("$game_levels$", N2) && FS.exist("$game_levels$", N3) &&
		FS.exist("$game_levels$", N4))
	{
		sLevelInfo LI;
		LI.folder = xr_strdup(folder);
		LI.name = nullptr;
		Levels.push_back(LI);
	}
}

void CApplication::Level_Scan()
{
	for (u32 i = 0; i < Levels.size(); i++)
	{
		xr_free(Levels[i].folder);
		xr_free(Levels[i].name);
	}
	Levels.clear();

	xr_vector<char*>* folder = FS.file_list_open("$game_levels$", FS_ListFolders | FS_RootOnly);
	//. R_ASSERT (folder&&folder->size());

	for (u32 i = 0; i < folder->size(); ++i)
		Level_Append((*folder)[i]);

	FS.file_list_close(folder);
}

void gen_logo_name(string_path& dest, LPCSTR level_name, int num)
{
	strconcat(sizeof(dest), dest, "intro\\intro_", level_name);

	u32 len = xr_strlen(dest);
	if (dest[len - 1] == '\\')
		dest[len - 1] = 0;

	string16 buff;
	xr_strcat(dest, sizeof(dest), "_");
	xr_strcat(dest, sizeof(dest), xr_itoa(num + 1, buff, 10));
}

void CApplication::Level_Set(u32 L)
{
	if (L >= Levels.size())
		return;
	FS.get_path("$level$")->_set(Levels[L].folder);

	static string_path path;

	if (Level_Current != L)
	{
		path[0] = 0;

		Level_Current = L;

		int count = 0;
		while (true)
		{
			string_path temp2;
			gen_logo_name(path, Levels[L].folder, count);
			if (FS.exist(temp2, "$game_textures$", path, ".dds") || FS.exist(temp2, "$level$", path, ".dds"))
				count++;
			else
				break;
		}

		if (count)
		{
			int num = ::Random.randI(count);
			gen_logo_name(path, Levels[L].folder, num);
		}
	}

	if (path[0] && loadingScreen)
		loadingScreen->SetLevelLogo(path);
}

int CApplication::Level_ID(LPCSTR name, LPCSTR ver, bool bSet)
{
	int result = -1;
	auto it = FS.m_archives.begin();
	auto it_e = FS.m_archives.end();
	bool arch_res = false;

	for (; it != it_e; ++it)
	{
		CLocatorAPI::archive& A = *it;
		if (A.hSrcFile == nullptr)
		{
			LPCSTR ln = A.header->r_string("header", "level_name");
			LPCSTR lv = A.header->r_string("header", "level_ver");
			if (0 == xr_stricmp(ln, name) && 0 == xr_stricmp(lv, ver))
			{
				FS.LoadArchive(A);
				arch_res = true;
			}
		}
	}

	if (arch_res)
		Level_Scan();

	string256 buffer;
	strconcat(sizeof(buffer), buffer, name, "\\");
	for (u32 I = 0; I < Levels.size(); ++I)
	{
		if (0 == xr_stricmp(buffer, Levels[I].folder))
		{
			result = int(I);
			break;
		}
	}

	if (bSet && result != -1)
		Level_Set(result);

	if (arch_res)
		g_pGamePersistent->OnAssetsChanged();
	return result;
}

CInifile* CApplication::GetArchiveHeader(LPCSTR name, LPCSTR ver)
{
	auto it = FS.m_archives.begin();
	auto it_e = FS.m_archives.end();

	for (; it != it_e; ++it)
	{
		CLocatorAPI::archive& A = *it;

		LPCSTR ln = A.header->r_string("header", "level_name");
		LPCSTR lv = A.header->r_string("header", "level_ver");
		if (0 == xr_stricmp(ln, name) && 0 == xr_stricmp(lv, ver))
		{
			return A.header;
		}
	}
	return nullptr;
}

void CApplication::LoadAllArchives()
{
	if (FS.load_all_unloaded_archives())
	{
		Level_Scan();
		g_pGamePersistent->OnAssetsChanged();
	}
}

#pragma optimize("g", off)
void CApplication::load_draw_internal()
{
	if (loadingScreen)
		loadingScreen->Update(load_stage, max_load_stage);
	else
		::Render->ClearTarget();
}