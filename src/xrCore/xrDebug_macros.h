#pragma once
#ifndef xrDebug_macrosH
#define xrDebug_macrosH
#include "xrDebug.h"

#define DEBUG_INFO {__FILE__, __LINE__, __FUNCTION__}
#define CHECK_OR_EXIT(expr, message)\
	do\
	{\
		if (!(expr))\
			xrDebug::DoExit(message);\
	} while (false)
#define R_ASSERT(expr)\
	do\
	{\
		static bool ignoreAlways = false;\
		if (!ignoreAlways && !(expr))\
			xrDebug::Fail(ignoreAlways, DEBUG_INFO, #expr);\
	} while (false)
#define R_ASSERT2(expr, desc)\
	do\
	{\
		static bool ignoreAlways = false;\
		if (!ignoreAlways && !(expr))\
			xrDebug::Fail(ignoreAlways, DEBUG_INFO, #expr, desc);\
	} while (false)
#define R_ASSERT3(expr, desc, arg1)\
	do\
	{\
		static bool ignoreAlways = false;\
		if (!ignoreAlways && !(expr))\
			xrDebug::Fail(ignoreAlways, DEBUG_INFO, #expr, desc, arg1);\
	} while (false)
#define R_ASSERT4(expr, desc, arg1, arg2)\
	do\
	{\
		static bool ignoreAlways = false;\
		if (!ignoreAlways && !(expr))\
			xrDebug::Fail(ignoreAlways, DEBUG_INFO, #expr, desc, arg1, arg2);\
	} while (false)
#define R_CHK(expr)\
	do\
	{\
		static bool ignoreAlways = false;\
		HRESULT hr = expr;\
		if (!ignoreAlways && FAILED(hr))\
			xrDebug::Fail(ignoreAlways, DEBUG_INFO, #expr, hr);\
	} while (false)
#define R_CHK2(expr, arg1)\
	do\
	{\
		static bool ignoreAlways = false;\
		HRESULT hr = expr;\
		if (!ignoreAlways && FAILED(hr))\
			xrDebug::Fail(ignoreAlways, DEBUG_INFO, #expr, hr, arg1);\
	} while (false)
#define FATAL(...) xrDebug::Fatal(DEBUG_INFO,__VA_ARGS__)
#define FATAL_F(format, ...) xrDebug::Fatal(DEBUG_INFO, format, __VA_ARGS__)

#ifdef VERIFY
#undef VERIFY
#endif

#ifdef DEBUG
#define NODEFAULT FATAL("nodefault reached")
#define VERIFY(expr)\
	do\
	{\
		if (!(expr))\
			xrDebug::SoftFail(DEBUG_INFO, #expr);\
	} while (false);
#define VERIFY2(expr, desc)\
	do\
	{\
		if (!(expr))\
			xrDebug::SoftFail(DEBUG_INFO, #expr, desc);\
	} while (false);
#define VERIFY3(expr, desc, arg1)\
	do\
	{\
		if (!(expr))\
			xrDebug::SoftFail(DEBUG_INFO, #expr, desc, arg1);\
	} while (false);
#define VERIFY4(expr, desc, arg1, arg2)\
	do\
	{\
		if (!(expr))\
			xrDebug::SoftFail(DEBUG_INFO, #expr, desc, arg1, arg2);\
	} while (false);
#else // DEBUG
#ifdef __BORLANDC__
#define NODEFAULT
#else
#define NODEFAULT XR_ASSUME(0)
#endif
#define VERIFY(expr)
#define VERIFY2(expr, desc)
#define VERIFY3(expr, desc, arg1)
#define VERIFY4(expr, desc, arg1, arg2)
#endif // DEBUG

#define CHK_DX(expr) expr
//---------------------------------------------------------------------------------------------
// FIXMEs / TODOs / NOTE macros
//---------------------------------------------------------------------------------------------
#define _QUOTE(x) #x
#define QUOTE(x) _QUOTE(x)
#define __FILE__LINE__ __FILE__ "(" QUOTE(__LINE__) ") : "

#define NOTE(x) message(x)
#define FILE_LINE message(__FILE__LINE__)

#define TODO(x) message(__FILE__LINE__"\n"\
	" ------------------------------------------------\n"\
	"| TODO : " #x "\n"\
	" -------------------------------------------------\n")
#define FIXME(x) message(__FILE__LINE__"\n"\
	" ------------------------------------------------\n"\
	"| FIXME : " #x "\n"\
	" -------------------------------------------------\n")
#define todo(x) message(__FILE__LINE__" TODO : " #x "\n") 
#define fixme(x) message(__FILE__LINE__" FIXME: " #x "\n") 

#endif // xrDebug_macrosH