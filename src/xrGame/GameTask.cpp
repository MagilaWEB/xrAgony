#include "pch_script.h"
#include "GameTask.h"
#include "xrUICore/XML/xrUIXmlParser.h"
#include "encyclopedia_article.h"
#include "map_location.h"
#include "map_spot.h"
#include "map_manager.h"

#include "Level.h"
#include "actor.h"
#include "xrScriptEngine/script_engine.hpp"
#include "xrScriptEngine/script_callback_ex.h"
#include "script_game_object.h"
#include "ai_space.h"
#include "alife_object_registry.h"
#include "alife_simulator.h"
#include "alife_story_registry.h"
#include "game_object_space.h"
#include "Common/object_broker.h"
#include "xrUICore/XML/UITextureMaster.h"

CGameTask::CGameTask()
	: m_map_object_id(0), m_TimeToComplete(0), m_priority(0)
{
	m_ReceiveTime = 0;
	m_FinishTime = 0;
	m_timer_finish = 0;
	m_Title = nullptr;
	m_Description = nullptr;
	m_ID = nullptr;
	m_task_type = eTaskTypeDummy;
	m_task_state = eTaskStateDummy;
	m_linked_map_location = nullptr;
	m_read = false;
}

void CGameTask::SetTaskState(ETaskState state)
{
	m_task_state = state;
	if ((m_task_state == eTaskStateFail) || (m_task_state == eTaskStateCompleted))
	{
		RemoveMapLocations(false);
		m_FinishTime = Level().GetGameTime();

		if (m_task_state == eTaskStateFail)
		{
			SendInfo(m_infos_on_fail);
			CallAllFuncs(m_lua_functions_on_fail);
		}
		else
		{
			SendInfo(m_infos_on_complete);
			CallAllFuncs(m_lua_functions_on_complete);
		}
	}
	ChangeStateCallback();
}

void CGameTask::OnArrived()
{
	m_task_state = eTaskStateInProgress;
	m_read = false;

	CreateMapLocation(false);
}

void CGameTask::CreateMapLocation(bool on_load)
{
	if (m_map_object_id == u16(-1) || m_map_location.size() == 0)
	{
		return;
	}

	if (on_load)
	{
		xr_vector<CMapLocation*> res;
		Level().MapManager().GetMapLocations(m_map_location, m_map_object_id, res);
		for (CMapLocation* ml : res)
		{
			if (ml->m_owner_task_id == m_ID)
			{
				m_linked_map_location = ml;
				break;
			}
		}
	}
	else
	{
		m_linked_map_location = Level().MapManager().AddMapLocation(m_map_location, m_map_object_id);
		m_linked_map_location->m_owner_task_id = m_ID;

		if (m_map_hint.size())
			m_linked_map_location->SetHint(m_map_hint);

		m_linked_map_location->DisablePointer();
		m_linked_map_location->SetSerializable(true);
	}

	VERIFY(m_linked_map_location);

	if (m_linked_map_location->complex_spot())
		m_linked_map_location->complex_spot()->SetTimerFinish(m_timer_finish);
}

void CGameTask::RemoveMapLocations(bool notify)
{
	if (m_linked_map_location && !notify)
		Level().MapManager().RemoveMapLocation(m_linked_map_location);

	m_map_location = nullptr;
	m_linked_map_location = nullptr;
	m_map_object_id = u16(-1);
}

void CGameTask::ChangeMapLocation(LPCSTR new_map_location, u16 new_map_object_id)
{
	RemoveMapLocations(false);

	m_map_location = new_map_location;
	m_map_object_id = new_map_object_id;

	m_task_state = eTaskStateInProgress;
	CreateMapLocation(false);
}

void CGameTask::ChangeStateCallback()
{
	Actor()->callback(GameObject::eTaskStateChange)(this, GetTaskState());
}

ETaskState CGameTask::UpdateState()
{
	if ((m_ReceiveTime != m_TimeToComplete))
		if (Level().GetGameTime() > m_TimeToComplete)
			return eTaskStateFail;

	// check fail infos
	if (CheckInfo(m_failInfos))
		return eTaskStateFail;

	// check fail functor
	if (CheckFunctions(m_fail_lua_functions))
		return eTaskStateFail;

	// check complete infos
	if (CheckInfo(m_completeInfos))
		return eTaskStateCompleted;

	// check complete functor
	if (CheckFunctions(m_complete_lua_functions))
		return eTaskStateCompleted;

	return GetTaskState();
}

bool CGameTask::CheckInfo(const xr_vector<shared_str>& v) const
{
	bool res = false;
	for (shared_str srt : v)
	{
		res = Actor()->HasInfo(srt);
		if (!res)
			break;
	}
	return res;
}

