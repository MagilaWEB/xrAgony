#pragma once
#include "xrCore/_types.h"
#include "xrCommon/xr_string.h"
#include "xrCommon/xr_vector.h"

#include <string>

#pragma warning(push)
#pragma warning(disable : 4091) /// 'typedef ': ignored on left of '' when no variable is declared
#include <DbgHelp.h>
#pragma warning(pop)

class ErrorLocation
{
public:
	const char* File = nullptr;
	int Line = -1;
	const char* Function = nullptr;

	ErrorLocation(const char* file, int line, const char* function)
	{
		File = file;
		Line = line;
		Function = function;
	}

	ErrorLocation& operator=(const ErrorLocation& rhs)
	{
		File = rhs.File;
		Line = rhs.Line;
		Function = rhs.Function;
		return *this;
	}
};

class XRCORE_API xrDebug
{
public:
	using OutOfMemoryCallbackFunc = void(*)();
	using CrashHandler = void(*)();
	using DialogHandler = void(*)(bool);
	using UnhandledExceptionFilter = LONG(WINAPI*)(EXCEPTION_POINTERS* exPtrs);

private:
	static UnhandledExceptionFilter PrevFilter;
	static OutOfMemoryCallbackFunc OutOfMemoryCallback;
	static CrashHandler OnCrash;
	static DialogHandler OnDialog;
	static string_path BugReportFile;
	static bool ErrorAfterDialog;

public:
	xrDebug() = delete;
	static void Initialize();
	static void OnThreadSpawn();
	static OutOfMemoryCallbackFunc GetOutOfMemoryCallback() { return OutOfMemoryCallback; }
	static void SetOutOfMemoryCallback(OutOfMemoryCallbackFunc cb) { OutOfMemoryCallback = cb; }
	static CrashHandler GetCrashHandler() { return OnCrash; }
	static void SetCrashHandler(CrashHandler handler) { OnCrash = handler; }
	static DialogHandler GetDialogHandler() { return OnDialog; }
	static void SetDialogHandler(DialogHandler handler) { OnDialog = handler; }
	static const char* ErrorToString(long code);
	static void ShowMSGboxAboutError(LPCSTR title, LPCSTR description);
	static void SetBugReportFile(const char* fileName);
	static void GatherInfo(char* assertionInfo, const ErrorLocation& loc, const char* expr, const char* desc,
		const char* arg1 = nullptr, const char* arg2 = nullptr);
	static void Fatal(const ErrorLocation& loc, const char* format, ...);
	static void Fail(bool& ignoreAlways, const ErrorLocation& loc, const char* expr, long hresult,
		const char* arg1 = nullptr, const char* arg2 = nullptr);
	static void Fail(bool& ignoreAlways, const ErrorLocation& loc, const char* expr,
		const char* desc = "assertion failed", const char* arg1 = nullptr, const char* arg2 = nullptr);
	static void Fail(bool& ignoreAlways, const ErrorLocation& loc, const char* expr, const std::string& desc,
		const char* arg1 = nullptr, const char* arg2 = nullptr);
	static void DoExit(const std::string& message);

	static void SoftFail(const ErrorLocation& loc, const char* expr, const char* desc = nullptr,
		const char* arg1 = nullptr, const char* arg2 = nullptr);
	static void SoftFail(const ErrorLocation& loc, const char* expr, const std::string& desc, const char* arg1 = nullptr,
		const char* arg2 = nullptr);

	static void LogStackTrace(const char* header);
	static xr_vector<xr_string> BuildStackTrace(u16 maxFramesCount = 512);

	static void DeinitializeSymbolEngine();
private:
	static bool symEngineInitialized;
	static void FormatLastError(char* buffer, const size_t& bufferSize);
	static LONG WINAPI UnhandledFilter(EXCEPTION_POINTERS* exPtrs);
	static xr_vector<xr_string> BuildStackTrace(PCONTEXT threadCtx, u16 maxFramesCount);
	static bool GetNextStackFrameString(LPSTACKFRAME stackFrame, PCONTEXT threadCtx, xr_string& frameStr);
	static bool InitializeSymbolEngine();
};

#include "xrDebug_macros.h"
