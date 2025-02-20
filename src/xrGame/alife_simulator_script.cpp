////////////////////////////////////////////////////////////////////////////
//	Module 		: alife_simulator_script.cpp
//	Created 	: 25.12.2002
//  Modified 	: 13.05.2004
//	Author		: Dmitriy Iassenev
//	Description : ALife Simulator script export
////////////////////////////////////////////////////////////////////////////

#include "pch_script.h"
#include "alife_simulator.h"
#include "ai_space.h"
#include "alife_object_registry.h"
#include "alife_story_registry.h"
#include "xrScriptEngine/script_engine.hpp"
#include "xrServer_Objects_ALife_Monsters.h"
#include "restriction_space.h"
#include "alife_graph_registry.h"
#include "alife_spawn_registry.h"
#include "alife_registry_container.h"
#include "xrServer.h"
#include "Level.h"
#include "xrScriptEngine/ScriptExporter.hpp"

using namespace luabind;
using namespace luabind::policy;

typedef xr_vector<std::pair<shared_str, int>> STORY_PAIRS;
typedef STORY_PAIRS SPAWN_STORY_PAIRS;
LPCSTR _INVALID_STORY_ID = "INVALID_STORY_ID";
LPCSTR _INVALID_SPAWN_STORY_ID = "INVALID_SPAWN_STORY_ID";
STORY_PAIRS story_ids;
SPAWN_STORY_PAIRS spawn_story_ids;

CALifeSimulator* alife() { return (const_cast<CALifeSimulator*>(ai().get_alife())); }
CSE_ALifeDynamicObject* alife_object(const CALifeSimulator* self, ALife::_OBJECT_ID object_id)
{
	VERIFY(self);
	if (object_id == 0xffff)
	{
		Msg("alife():object(id) ! invalid id specified");
		return nullptr;
	}
	return (self->objects().object(object_id, true));
}

bool valid_object_id(const CALifeSimulator* self, ALife::_OBJECT_ID object_id)
{
	VERIFY(self);
	return (object_id != 0xffff);
}

CSE_ALifeDynamicObject* alife_object(const CALifeSimulator* self, LPCSTR name)
{
	VERIFY(self);

	for (CALifeObjectRegistry::OBJECT_REGISTRY::const_iterator it = self->objects().objects().begin();
		 it != self->objects().objects().end(); it++)
	{
		CSE_ALifeDynamicObject* obj = it->second;
		if (xr_strcmp(obj->name_replace(), name) == 0)
			return (it->second);
	}

	return (0);
}

CSE_ALifeDynamicObject* alife_object(const CALifeSimulator* self, ALife::_OBJECT_ID id, bool no_assert)
{
	VERIFY(self);
	return (self->objects().object(id, no_assert));
}

CSE_ALifeDynamicObject* alife_story_object(const CALifeSimulator* self, ALife::_STORY_ID id)
{
	return (self->story_objects().object(id, true));
}

template <typename _id_type>
void generate_story_ids(STORY_PAIRS& result, _id_type INVALID_ID, LPCSTR section_name, LPCSTR INVALID_ID_STRING,
	LPCSTR invalid_id_description, LPCSTR invalid_id_redefinition, LPCSTR duplicated_id_description)
{
	result.clear();

	CInifile* Ini = pGameIni;

	LPCSTR N, V;
	u32 k;
	shared_str temp;
	LPCSTR section = section_name;
	R_ASSERT(Ini->section_exist(section));

	for (k = 0; Ini->r_line(section, k, &N, &V); ++k)
	{
		temp = Ini->r_string_wb(section, N);

		R_ASSERT3(!strchr(*temp, ' '), invalid_id_description, *temp);
		R_ASSERT2(xr_strcmp(*temp, INVALID_ID_STRING), invalid_id_redefinition);

		STORY_PAIRS::const_iterator I = result.begin();
		STORY_PAIRS::const_iterator E = result.end();
		for (; I != E; ++I)
			R_ASSERT3((*I).first != temp, duplicated_id_description, *temp);

		result.emplace_back(*temp, atoi(N));
	}

	result.emplace_back(INVALID_ID_STRING, INVALID_ID);
}

