////////////////////////////////////////////////////////////////////////////
//	Module 		: script_game_object_impl.h
//	Created 	: 25.09.2003
//  Modified 	: 29.06.2004
//	Author		: Dmitriy Iassenev
//	Description : Script game object class implementation
////////////////////////////////////////////////////////////////////////////

#pragma once

#include "gameobject.h"
#include "ai_space.h"
#include "xrScriptEngine/script_engine.hpp"

IC CGameObject& CScriptGameObject::object() const
{
#ifdef DEBUG
	__try
	{
		if (m_game_object && m_game_object->lua_game_object() == this)
			return (*m_game_object);
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
	}

	::ScriptEngine->script_log(
		LuaMessageType::Error, "you are trying to use a destroyed object [%x]", m_game_object);
	THROW2(m_game_object && m_game_object->lua_game_object() == this,
		"Probably, you are trying to use a destroyed object!");
#endif // #ifdef DEBUG
	return (*m_game_object);
}
