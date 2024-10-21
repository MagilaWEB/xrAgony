#pragma once
#ifndef xrDebug_macrosH
#define xrDebug_macrosH
#include "xrDebug.h"

#define DEBUG_INFO {__FILE__, __LINE__, __FUNCTION__}
#define CHECK_OR_EXIT(expr, message)\
	{\
		if (!(expr))\
			xrDebug::DoExit(message);\
	}
#define R_ASSERT(expr)\
	{\
		static bool ignoreAlways = false;\
		if (!ignoreAlways && !(expr))\
			xrDebug::Fail(ignoreAlways, DEBUG_INFO, #expr);\
	}
#define R_ASSERT2(expr, desc)\
	{\
		static bool ignoreAlways = false;\
		if (!ignoreAlways && !(expr))\
			xrDebug::Fail(ignoreAlways, DEBUG_INFO, #expr, desc);\
	}
#define R_ASSERT3(expr, desc, arg1)\
	{\
		static bool ignoreAlways = false;\
		if (!ignoreAlways && !(expr))\
			xrDebug::Fail(ignoreAlways, DEBUG_INFO, #expr, desc, arg1);\
	}
#define R_ASSERT4(expr, desc, arg1, arg2)\
	{\
		static bool ignoreAlways = false;\
		if (!ignoreAlways && !(expr))\
			xrDebug::Fail(ignoreAlways, DEBUG_INFO, #expr, desc, arg1, arg2);\
	}
#define R_CHK(expr)\
	{\
		static bool ignoreAlways = false;\
		HRESULT hr = expr;\
		if (!ignoreAlways && FAILED(hr))\
			xrDebug::Fail(ignoreAlways, DEBUG_INFO, #expr, hr);\
	}
#define R_CHK2(expr, arg1)\
	{\
		static bool ignoreAlways = false;\
		HRESULT hr = expr;\
		if (!ignoreAlways && FAILED(hr))\
			xrDebug::Fail(ignoreAlways, DEBUG_INFO, #expr, hr, arg1);\
	}
#define FATAL(...) xrDebug::Fatal(DEBUG_INFO,__VA_ARGS__)
#define FATAL_F(format, ...) xrDebug::Fatal(DEBUG_INFO, format, __VA_ARGS__)

#ifdef VERIFY
#undef VERIFY
#endif

#ifdef DEBUG
#define NODEFAULT FATAL("nodefault reached")
#else // DEBUG
#ifdef __BORLANDC__
#define NODEFAULT
#else
#define NODEFAULT XR_ASSUME(0)
#endif
#endif // DEBUG

#define VERIFY(expr)\
	{\
		if (!(expr))\
			xrDebug::SoftFail(DEBUG_INFO, #expr);\
	}
#define VERIFY2(expr, desc)\
	{\
		if (!(expr))\
			xrDebug::SoftFail(DEBUG_INFO, #expr, desc);\
	}
#define VERIFY3(expr, desc, arg1)\
	{\
		if (!(expr))\
			xrDebug::SoftFail(DEBUG_INFO, #expr, desc, arg1);\
	}
#define VERIFY4(expr, desc, arg1, arg2)\
	{\
		if (!(expr))\
			xrDebug::SoftFail(DEBUG_INFO, #expr, desc, arg1, arg2);\
	}

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