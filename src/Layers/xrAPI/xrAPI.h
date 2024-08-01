#pragma once

class IRender;
class IRenderFactory;
class IDebugRender;
class CDUInterface;
struct xr_token;
class IUIRender;
class CGameMtlLibrary;
class CRender;
class CScriptEngine;
class AISpaceBase;
class ISoundManager;
class ui_core;

extern XRAPI_API IRender* Render;
extern XRAPI_API IDebugRender* DRender;
extern XRAPI_API CDUInterface* DU;
extern XRAPI_API IUIRender* UIRender;
extern XRAPI_API CGameMtlLibrary* PGMLib;
extern XRAPI_API IRenderFactory* RenderFactory;
extern XRAPI_API CScriptEngine* ScriptEngine;
extern XRAPI_API AISpaceBase* AISpace;
extern XRAPI_API ISoundManager* Sound;
extern XRAPI_API ui_core* UICore;

//extern XRAPI_API int CurrentRenderer;