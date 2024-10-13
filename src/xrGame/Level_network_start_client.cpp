#include "stdafx.h"

#include "Level.h"
#include "xrEngine/x_ray.h"
#include "xrEngine/IGame_Persistent.h"

#include "ai_space.h"
#include "game_cl_base.h"
#include "NET_Queue.h"
#include "hudmanager.h"

#include "xrPhysics/iphworld.h"

#include "phcommander.h"
#include "physics_game.h"
#include "string_table.h"

bool CLevel::net_start_client1()
{
	// name_of_server
	string64 name_of_server = "";
	if (strchr(*m_caClientOptions, '/'))
		strncpy_s(name_of_server, *m_caClientOptions, strchr(*m_caClientOptions, '/') - *m_caClientOptions);

	if (strchr(name_of_server, '/'))
		*strchr(name_of_server, '/') = 0;

	//string256 temp;
	//xr_sprintf(temp, "%s %s",
	//	StringTable().translate("st_client_connecting_to").c_str(),
	//	name_of_server);

	//pApp->SetLoadStageTitle(temp);
	return true;
}

#include "xrServer.h"

bool CLevel::net_start_client2()
{
	Server->CreateSVClient();

	// offline account creation
	ClientReceive();
	Server->Update();

	connected_to_server = Connect2Server(*m_caClientOptions);

	return true;
}

bool CLevel::net_start_client3()
{
	if (connected_to_server)
	{
		LPCSTR level_name = nullptr;
		LPCSTR level_ver = nullptr;
		LPCSTR download_url = nullptr;

		shared_str const& server_options = Server->GetConnectOptions();
		level_name = name().c_str();
		level_ver = Server->level_version(server_options).c_str(); // 1.0

		// Determine internal level-ID
		int level_id = pApp->Level_ID(level_name, level_ver, true);
		if (level_id == -1)
		{
			connected_to_server = FALSE;
			Msg("! Level (name:%s), (version:%s), not found, try to download from:%s", level_name, level_ver,
				download_url);
			map_data.m_name = level_name;
			map_data.m_map_version = level_ver;
			map_data.m_map_download_url = download_url;
			map_data.m_map_loaded = false;
			::Render->grass_level_density = READ_IF_EXISTS(pGameIni, r_float, name().c_str(), "grass_density", 1.f);
			::Render->grass_level_scale = READ_IF_EXISTS(pGameIni, r_float, name().c_str(), "grass_scale", 1.f);
			return false;
		}
#ifdef DEBUG
		Msg("--- net_start_client3: level_id [%d], level_name[%s], level_version[%s]", level_id, level_name, level_ver);
#endif // #ifdef DEBUG
		map_data.m_name = level_name;
		map_data.m_map_version = level_ver;
		map_data.m_map_download_url = download_url;
		map_data.m_map_loaded = true;

		::Render->grass_level_density = READ_IF_EXISTS(pGameIni, r_float, name().c_str(), "grass_density", 1.f);
		::Render->grass_level_scale = READ_IF_EXISTS(pGameIni, r_float, name().c_str(), "grass_scale", 1.f);

		//deny_m_spawn = FALSE;
		// Load level
		R_ASSERT2(Load(level_id), "Loading failed.");
	}
	return true;
}

bool CLevel::net_start_client4()
{
	if (connected_to_server)
	{
		// Begin spawn
		pApp->SetLoadStageTitle("st_loading_create_physics");

		// Send physics to single or multithreaded mode

		create_physics_world(&ObjectSpace, &Objects, &Device);

		R_ASSERT(physics_world());

		m_ph_commander_physics_worldstep = new CPHCommander();
		physics_world()->set_update_callback(m_ph_commander_physics_worldstep);

		physics_world()->set_default_contact_shotmark(ContactShotMark);
		physics_world()->set_default_character_contact_shotmark(CharacterContactShotMark);

		VERIFY(physics_world());
		physics_world()->set_step_time_callback((PhysicsStepTimeCallback*)&PhisStepsCallback);

		// Send network to single or multithreaded mode
		// *note: release version always has "mt_*" enabled
		extern pureFrame* g_pNetProcessor;
		Device.seqFrameMT.Remove(g_pNetProcessor);
		Device.seqFrame.Remove(g_pNetProcessor);
		Device.seqFrameMT.Add(g_pNetProcessor, REG_PRIORITY_HIGH + 2);
	}
	return true;
}
//
//bool CLevel::net_start_client5()
//{
//	if (connected_to_server)
//	{
//		// Textures
//		pApp->SetLoadStageTitle("st_loading_textures");
//		//::Render->DeferredLoad(FALSE);
//		//::Render->ResourcesDeferredUpload();
//
//		deny_m_spawn = TRUE;
//	}
//	return true;
//}

bool CLevel::net_start_client6()
{
	if (connected_to_server)
	{
		// Sync
		if (!synchronize_map_data())
			return false;

		if (!game_configured)
		{
			return true;
		}
		g_hud->Load();
		g_hud->OnConnected();

#ifdef DEBUG
		Msg("--- net_start_client6");
#endif // #ifdef DEBUG

		if (game)
			game->OnConnected();

		Device.PreCache(60, true, true);
		net_start_result_total = TRUE;
	}
	else
	{
		net_start_result_total = FALSE;
	}
	return true;
}
