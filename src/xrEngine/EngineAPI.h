// EngineAPI.h: interface for the CEngineAPI class.
//
//****************************************************************************
// Support for extension DLLs
//****************************************************************************
#pragma once
#include <memory>

#include "xrEngine/Engine.h"
#include "xrCore/ModuleLookup.hpp"
#include "xrCore/clsid.h"
#include "xrCore/xrCore_benchmark_macros.h"

class IGame_Persistent;

class IFactoryObject
{
public:
	virtual ~IFactoryObject() = 0;
	virtual CLASS_ID& GetClassId() = 0;
	virtual IFactoryObject* _construct() = 0;
};

inline IFactoryObject::~IFactoryObject() {}
inline IFactoryObject* IFactoryObject::_construct() { return this; }
class ENGINE_API FactoryObjectBase : public virtual IFactoryObject
{
public:
	CLASS_ID CLS_ID;

	FactoryObjectBase(void* params) { CLS_ID = 0; };
	FactoryObjectBase() { CLS_ID = 0; };
	virtual CLASS_ID& GetClassId() override { return CLS_ID; }
	virtual IFactoryObject* _construct() override { return IFactoryObject::_construct(); }
	virtual ~FactoryObjectBase(){};
};

// Class creation/destroying interface
extern "C" {
using Factory_Create = DLL_API IFactoryObject* __cdecl(CLASS_ID CLS_ID);
using Factory_Destroy = DLL_API void __cdecl(IFactoryObject* O);
using CreateGamePersistent = DLL_API IGame_Persistent* __cdecl();
using DestroyGamePersistent = DLL_API void __cdecl(IGame_Persistent*& O);
};

// Tuning interface
extern "C" {
using VTPause = void __cdecl(void);
using VTResume = void __cdecl(void);
};

class ENGINE_API CEngineAPI
{
	using SupportCheck = bool(*)();
	using SetupEnv = void(*)();

	XRay::Module hGame;
	XRay::Module hTuner;
	xr_vector<XRay::Module> m_renderers;

	SetupEnv m_setupSelectedRenderer;

public:
	BENCH_SEC_SCRAMBLEMEMBER1
	Factory_Create* pCreate;
	Factory_Destroy* pDestroy;

	CreateGamePersistent* pCreateGamePersisten;
	DestroyGamePersistent* pDestroyGamePersistent;

	bool tune_enabled;
	VTPause* tune_pause;
	VTResume* tune_resume;
	void Initialize();
	void CloseUnusedLibraries();

	void InitializeRenderers();
	void SelectRenderer();

	void Destroy();

	void CreateRendererList();

	CEngineAPI();
	~CEngineAPI();
};

ENGINE_API bool is_enough_address_space_available();

#define NEW_INSTANCE(a) Engine.External.pCreate(a)
#define DEL_INSTANCE(a)\
	{\
		Engine.External.pDestroy(a);\
		a = NULL;\
	}
