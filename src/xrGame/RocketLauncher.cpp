//////////////////////////////////////////////////////////////////////
// RocketLauncher.cpp:	интерфейс для семейства объектов
//						стреляющих гранатами и ракетами
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "RocketLauncher.h"
#include "CustomRocket.h"
#include "xrserver_objects_alife_items.h"
#include "Level.h"
#include "xrAICore/Navigation/ai_object_location.h"
#include "xrEngine/IGame_Persistent.h"

CRocketLauncher::CRocketLauncher()
{
	//	m_pRocket =  nullptr;
}
CRocketLauncher::~CRocketLauncher() {}
void CRocketLauncher::Load(LPCSTR section) { m_fLaunchSpeed = pSettings->r_float(section, "launch_speed"); }
void CRocketLauncher::SpawnRocket(const shared_str& rocket_section, CGameObject* parent_rocket_launcher)
{
	if (OnClient())
		return;

	CSE_Abstract* D = F_entity_Create(rocket_section.c_str());
	R_ASSERT(D);
	CSE_Temporary* l_tpTemporary = smart_cast<CSE_Temporary*>(D);
	R_ASSERT(l_tpTemporary);
	l_tpTemporary->m_tNodeID = parent_rocket_launcher->ai_location().level_vertex_id();
	D->s_name = rocket_section;
	D->set_name_replace("");

	//.	D->s_gameid			=	u8(GameID());
	D->s_RP = 0xff;
	D->ID = 0xffff;
	D->ID_Parent = parent_rocket_launcher->ID();
	D->ID_Phantom = 0xffff;
	D->s_flags.assign(M_SPAWN_OBJECT_LOCAL);
	D->RespawnTime = 0;

	NET_Packet P;
	D->Spawn_Write(P, TRUE);
	Level().Send(P);
	F_entity_Destroy(D);
}

void CRocketLauncher::AttachRocket(u16 rocket_id, CGameObject* parent_rocket_launcher)
{
	CCustomRocket* pRocket = smart_cast<CCustomRocket*>(Level().Objects.net_Find(rocket_id));
	pRocket->m_pOwner = smart_cast<CGameObject*>(parent_rocket_launcher->H_Root());
	VERIFY(pRocket->m_pOwner);
	pRocket->H_SetParent(parent_rocket_launcher);
	m_rockets.push_back(pRocket);
}

void CRocketLauncher::DetachRocket(u16 rocket_id, bool bLaunch)
{
	CCustomRocket* pRocket = smart_cast<CCustomRocket*>(Level().Objects.net_Find(rocket_id));
	if (!pRocket && OnClient())
		return;

	VERIFY(pRocket);

	bool find_result = false;

	if (!m_rockets.empty())
	{
		auto It = m_rockets.find(pRocket);
		find_result = It != m_rockets.end();

		if (OnServer())
			VERIFY(find_result);

		if (find_result)
		{
			(*It)->m_bLaunched = bLaunch;
			(*It)->H_SetParent(nullptr);
			m_rockets.erase(It);
		}
	}

	if (!m_launched_rockets.empty())
	{
		auto It_l = m_launched_rockets.find(pRocket);
		find_result = It_l != m_launched_rockets.end();

		if (OnServer())
			VERIFY(find_result);

		if (find_result)
		{
			(*It_l)->m_bLaunched = bLaunch;
			(*It_l)->H_SetParent(nullptr);
			m_launched_rockets.erase(It_l);
		}
	}
}

void CRocketLauncher::LaunchRocket(const Fmatrix& xform, const Fvector& vel, const Fvector& angular_vel)
{
	VERIFY2(_valid(xform), "CRocketLauncher::LaunchRocket. Invalid xform argument!");
	getCurrentRocket()->SetLaunchParams(xform, vel, angular_vel);

	m_launched_rockets.push_back(getCurrentRocket());
}

CCustomRocket* CRocketLauncher::getCurrentRocket()
{
	if (m_rockets.size())
		return m_rockets.back();
	return (CCustomRocket*)0;
}

void CRocketLauncher::dropCurrentRocket() { m_rockets.pop_back(); }
u32 CRocketLauncher::getRocketCount() { return m_rockets.size(); }