void kill_entity0(CALifeSimulator* alife, CSE_ALifeMonsterAbstract* monster, const GameGraph::_GRAPH_ID& game_vertex_id)
{
	alife->kill_entity(monster, game_vertex_id, 0);
}

void kill_entity1(CALifeSimulator* alife, CSE_ALifeMonsterAbstract* monster)
{
	alife->kill_entity(monster, monster->m_tGraphID, 0);
}

void add_in_restriction(CALifeSimulator* alife, CSE_ALifeMonsterAbstract* monster, ALife::_OBJECT_ID id)
{
	alife->add_restriction(monster->ID, id, RestrictionSpace::eRestrictorTypeIn);
}

void add_out_restriction(CALifeSimulator* alife, CSE_ALifeMonsterAbstract* monster, ALife::_OBJECT_ID id)
{
	alife->add_restriction(monster->ID, id, RestrictionSpace::eRestrictorTypeOut);
}

void remove_in_restriction(CALifeSimulator* alife, CSE_ALifeMonsterAbstract* monster, ALife::_OBJECT_ID id)
{
	alife->remove_restriction(monster->ID, id, RestrictionSpace::eRestrictorTypeIn);
}

void remove_out_restriction(CALifeSimulator* alife, CSE_ALifeMonsterAbstract* monster, ALife::_OBJECT_ID id)
{
	alife->remove_restriction(monster->ID, id, RestrictionSpace::eRestrictorTypeOut);
}

u32 get_level_id(CALifeSimulator* self) { return (self->graph().level().level_id()); }
CSE_ALifeDynamicObject* CALifeSimulator__create(CALifeSimulator* self, ALife::_SPAWN_ID spawn_id)
{
	const CALifeSpawnRegistry::SPAWN_GRAPH::CVertex* vertex = ai().alife().spawns().spawns().vertex(spawn_id);
	THROW2(vertex, "Invalid spawn id!");

	CSE_ALifeDynamicObject* spawn = smart_cast<CSE_ALifeDynamicObject*>(&vertex->data()->object());
	THROW(spawn);

	CSE_ALifeDynamicObject* object;
	self->create(object, spawn, spawn_id);

	return (object);
}

CSE_Abstract* CALifeSimulator__spawn_item(CALifeSimulator* self, LPCSTR section, const Fvector& position,
	u32 level_vertex_id, GameGraph::_GRAPH_ID game_vertex_id)
{
	THROW(self);
	return (self->spawn_item(section, position, level_vertex_id, game_vertex_id, ALife::_OBJECT_ID(-1)));
}

CSE_Abstract* CALifeSimulator__spawn_item2(CALifeSimulator* self, LPCSTR section, const Fvector& position,
	u32 level_vertex_id, GameGraph::_GRAPH_ID game_vertex_id, ALife::_OBJECT_ID id_parent)
{
	if (id_parent == ALife::_OBJECT_ID(-1))
		return (self->spawn_item(section, position, level_vertex_id, game_vertex_id, id_parent));

	CSE_ALifeDynamicObject* object = ai().alife().objects().object(id_parent, true);
	if (!object)
	{
		Msg("! invalid parent id [%d] specified", id_parent);
		return (0);
	}

	if (!object->m_bOnline)
		return (self->spawn_item(section, position, level_vertex_id, game_vertex_id, id_parent));

	NET_Packet packet;
	packet.w_begin(M_SPAWN);
	packet.w_stringZ(section);

	CSE_Abstract* item = self->spawn_item(section, position, level_vertex_id, game_vertex_id, id_parent, false);
	item->Spawn_Write(packet, FALSE);
	self->server().FreeID(item->ID, 0);
	F_entity_Destroy(item);

	ClientID clientID;
	clientID.set(0xffff);

	u16 dummy;
	packet.r_begin(dummy);
	VERIFY(dummy == M_SPAWN);
	return (self->server().Process_spawn(packet, clientID));
}

