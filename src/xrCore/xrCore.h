#pragma once

#include "xrCore_benchmark_macros.h"

#if !defined(_CPPUNWIND)
#error Please enable exceptions...
#endif

#ifndef _MT
#error Please enable multi-threaded library...
#endif

#if defined(MASTER_GOLD)
//  release master gold
#	if defined(__cpp_exceptions) && defined(XR_PLATFORM_WINDOWS)
#		error Please disable exceptions...
#	endif
#	define XRAY_EXCEPTIONS 0
#else
//  release, debug or mixed
#	if !defined(__cpp_exceptions)
#		error Please enable exceptions...
#	endif
#	define XRAY_EXCEPTIONS 1
#endif

#if !defined(DEBUG) && (defined(_DEBUG) || defined(MIXED))
#define DEBUG
#endif

// Warnings
#pragma warning(disable : 4100) // unreferenced formal parameter
#pragma warning(disable : 4127) // conditional expression is constant
#pragma warning(disable : 4201) // nonstandard extension used : nameless struct/union
#pragma warning(disable : 4251) // object needs DLL interface
#pragma warning(disable : 4345)
//#pragma warning (disable : 4530 ) // C++ exception handler used, but unwind semantics are not enabled

#ifdef XR_X64
#pragma warning(disable : 4512)
#endif

#pragma warning(disable : 4714) // __forceinline not inlined

#ifndef DEBUG
#pragma warning(disable : 4189) // local variable is initialized but not referenced
#endif // frequently in release code due to large amount of VERIFY

#pragma comment(lib, "luabind.lib")

// Our headers
#include "xrDebug.h"

#include "clsid.h"
//#include "Threading/Lock.hpp"
#include "xrMemory.h"

#include "_std_extensions.h"
#include "xrCommon/xr_smart_pointers.h"
#include "xrCommon/xr_vector.h"
#include "xrCommon/xr_set.h"
#include "xrsharedmem.h"
#include "xrstring.h"
#include "xr_resource.h"
#include "Compression/rt_compressor.h"
#include "xr_shared.h"
#include "string_concatenations.h"
#include "_flags.h"

// stl ext
struct XRCORE_API xr_rtoken
{
	shared_str name;
	int id;

	xr_rtoken(pcstr _nm, int _id)
	{
		name = _nm;
		id = _id;
	}

	void rename(pcstr _nm) { name = _nm; }
	bool equal(pcstr _nm) const { return (0 == xr_strcmp(*name, _nm)); }
};

#include "xr_shortcut.h"

using RStringVec = xr_vector<shared_str>;
using RStringSet = xr_set<shared_str>;
using RTokenVec = xr_vector<xr_rtoken>;

#include "FS.h"
#include "log.h"
#include "xr_trims.h"
#include "xr_ini.h"
#ifdef NO_FS_SCAN
#include "ELocatorAPI.h"
#else
#include "LocatorAPI.h"
#endif
#include "FileSystem.h"
#include "FTimer.h"
#include "fastdelegate.h"
#include "intrusive_ptr.h"

#include "net_utils.h"
#include "Threading/xrThread.h"

// destructor
template <class T>
class destructor
{
	T* ptr;

public:
	destructor(T* p) { ptr = p; }
	~destructor() { xr_delete(ptr); }
	T& operator()() { return *ptr; }
};

// ***** The Core definition *****
class XRCORE_API xrCore
{
	const char* buildDate = "";
	u32 buildId = 0;

public:
	string64 ApplicationName;
	string_path ApplicationPath;
	string_path WorkingPath;
	string64 UserName;
	string64 CompName;
	char* Params;
	DWORD dwFrame;
	bool PluginMode;

	Flags16 ParamFlags; //Alun: TODO: Add all params
	enum ParamFlag {
		verboselog = (1 << 0),
		fpslock60 = (1 << 1),
		fpslock120 = (1 << 2),
		fpslock144 = (1 << 3),
		fpslock240 = (1 << 4),
		nofpslock = (1 << 5),
		dbgbullet = (1 << 6),
		genbump = (1 << 7),
		dev = (1 << 8),
	};

	void Initialize(
		pcstr ApplicationName, LogCallback cb = nullptr, bool init_fs = true, pcstr fs_fname = nullptr, bool plugin = false);
	void initParamFlags();
	void _destroy();
	const char* GetBuildDate() const { return buildDate; }
	u32 GetBuildId() const { return buildId; }
	static constexpr pcstr GetBuildConfiguration();

private:
	void CalculateBuildId();
};

extern XRCORE_API xrCore Core;
