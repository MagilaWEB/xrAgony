////////////////////////////////////////////////////////////////////////////
//	Module 		: base_client_classes_wrappers.h
//	Created 	: 20.12.2004
//  Modified 	: 20.12.2004
//	Author		: Dmitriy Iassenev
//	Description : XRay base client classes wrappers
////////////////////////////////////////////////////////////////////////////

#pragma once
#include "xrEngine/engineapi.h"
#include "xrCDB/ispatial.h"
#include "xrEngine/isheduled.h"
#include "xrEngine/irenderable.h"
#include "xrEngine/icollidable.h"
#include "xrEngine/xr_object.h"
#include "entity.h"
#include "ai_space.h"
#include "xrScriptEngine/script_engine.hpp"
#include "xrServer_Object_Base.h"

template <typename TBase>
class FactoryObjectWrapperTpl : public TBase, public luabind::wrap_base
{
public:
	FactoryObjectWrapperTpl() = default;
	virtual ~FactoryObjectWrapperTpl() = default;

	virtual IFactoryObject* _construct() { return call<IFactoryObject*>("_construct"); }
	static IFactoryObject* _construct_static(TBase* self) { return (self->TBase::_construct()); }

private:
	// not exported
	virtual CLASS_ID& GetClassId() override
	{
		static CLASS_ID id = -1;
		return id;
	}
};

typedef FactoryObjectWrapperTpl<IFactoryObject> FactoryObjectWrapper;

template <typename TBase, typename... TClasses>
class ISheduledWrapper : public TBase, public TClasses...
{
public:
	ISheduledWrapper() = default;
	virtual ~ISheduledWrapper() = default;

	virtual float shedule_Scale() { return 1; }

	virtual void shedule_Update(u32 dt) { TBase::shedule_Update(dt); }
};

typedef ISheduledWrapper<ISheduled, luabind::wrap_base> CISheduledWrapper;

template <typename... TBaseClasses>
class IRenderableWrapper : public TBaseClasses...
{
public:
	IRenderableWrapper() = default;
	virtual ~IRenderableWrapper() = default;
};

using CIRenderableWrapper = IRenderableWrapper<IRenderable, luabind::wrap_base>;

using CGameObjectIFactoryObject = FactoryObjectWrapperTpl<CGameObject>;
using CGameObjectISheduled = ISheduledWrapper<CGameObjectIFactoryObject>;
using CGameObjectIRenderable = IRenderableWrapper<CGameObjectISheduled>;

class CGameObjectWrapper : public CGameObjectIRenderable
{
public:
	CGameObjectWrapper() = default;
	virtual ~CGameObjectWrapper() = default;

	virtual bool use(CGameObject* who_use) { return call<bool>("use", who_use); }
	static bool use_static(CGameObject* self, CGameObject* who_use) { return self->CGameObject::use(who_use); }
	virtual void net_Import(NET_Packet& packet) { call<void>("net_Import", &packet); }
	static void net_Import_static(CGameObject* self, NET_Packet* packet) { self->CGameObject::net_Import(*packet); }
	virtual void net_Export(NET_Packet& packet) { call<void>("net_Export", &packet); }
	static void net_Export_static(CGameObject* self, NET_Packet* packet) { self->CGameObject::net_Export(*packet); }
	virtual BOOL net_Spawn(CSE_Abstract* data) {return call<bool>("net_Spawn", data); }
	static bool net_Spawn_static(CGameObject* self, CSE_Abstract* abstract)
	{
		return (!!self->CGameObject::net_Spawn(abstract));
	}
};

class CEntityWrapper : public CEntity, public luabind::wrap_base
{
public:
	CEntityWrapper() = default;
	virtual ~CEntityWrapper() = default;

	virtual void HitSignal(float P, Fvector& local_dir, IGameObject* who, s16 element)
	{
		call<void>("HitSignal", P, local_dir, who, element);
	}

	static void HitSignal_static(CEntity* self, float P, Fvector& local_dir, IGameObject* who, s16 element)
	{
		::ScriptEngine->script_log(
			LuaMessageType::Error, "You are trying to call a pure virtual function CEntity::HitSignal!");
	}

	virtual void HitImpulse(float P, Fvector& vWorldDir, Fvector& vLocalDir)
	{
		call<void>("HitImpulse", P, vWorldDir, vLocalDir);
	}

	static void HitImpulse_static(float P, Fvector& vWorldDir, Fvector& vLocalDir)
	{
		::ScriptEngine->script_log(
			LuaMessageType::Error, "You are trying to call a pure virtual function CEntity::HitImpulse!");
	}
};