//Alundaio: Allows to call alife():register(se_obj) manually afterward so that packet editing can be done safely when spawning object with a parent
CSE_Abstract* CALifeSimulator__spawn_item3(CALifeSimulator* self, pcstr section, const Fvector& position,
											u32 level_vertex_id, GameGraph::_GRAPH_ID game_vertex_id,
											ALife::_OBJECT_ID id_parent, bool reg /*= true*/)
{
	if (reg == true)
		return CALifeSimulator__spawn_item2(self, section, position, level_vertex_id, game_vertex_id, id_parent);

	if (id_parent == ALife::_OBJECT_ID(-1))
		return self->spawn_item(section, position, level_vertex_id, game_vertex_id, id_parent);

	CSE_ALifeDynamicObject* object = ai().alife().objects().object(id_parent, true);
	if (!object)
	{
		Msg("! invalid parent id [%d] specified", id_parent);
		return nullptr;
	}

	if (!object->m_bOnline)
		return self->spawn_item(section, position, level_vertex_id, game_vertex_id, id_parent);

	CSE_Abstract* item = self->spawn_item(section, position, level_vertex_id, game_vertex_id, id_parent, false);

	return item;
}

CSE_Abstract* CALifeSimulator__spawn_ammo(CALifeSimulator* self, LPCSTR section, const Fvector& position,
	u32 level_vertex_id, GameGraph::_GRAPH_ID game_vertex_id, ALife::_OBJECT_ID id_parent, int ammo_to_spawn)
{
	//	if (id_parent == ALife::_OBJECT_ID(-1))
	//		return (self->spawn_item(section,position,level_vertex_id,game_vertex_id,id_parent));
	CSE_ALifeDynamicObject* object = 0;
	if (id_parent != ALife::_OBJECT_ID(-1))
	{
		object = ai().alife().objects().object(id_parent, true);
		if (!object)
		{
			Msg("! invalid parent id [%d] specified", id_parent);
			return (0);
		}
	}

	if (!object || !object->m_bOnline)
	{
		CSE_Abstract* item = self->spawn_item(section, position, level_vertex_id, game_vertex_id, id_parent);

		CSE_ALifeItemAmmo* ammo = smart_cast<CSE_ALifeItemAmmo*>(item);
		THROW(ammo);
		THROW(ammo->m_boxSize >= ammo_to_spawn);
		ammo->a_elapsed = (u16)ammo_to_spawn;

		return (item);
	}

	NET_Packet packet;
	packet.w_begin(M_SPAWN);
	packet.w_stringZ(section);

	CSE_Abstract* item = self->spawn_item(section, position, level_vertex_id, game_vertex_id, id_parent, false);

	CSE_ALifeItemAmmo* ammo = smart_cast<CSE_ALifeItemAmmo*>(item);
	THROW(ammo);
	THROW(ammo->m_boxSize >= ammo_to_spawn);
	ammo->a_elapsed = (u16)ammo_to_spawn;

	item->Spawn_Write(packet, FALSE);
	self->server().FreeID(item->ID, 0);
	F_entity_Destroy(item);

	ClientID clientID;
	clientID.set(0xffff);

	u16 dummy;
	packet.r_begin(dummy);
	VERIFY(dummy == M_SPAWN);
	return (self->server().Process_spawn(packet, clientID));
}

ALife::_SPAWN_ID CALifeSimulator__spawn_id(CALifeSimulator* self, ALife::_SPAWN_STORY_ID spawn_story_id)
{
	return (((const CALifeSimulator*)self)->spawns().spawn_id(spawn_story_id));
}

void CALifeSimulator__release(CALifeSimulator* self, CSE_Abstract* object, bool)
{
	VERIFY(self);
	//	self->release						(object,true);

	THROW(object);
	CSE_ALifeObject* alife_object = smart_cast<CSE_ALifeObject*>(object);
	THROW(alife_object);
	if (!alife_object->m_bOnline)
	{
		self->release(object, true);
		return;
	}

	// awesome hack, for everyone only
	IGameObject* obj = Level().Objects.net_Find(object->ID);
	if (!obj)
		return;

	if (obj->getDestroy())
		return;

	// awful hack, for stohe only
	NET_Packet packet;
	packet.w_begin(M_EVENT);
	packet.w_u32(Level().timeServer());
	packet.w_u16(GE_DESTROY);
	packet.w_u16(object->ID);
	Level().Send(packet);
}

