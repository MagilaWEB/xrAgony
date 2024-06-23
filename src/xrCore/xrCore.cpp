// xrCore.cpp : Defines the entry point for the DLL application.
//
#include "stdafx.h"
#pragma hdrstop

#include <mmsystem.h>
#include <objbase.h>

#include "xrCore.h"
#include "xrCore/_std_extensions.h"

#pragma comment(lib, "winmm.lib")

XRCORE_API xrCore Core;

namespace CPU
{
	extern void Detect();
};

static u32 init_counter = 0;
void xrCore::Initialize(pcstr _ApplicationName, LogCallback cb, bool init_fs, pcstr fs_fname, bool plugin)
{
	xrThread::init_main_thread();

	xr_strcpy(ApplicationName, _ApplicationName);

	if (0 == init_counter)
	{
		CalculateBuildId();
		PluginMode = plugin;

		Params = xr_strdup(GetCommandLine());

		CoInitializeEx(nullptr, COINIT_MULTITHREADED);

		initParamFlags();

		string_path fn, dr, di;

		// application path
		GetModuleFileName(GetModuleHandle("xrCore"), fn, sizeof(fn));

		_splitpath(fn, dr, di, nullptr, nullptr);
		strconcat(sizeof(ApplicationPath), ApplicationPath, dr, di);

		GetCurrentDirectory(sizeof(WorkingPath), WorkingPath);

		// User/Comp Name
		DWORD sz_user = sizeof(UserName);
		GetUserName(UserName, &sz_user);

		DWORD sz_comp = sizeof(CompName);
		GetComputerName(CompName, &sz_comp);

		// Mathematics & PSI detection
		CPU::Detect();

		Memory._initialize();

		InitLog();

		Msg("%s %s build %d, %s\n", "A.G.O.N.Y Engine", GetBuildConfiguration(), buildId, buildDate);
		Msg("command line %s\n", Params);
		_initialize_cpu();
		R_ASSERT(CPU::ID.hasSSE());
		// xrDebug::Initialize ();

		rtc_initialize();

		xr_FS = std::make_unique<CLocatorAPI>();

		xr_EFS = std::make_unique<EFS_Utils>();
		//. R_ASSERT (co_res==S_OK);
	}

	if (init_fs)
	{
		u32 flags = 0u;
		if (strstr(Params, "-build") != nullptr)
			flags |= CLocatorAPI::flBuildCopy;
		if (strstr(Params, "-ebuild") != nullptr)
			flags |= CLocatorAPI::flBuildCopy | CLocatorAPI::flEBuildCopy;

#ifdef DEBUG
		if (strstr(Params, "-cache"))
			flags |= CLocatorAPI::flCacheFiles;
		else
			flags &= ~CLocatorAPI::flCacheFiles;
#endif // DEBUG

		flags |= CLocatorAPI::flScanAppRoot;

#ifndef ELocatorAPIH
		if (strstr(Params, "-file_activity") != nullptr)
			flags |= CLocatorAPI::flDumpFileActivity;
#endif
		FS._initialize(flags, nullptr, fs_fname);
		EFS._initialize();
#ifdef DEBUG
		Msg("Process heap 0x%08x", GetProcessHeap());
#endif // DEBUG
	}
	SetLogCB(cb);
	init_counter++;
}

void xrCore::initParamFlags()
{
	if (strstr(Params, "-verboselog"))
		ParamFlags.set(ParamFlag::verboselog, true);

	if (strstr(Params, "-dbgbullet"))
		ParamFlags.set(ParamFlag::dbgbullet, true);

	if (strstr(Params, "-dev"))
		ParamFlags.set(ParamFlag::dev, true);

	if (strstr(Params, "-genbump"))
		ParamFlags.set(ParamFlag::genbump, true);
}

#include "compression_ppmd_stream.h"
extern compression::ppmd::stream* trained_model;
void xrCore::_destroy()
{
	--init_counter;
	if (0 == init_counter)
	{
		FS._destroy();
		EFS._destroy();
		xr_FS.reset();
		xr_EFS.reset();

		if (trained_model)
		{
			void* buffer = trained_model->buffer();
			xr_free(buffer);
			xr_delete(trained_model);
		}

		xr_free(Params);
		Memory._destroy();
	}
}

constexpr pcstr xrCore::GetBuildConfiguration()
{
#ifdef NDEBUG
#ifdef XR_X64
	return "Rx64";
#else
	return "Rx86";
#endif
#elif defined(MIXED)
#ifdef XR_X64
	return "Mx64";
#else
	return "Mx86";
#endif
#else
#ifdef XR_X64
	return "Dx64";
#else
	return "Dx86";
#endif
#endif
}

void xrCore::CalculateBuildId()
{
	const int startDay = 31;
	const int startMonth = 1;
	const int startYear = 1999;
	const char* monthId[12] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };
	const int daysInMonth[12] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
	buildDate = __DATE__;
	int days;
	int months = 0;
	int years;
	string16 month;
	string256 buffer;
	xr_strcpy(buffer, buildDate);
	sscanf(buffer, "%s %d %d", month, &days, &years);
	for (int i = 0; i < 12; i++)
	{
		if (xr_stricmp(monthId[i], month))
			continue;
		months = i;
		break;
	}
	buildId = (years - startYear) * 365 + days - startDay;
	for (int i = 0; i < months; i++)
		buildId += daysInMonth[i];
	for (int i = 0; i < startMonth - 1; i++)
		buildId -= daysInMonth[i];
}

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD ul_reason_for_call, LPVOID lpvReserved)
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
	{
		_clear87();
#ifdef _M_IX86
		_control87(_PC_53, MCW_PC);
#endif
		_control87(_RC_CHOP, MCW_RC);
		_control87(_RC_NEAR, MCW_RC);
		_control87(_MCW_EM, MCW_EM);
	}
	//. LogFile.reserve (256);
	break;
	case DLL_THREAD_ATTACH:
		// if (!strstr(GetCommandLine(), "-editor"))
		//	 CoInitializeEx(NULL, COINIT_MULTITHREADED);
		timeBeginPeriod(1);
		break;
	case DLL_THREAD_DETACH:
		break;
	case DLL_PROCESS_DETACH:
#ifdef USE_MEMORY_MONITOR
		memory_monitor::flush_each_time(true);
#endif // USE_MEMORY_MONITOR
		break;
	}
	return TRUE;
}