#include "stdafx.h"
#include "main.h"
#include "resource.h"
#include "StickyKeyFilter.hpp"
#include <locale.h>
#include <process.h>
#include "IGame_Persistent.h"
#include "LightAnimLibrary.h"
#include "splash.h"
#include "std_classes.h"
#include "Text_Console.h"
#include "x_ray.h"
#include "xr_input.h"
#include "xr_ioc_cmd.h"
#include "xrCDB/ISpatial.h"
#include "xrSASH.h"

#ifdef MASTER_GOLD
#define NO_MULTI_INSTANCES
#endif

// global variables
ENGINE_API CInifile* pGameIni = nullptr;
ENGINE_API bool g_bBenchmark = false;
string512 g_sBenchmarkName;
ENGINE_API string512 g_sLaunchOnExit_params;
ENGINE_API string512 g_sLaunchOnExit_app;
ENGINE_API string_path g_sLaunchWorkingFolder;

namespace
{
	bool CheckBenchmark();
	void RunBenchmark(pcstr name);
} // namespace

ENGINE_API void InitEngine()
{
	Engine.Initialize();
	Device.Initialize();
}

ENGINE_API void InitSettings()
{
	string_path fname;
	FS.update_path(fname, "$game_config$", "system.ltx");
#ifdef DEBUG
	Msg("Updated path to system.ltx is %s", fname);
#endif
	pSettings = new CInifile(fname, TRUE);
	CHECK_OR_EXIT(pSettings->section_count(),
		make_string("Cannot find file %s.\nReinstalling application may fix this problem.", fname));

	FS.update_path(fname, "$game_config$", "game.ltx");
	pGameIni = new CInifile(fname, TRUE);
	CHECK_OR_EXIT(pGameIni->section_count(),
		make_string("Cannot find file %s.\nReinstalling application may fix this problem.", fname));
}

ENGINE_API void InitConsole()
{
	Console = new CConsole();

	Console->Initialize();
	xr_strcpy(Console->ConfigFile, "user.ltx");
	if (strstr(Core.Params, "-ltx "))
	{
		string64 c_name;
		sscanf(strstr(Core.Params, "-ltx ") + strlen("-ltx "), "%[^ ] ", c_name);
		xr_strcpy(Console->ConfigFile, c_name);
	}
}

ENGINE_API void InitInput()
{
	bool captureInput = !strstr(Core.Params, "-i");
	pInput = new CInput(captureInput);
}

ENGINE_API void destroyInput() { xr_delete(pInput); }

ENGINE_API void destroySettings()
{
	auto s = const_cast<CInifile**>(&pSettings);
	xr_delete(*s);
	xr_delete(pGameIni);
}

ENGINE_API void destroyConsole()
{
	Console->Execute("cfg_save");
	Console->Destroy();
	xr_delete(Console);
}

ENGINE_API void destroyEngine()
{
	Device.Destroy();
	Engine.Destroy();
}

void execUserScript()
{
	Console->Execute("default_controls");
	Console->ExecuteScript(Console->ConfigFile);
}

ENGINE_API void Startup()
{
	execUserScript();
	ISoundManager::_initDevice();
	
	// Initialize APP
	Device.Create();
	LALib.OnCreate();
	pApp = new CApplication();
	g_pGamePersistent = Engine.External.pCreateGamePersisten();
	R_ASSERT(g_pGamePersistent);
	g_SpatialSpace = new ISpatial_DB("Spatial obj");
	g_SpatialSpacePhysic = new ISpatial_DB("Spatial phys");

	// Main cycle
	Device.Run();

	xrDebug::DeinitializeSymbolEngine();

	// Destroy APP
	xr_delete(g_SpatialSpacePhysic);
	xr_delete(g_SpatialSpace);
	Engine.External.pDestroyGamePersistent(g_pGamePersistent);
	xr_delete(pApp);
	Engine.Event.Dump();
	// Destroying
	destroyInput();
	if (!g_bBenchmark && !g_SASH.IsRunning())
		destroySettings();
	LALib.OnDestroy();
	if (!g_bBenchmark && !g_SASH.IsRunning())
		destroyConsole();
	else
		Console->Destroy();
	destroyEngine();
	ISoundManager::_destroy();
}