void CALifeSimulator__release2(CALifeSimulator *self, CSE_Abstract *object)
{
	CALifeSimulator__release(self, object, false);
}

LPCSTR get_level_name(const CALifeSimulator* self, int level_id)
{
	LPCSTR result = *ai().game_graph().header().level((GameGraph::_LEVEL_ID)level_id).name();
	return (result);
}

CSE_ALifeCreatureActor* get_actor(const CALifeSimulator* self)
{
	THROW(self);
	return (self->graph().actor());
}

KNOWN_INFO_VECTOR* registry(const CALifeSimulator* self, const ALife::_OBJECT_ID& id)
{
	THROW(self);
	return (self->registry(info_portions).object(id, true));
}

bool has_info(const CALifeSimulator* self, const ALife::_OBJECT_ID& id, LPCSTR info_id)
{
	const KNOWN_INFO_VECTOR* known_info = registry(self, id);
	if (!known_info)
		return (false);

	if (std::find_if(known_info->begin(), known_info->end(), CFindByIDPred(info_id)) == known_info->end())
		return (false);

	return (true);
}

bool dont_has_info(const CALifeSimulator* self, const ALife::_OBJECT_ID& id, LPCSTR info_id)
{
	THROW(self);
	// absurdly, but only because of scriptwriters needs
	return (!has_info(self, id, info_id));
}

void AlifeGiveInfo(const CALifeSimulator *alife, const ALife::_OBJECT_ID &id, LPCSTR info_id)
{
	KNOWN_INFO_VECTOR *known_info = alife->registry(info_portions).object(id, true);
	if (!known_info)
		return;

	if (std::find_if(known_info->begin(), known_info->end(), CFindByIDPred(info_id)) == known_info->end())
	{
		known_info->push_back(info_id);
	}

	return;
}

void AlifeRemoveInfo(const CALifeSimulator *alife, const ALife::_OBJECT_ID &id, LPCSTR info_id)
{
	KNOWN_INFO_VECTOR	*known_info = alife->registry(info_portions).object(id, true);
	if (!known_info)
		return;
	known_info->erase(std::find_if(known_info->begin(), known_info->end(), CFindByIDPred(info_id)), known_info->end());
}

//Alundaio: teleport object
void teleport_object(CALifeSimulator* alife, ALife::_OBJECT_ID id, GameGraph::_GRAPH_ID game_vertex_id, u32 level_vertex_id, const Fvector& position)
{
	alife->teleport_object(id, game_vertex_id, level_vertex_id, position);
}

void IterateInfo(const CALifeSimulator* alife, const ALife::_OBJECT_ID& id, const luabind::functor<void>& functor)
{
	const auto known_info = registry(alife, id);
	if (!known_info)
		return;

	for (const auto& it : *known_info)
		functor(id, it.c_str());
}

CSE_Abstract* reprocess_spawn(CALifeSimulator* self, CSE_Abstract* object)
{
	NET_Packet packet;
	packet.w_begin(M_SPAWN);
	packet.w_stringZ(object->s_name);

	object->Spawn_Write(packet, FALSE);
	self->server().FreeID(object->ID, 0);
	F_entity_Destroy(object);

	ClientID clientID;
	clientID.set(0xffff);

	u16 dummy;
	packet.r_begin(dummy);

	return self->server().Process_spawn(packet, clientID);
}