bool CGameTask::CheckFunctions(const task_state_functors& v) const
{
	bool res = false;
	for (luabind::functor<bool> functor : v)
	{
		if (functor.is_valid())
			res = functor(m_ID.c_str());

		if (!res)
			break;
	}
	return res;
}
void CGameTask::CallAllFuncs(const task_state_functors& v)
{
	for (luabind::functor<bool> functor : v)
		if (functor.is_valid())
			functor(m_ID.c_str());
}
void CGameTask::SendInfo(const xr_vector<shared_str>& v)
{
	for (shared_str srt : v)
		Actor()->TransferInfo(srt, true);
}

void CGameTask::save_task(IWriter& stream)
{
	save_data(m_task_state, stream);
	save_data(m_task_type, stream);
	save_data(m_ReceiveTime, stream);
	save_data(m_FinishTime, stream);
	save_data(m_TimeToComplete, stream);
	save_data(m_timer_finish, stream);

	save_data(m_Title, stream);
	save_data(m_Description, stream);
	save_data(m_pScriptHelper, stream);
	save_data(m_icon_texture_name, stream);
	save_data(m_map_hint, stream);
	save_data(m_map_location, stream);
	save_data(m_map_object_id, stream);
	save_data(m_priority, stream);
}

void CGameTask::load_task(IReader& stream)
{
	load_data(m_task_state, stream);
	load_data(m_task_type, stream);
	load_data(m_ReceiveTime, stream);
	load_data(m_FinishTime, stream);
	load_data(m_TimeToComplete, stream);
	load_data(m_timer_finish, stream);

	load_data(m_Title, stream);
	load_data(m_Description, stream);
	load_data(m_pScriptHelper, stream);
	load_data(m_icon_texture_name, stream);
	load_data(m_map_hint, stream);
	load_data(m_map_location, stream);
	load_data(m_map_object_id, stream);
	load_data(m_priority, stream);
	CommitScriptHelperContents();
	CreateMapLocation(true);
}

void CGameTask::CommitScriptHelperContents()
{
	m_pScriptHelper.init_functors(m_pScriptHelper.m_s_complete_lua_functions, m_complete_lua_functions);
	m_pScriptHelper.init_functors(m_pScriptHelper.m_s_fail_lua_functions, m_fail_lua_functions);
	m_pScriptHelper.init_functors(m_pScriptHelper.m_s_lua_functions_on_complete, m_lua_functions_on_complete);
	m_pScriptHelper.init_functors(m_pScriptHelper.m_s_lua_functions_on_fail, m_lua_functions_on_fail);
}

void CGameTask::AddCompleteInfo_script(LPCSTR _str) { m_completeInfos.push_back(_str); }
void CGameTask::AddFailInfo_script(LPCSTR _str) { m_failInfos.push_back(_str); }
void CGameTask::AddOnCompleteInfo_script(LPCSTR _str) { m_infos_on_complete.push_back(_str); }
void CGameTask::AddOnFailInfo_script(LPCSTR _str) { m_infos_on_fail.push_back(_str); }
void CGameTask::AddCompleteFunc_script(LPCSTR _str) { m_pScriptHelper.m_s_complete_lua_functions.push_back(_str); }
void CGameTask::AddFailFunc_script(LPCSTR _str) { m_pScriptHelper.m_s_fail_lua_functions.push_back(_str); }
void CGameTask::AddOnCompleteFunc_script(LPCSTR _str) { m_pScriptHelper.m_s_lua_functions_on_complete.push_back(_str); }
void CGameTask::AddOnFailFunc_script(LPCSTR _str) { m_pScriptHelper.m_s_lua_functions_on_fail.push_back(_str); }
void SScriptTaskHelper::init_functors(xr_vector<shared_str>& v_src, task_state_functors& v_dest)
{
	v_dest.resize(v_src.size());

	for (u32 it = 0; it < v_src.size(); ++it)
	{
		bool functor_exists = ::ScriptEngine->functor(v_src[it].c_str(), v_dest[it]);
		if (!functor_exists)
			Log("Cannot find script function described in task objective  ", v_src[it].c_str());
	}
}

void SScriptTaskHelper::load(IReader& stream)
{
	load_data(m_s_complete_lua_functions, stream);
	load_data(m_s_fail_lua_functions, stream);
	load_data(m_s_lua_functions_on_complete, stream);
	load_data(m_s_lua_functions_on_fail, stream);
}

void SScriptTaskHelper::save(IWriter& stream)
{
	save_data(m_s_complete_lua_functions, stream);
	save_data(m_s_fail_lua_functions, stream);
	save_data(m_s_lua_functions_on_complete, stream);
	save_data(m_s_lua_functions_on_fail, stream);
}

void SGameTaskKey::save(IWriter& stream)
{
	save_data(task_id, stream);
	game_task->save_task(stream);
}

void SGameTaskKey::load(IReader& stream)
{
	game_task = new CGameTask();
	load_data(task_id, stream);
	game_task->m_ID = task_id;
	game_task->load_task(stream);
}

void SGameTaskKey::destroy() { delete_data(game_task); }