ENGINE_API int RunApplication()
{
	R_ASSERT2(Core.Params, "Core must be initialized");

#ifdef NO_MULTI_INSTANCES
	CreateMutex(nullptr, TRUE, "Local\\stalker_agony");
	if (GetLastError() == ERROR_ALREADY_EXISTS)
	{
		splash::hide();
		MessageBox(nullptr, "The game has already been launched!", nullptr, MB_ICONERROR | MB_OK);
		return 2;
	}
#endif
	* g_sLaunchOnExit_app = 0;
	*g_sLaunchOnExit_params = 0;

	InitSettings();
	// Adjust player & computer name for Asian
	if (pSettings->line_exist("string_table", "no_native_input"))
	{
		xr_strcpy(Core.UserName, sizeof(Core.UserName), "Player");
		xr_strcpy(Core.CompName, sizeof(Core.CompName), "Computer");
	}

	InitEngine();
	InitInput();
	ISoundManager::_create();
	InitConsole();
	Engine.External.CreateRendererList();

	if (CheckBenchmark())
		return 0;

	CCC_LoadCFG_custom cmd("renderer ");
	cmd.Execute(Console->ConfigFile);

	Engine.External.Initialize();
	Startup();
	// check for need to execute something external
	if (xr_strlen(g_sLaunchOnExit_app))
	{
		// CreateProcess need to return results to next two structures
		STARTUPINFO si = {};
		si.cb = sizeof(si);
		PROCESS_INFORMATION pi = {};
		// We use CreateProcess to setup working folder
		pcstr tempDir = xr_strlen(g_sLaunchWorkingFolder) ? g_sLaunchWorkingFolder : nullptr;
		CreateProcess(
			g_sLaunchOnExit_app, g_sLaunchOnExit_params, nullptr, nullptr, FALSE, 0, nullptr, tempDir, &si, &pi);
	}
	return 0;
	}

namespace
{
	bool CheckBenchmark()
	{
		pcstr benchName = "-batch_benchmark ";
		if (strstr(Core.Params, benchName))
		{
			const u32 sz = xr_strlen(benchName);
			string64 benchmarkName;
			sscanf(strstr(Core.Params, benchName) + sz, "%[^ ] ", benchmarkName);
			RunBenchmark(benchmarkName);
			return true;
		}

		pcstr sashName = "-openautomate ";
		if (strstr(Core.Params, sashName))
		{
			const u32 sz = xr_strlen(sashName);
			string512 sashArg;
			sscanf(strstr(Core.Params, sashName) + sz, "%[^ ] ", sashArg);
			g_SASH.Init(sashArg);
			g_SASH.MainLoop();
			return true;
		}

		return false;
	}
	void RunBenchmark(pcstr name)
	{
		g_bBenchmark = true;
		string_path cfgPath;
		FS.update_path(cfgPath, "$app_data_root$", name);
		CInifile ini(cfgPath);
		const u32 benchmarkCount = ini.line_count("benchmark");
		for (u32 i = 0; i < benchmarkCount; i++)
		{
			LPCSTR benchmarkName, t;
			ini.r_line("benchmark", i, &benchmarkName, &t);
			xr_strcpy(g_sBenchmarkName, benchmarkName);
			shared_str benchmarkCommand = ini.r_string_wb("benchmark", benchmarkName);
			u32 cmdSize = benchmarkCommand.size() + 1;
			Core.Params = (char*)xr_realloc(Core.Params, cmdSize);
			xr_strcpy(Core.Params, cmdSize, benchmarkCommand.c_str());
			xr_strlwr(Core.Params);
			InitInput();
			if (i)
				InitEngine();
			Engine.External.Initialize();
			xr_strcpy(Console->ConfigFile, "user.ltx");
			if (strstr(Core.Params, "-ltx "))
			{
				string64 cfgName;
				sscanf(strstr(Core.Params, "-ltx ") + strlen("-ltx "), "%[^ ] ", cfgName);
				xr_strcpy(Console->ConfigFile, cfgName);
			}
			Startup();
		}
	}
} // namespace

int StackoverflowFilter(const int exceptionCode)
{
	if (exceptionCode == EXCEPTION_STACK_OVERFLOW)
		return EXCEPTION_EXECUTE_HANDLER;
	return EXCEPTION_CONTINUE_SEARCH;
}

int APIENTRY WinMain(HINSTANCE inst, HINSTANCE prevInst, char* commandLine, int cmdShow)
{
	int result = 0;

	auto entry_point = [commandLine]()
	{
		if (strstr(commandLine, "-nosplash") == nullptr)
		{
#ifndef DEBUG
			const bool topmost = strstr(commandLine, "-splashnotop") == nullptr ? true : false;
#else
			constexpr bool topmost = false;
#endif
			splash::show(topmost);
		}

		xrDebug::Initialize();

		StickyKeyFilter filter;
		filter.initialize();

		pcstr fsltx = "-fsltx ";
		string_path fsgame = "";
		if (strstr(commandLine, fsltx))
		{
			const u32 sz = xr_strlen(fsltx);
			sscanf(strstr(commandLine, fsltx) + sz, "%[^ ] ", fsgame);
		}
		Core.Initialize("xrAgony", nullptr, true, *fsgame ? fsgame : nullptr);

		auto result = RunApplication();

		Core._destroy();

		return result;
	};
	// BugTrap can't handle stack overflow exception, so handle it here
	__try
	{
		result = entry_point();
	}
	__except (StackoverflowFilter(GetExceptionCode()))
	{
		_resetstkoflw();
		FATAL("stack overflow");
	}
	return result;
}