CSE_Abstract* try_to_clone_object(CALifeSimulator* self, CSE_Abstract* object, pcstr section, const Fvector& position,
								  u32 level_vertex_id, GameGraph::_GRAPH_ID game_vertex_id, ALife::_OBJECT_ID id_parent,
								  bool bRegister = true)
{
	CSE_ALifeItemWeaponMagazined* wpnmag = smart_cast<CSE_ALifeItemWeaponMagazined*>(object);
	if (wpnmag)
	{
		CSE_Abstract* absClone = self->spawn_item(section, position, level_vertex_id, game_vertex_id, id_parent, false);
		if (!absClone)
			return nullptr;

		CSE_ALifeItemWeaponMagazined * clone = smart_cast<CSE_ALifeItemWeaponMagazined*>(absClone);
		if (!clone)
			return nullptr;

		clone->wpn_flags = wpnmag->wpn_flags;
		clone->m_addon_flags = wpnmag->m_addon_flags;
		clone->m_fCondition = wpnmag->m_fCondition;
		clone->ammo_type.data = wpnmag->ammo_type.data;
		clone->m_upgrades = wpnmag->m_upgrades;
		clone->a_elapsed.data = wpnmag->a_elapsed.data;
		clone->a_current_addon.data = wpnmag->a_current_addon.data;

		return bRegister ? reprocess_spawn(self, absClone) : absClone;
		// (self->server().Process_spawn(packet, clientID));
	}

	return nullptr;
}
CSE_Abstract* try_to_clone_object(CALifeSimulator* self, CSE_Abstract* object, pcstr section, const Fvector& position,
	u32 level_vertex_id, GameGraph::_GRAPH_ID game_vertex_id, ALife::_OBJECT_ID id_parent)
{
	return try_to_clone_object(self, object, section, position, level_vertex_id, game_vertex_id, id_parent, true);
}

void set_objects_per_update(CALifeSimulator* self, u32 count)
{
	self->objects_per_update(count);
}

void set_process_time(CALifeSimulator* self, int micro)
{
	self->set_process_time(micro);
}

const CALifeObjectRegistry::OBJECT_REGISTRY& alife_objects(const CALifeSimulator *self)
{
	VERIFY(self);
	return self->objects().objects();
}

xr_vector<u16>& get_children(const CALifeSimulator* self, CSE_Abstract* object)
{
	VERIFY(self);
	return object->children;
}
//-Alundaio

// clang-format off
SCRIPT_EXPORT(CALifeSimulator, (), {
	module(luaState)
	[
		class_<CALifeSimulator>("alife_simulator")
			.def("valid_object_id", &valid_object_id)
			.def("level_id", &get_level_id)
			.def("level_name", &get_level_name)
			.def("object", (CSE_ALifeDynamicObject * (*)(const CALifeSimulator*, ALife::_OBJECT_ID))(alife_object))
			.def("object", (CSE_ALifeDynamicObject * (*)(const CALifeSimulator*, ALife::_OBJECT_ID, bool))(alife_object))
			.def("object", (CSE_ALifeDynamicObject *(*) (const CALifeSimulator*, LPCSTR))(alife_object))
			.def("objects", &alife_objects, return_stl_pair_iterator())
			.def("story_object", (CSE_ALifeDynamicObject * (*)(const CALifeSimulator*, ALife::_STORY_ID))(alife_story_object))
			.def("set_switch_online", (void (CALifeSimulator::*)(ALife::_OBJECT_ID, bool))(&CALifeSimulator::set_switch_online))
			.def("set_switch_offline", (void (CALifeSimulator::*)(ALife::_OBJECT_ID, bool))(&CALifeSimulator::set_switch_offline))
			.def("set_interactive", (void (CALifeSimulator::*)(ALife::_OBJECT_ID, bool))(&CALifeSimulator::set_interactive))
			.def("kill_entity", &CALifeSimulator::kill_entity)
			.def("kill_entity", &kill_entity0)
			.def("kill_entity", &kill_entity1)
			.def("add_in_restriction", &add_in_restriction)
			.def("add_out_restriction", &add_out_restriction)
			.def("remove_in_restriction", &remove_in_restriction)
			.def("remove_out_restriction", &remove_out_restriction)
			.def("remove_all_restrictions", &CALifeSimulator::remove_all_restrictions)
			.def("create", &CALifeSimulator__create)
			.def("create", (CSE_Abstract* (*)(CALifeSimulator*, pcstr, const Fvector&, u32, GameGraph::_GRAPH_ID,
				ALife::_OBJECT_ID, bool))&CALifeSimulator__spawn_item3)
			.def("create", &CALifeSimulator__spawn_item2)
			.def("create", &CALifeSimulator__spawn_item)
			.def("create_ammo", &CALifeSimulator__spawn_ammo)
			.def("release", &CALifeSimulator__release)
			.def("release", &CALifeSimulator__release2)
			.def("spawn_id", &CALifeSimulator__spawn_id)
			.def("actor", &get_actor)
			.def("has_info", &has_info)
			.def("dont_has_info", &dont_has_info)
			.def("give_info", &AlifeGiveInfo)
			.def("disable_info", &AlifeRemoveInfo)
			.def("switch_distance", REMOVE_NOEXCEPT(&CALifeSimulator::switch_distance))
			.def("set_switch_distance", REMOVE_NOEXCEPT(&CALifeSimulator::set_switch_distance)) //Alundaio: renamed to set_switch_distance from switch_distance
			
			//Alundaio: extend alife simulator exports
			.def("teleport_object", &teleport_object)
			.def("iterate_info", &IterateInfo)
			.def("clone_weapon", (CSE_Abstract* (*)(CALifeSimulator*, CSE_Abstract*, pcstr, const Fvector&, u32,
				GameGraph::_GRAPH_ID, ALife::_OBJECT_ID))&try_to_clone_object)
			.def("clone_weapon", (CSE_Abstract* (*)(CALifeSimulator*, CSE_Abstract*, pcstr, const Fvector&, u32,
				GameGraph::_GRAPH_ID, ALife::_OBJECT_ID, bool))&try_to_clone_object)
			.def("register", &reprocess_spawn)
			.def("set_objects_per_update", &set_objects_per_update)
			.def("set_process_time", &set_process_time)
			.def("get_children", &get_children, return_stl_iterator())
			//Alundaio: END

		,
		def("alife", &alife)
	];
	class CALifeSimulatorExporter1
	{
	};
	{
		if (story_ids.empty())
			generate_story_ids(story_ids, INVALID_STORY_ID, "story_ids", "INVALID_STORY_ID",
				"Invalid story id description (contains spaces)!", "INVALID_STORY_ID redifinition!",
				"Duplicated story id description!");

		luabind::class_<CALifeSimulatorExporter1> instance("story_ids");

		STORY_PAIRS::const_iterator I = story_ids.begin();
		STORY_PAIRS::const_iterator E = story_ids.end();
		for (; I != E; ++I)
			instance.enum_("_story_ids")[luabind::value(*(*I).first, (*I).second)];

		luabind::module(luaState)[instance];
	}
	class CALifeSimulatorExporter2
	{
	};
	{
		if (spawn_story_ids.empty())
			generate_story_ids(spawn_story_ids, INVALID_SPAWN_STORY_ID, "spawn_story_ids", "INVALID_SPAWN_STORY_ID",
				"Invalid spawn story id description (contains spaces)!", "INVALID_SPAWN_STORY_ID redifinition!",
				"Duplicated spawn story id description!");

		luabind::class_<CALifeSimulatorExporter2> instance("spawn_story_ids");

		SPAWN_STORY_PAIRS::const_iterator I = spawn_story_ids.begin();
		SPAWN_STORY_PAIRS::const_iterator E = spawn_story_ids.end();
		for (; I != E; ++I)
			instance.enum_("_spawn_story_ids")[luabind::value(*(*I).first, (*I).second)];

		luabind::module(luaState)[instance];
	}
});
// clang-format on

#if 0 // def DEBUG
struct dummy {
	int count;
	lua_State* state;
	int ref;
};

void CALifeSimulator::validate			()
{
	typedef CALifeSpawnRegistry::SPAWN_GRAPH::const_vertex_iterator	const_vertex_iterator;
	const_vertex_iterator		I = spawns().spawns().vertices().begin();
	const_vertex_iterator		E = spawns().spawns().vertices().end();
	for ( ; I != E; ++I) {
		luabind::wrap_base		*base = smart_cast<luabind::wrap_base*>(&(*I).second->data()->object());
		if (!base)
			continue;

		if (!base->m_self.m_impl)
			continue;

		dummy					*_dummy = (dummy*)((void*)base->m_self.m_impl);
		lua_State				**_state = &_dummy->state;
		VERIFY2					(
			base->m_self.state(),
			make_string(
				"0x%08x name[%s] name_replace[%s]",
				*(int*)&_state,
				(*I).second->data()->object().name(),
				(*I).second->data()->object().name_replace()
			)
		);
	}
}
#endif // DEBUG
