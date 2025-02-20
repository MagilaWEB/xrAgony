//////////////////////////////////////////////////////////////////////////
// character_info.cpp			игровая информация для персонажей в игре
//
//////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "character_info.h"

#ifdef XRGAME_EXPORTS
#include "xrUICore/XML/xrUIXmlParser.h"
#include "PhraseDialog.h"
#include "xrServer_Objects_ALife_Monsters.h"
#else // XRGAME_EXPORTS
#include "xrUICore/XML/xrUIXmlParser.h"
#endif // XRGAME_EXPORTS

//////////////////////////////////////////////////////////////////////////
SCharacterProfile::SCharacterProfile()
{
	m_CharacterId = nullptr;
	m_Rank = NO_RANK;
	m_Reputation = NO_REPUTATION;
}

SCharacterProfile::~SCharacterProfile() {}
//////////////////////////////////////////////////////////////////////////

CCharacterInfo::CCharacterInfo()
{
#ifdef XRGAME_EXPORTS
	m_CurrentRank.set(NO_RANK);
	m_CurrentReputation.set(NO_REPUTATION);
#endif
}

CCharacterInfo::~CCharacterInfo() {}
void CCharacterInfo::Load(shared_str id)
{
	m_ProfileId = id;
	inherited_shared::load_shared(m_ProfileId, nullptr);
}

#ifdef XRGAME_EXPORTS

void CCharacterInfo::InitSpecificCharacter(shared_str new_id)
{
	R_ASSERT(new_id.size());
	m_SpecificCharacterId = new_id;

	m_SpecificCharacter.Load(m_SpecificCharacterId);
	if (Rank().value() == NO_RANK)
	{
		if (m_SpecificCharacter.RankDef().min != m_SpecificCharacter.RankDef().max)
		{
			int rank = ::Random.randI(m_SpecificCharacter.RankDef().min, m_SpecificCharacter.RankDef().max);
			SetRank(rank);
		}
		else
			SetRank(m_SpecificCharacter.RankDef().max);
	}
	if (Reputation().value() == NO_REPUTATION)
	{
		if (m_SpecificCharacter.ReputationDef().min != m_SpecificCharacter.ReputationDef().max)
		{
			int rep = ::Random.randI(m_SpecificCharacter.ReputationDef().min, m_SpecificCharacter.ReputationDef().max);
			SetReputation(rep);
		}
		else
			SetReputation(m_SpecificCharacter.ReputationDef().max);
	}
	if (Community().index() == NO_COMMUNITY_INDEX)
		SetCommunity(m_SpecificCharacter.Community().index());
	if (!m_StartDialog || !m_StartDialog.size())
		m_StartDialog = m_SpecificCharacter.data()->m_StartDialog;
}

#endif

void CCharacterInfo::load_shared(LPCSTR)
{
	const ITEM_DATA& item_data = *id_to_index::GetById(m_ProfileId);

	CUIXml* pXML = item_data._xml;
	pXML->SetLocalRoot(pXML->GetRoot());

	XML_NODE item_node = pXML->NavigateToNode(id_to_index::tag_name, item_data.pos_in_file);
	R_ASSERT3(item_node, "profile id=", *item_data.id);

	pXML->SetLocalRoot(item_node);

	LPCSTR spec_char = pXML->Read("specific_character", 0, nullptr);
	if (!spec_char)
	{
		data()->m_CharacterId = nullptr;

		LPCSTR char_class = pXML->Read("class", 0, nullptr);

		if (char_class)
		{
			char* buf_str = xr_strdup(char_class);
			xr_strlwr(buf_str);
			data()->m_Class = buf_str;
			xr_free(buf_str);
		}
		else
			data()->m_Class = NO_CHARACTER_CLASS;

		int min_rank = pXML->ReadAttribInt("rank", 0, "min", NO_RANK);
		int max_rank = pXML->ReadAttribInt("rank", 0, "max", NO_RANK);
		if (min_rank != NO_RANK && max_rank != NO_RANK)
		{
			min_rank = _min(min_rank, max_rank);
			max_rank = _max(max_rank, min_rank);
			if (max_rank != min_rank)
				data()->m_Rank = ::Random.randI(min_rank, max_rank);
			else
				data()->m_Rank = max_rank;
		}
		else
			data()->m_Rank = pXML->ReadInt("rank", 0, NO_RANK);

		int min_reputation = pXML->ReadAttribInt("reputation", 0, "min", NO_REPUTATION);
		int max_reputation = pXML->ReadAttribInt("reputation", 0, "max", NO_REPUTATION);
		if (min_reputation != NO_REPUTATION && max_reputation != NO_REPUTATION)
		{
			min_reputation = _min(min_reputation, max_reputation);
			max_reputation = _max(max_reputation, min_reputation);
			if (max_reputation != min_reputation)
				data()->m_Reputation = ::Random.randI(min_reputation, max_reputation);
			else
				data()->m_Reputation = max_reputation;
		}
		else
			data()->m_Reputation = pXML->ReadInt("reputation", 0, NO_REPUTATION);
	}
	else
		data()->m_CharacterId = spec_char;
}

#ifdef XRGAME_EXPORTS
void CCharacterInfo::Init(CSE_ALifeTraderAbstract* trader)
{
	SetCommunity(trader->m_community_index);
	SetRank(trader->m_rank);
	SetReputation(trader->m_reputation);
	Load(trader->character_profile());
	InitSpecificCharacter(trader->specific_character());
}

shared_str CCharacterInfo::Profile() const { return m_ProfileId; }
LPCSTR CCharacterInfo::Name() const
{
	R_ASSERT2(m_SpecificCharacterId.size(), m_SpecificCharacter.Name());
	return m_SpecificCharacter.Name();
}

shared_str CCharacterInfo::Bio() const { return m_SpecificCharacter.Bio(); }
void CCharacterInfo::SetRank(CHARACTER_RANK_VALUE rank) { m_CurrentRank.set(rank); }
void CCharacterInfo::SetReputation(CHARACTER_REPUTATION_VALUE reputation) { m_CurrentReputation.set(reputation); }
void CCharacterInfo::SetCommunity(CHARACTER_COMMUNITY_INDEX community)
{
	m_CurrentCommunity.set(community);
	m_Sympathy = m_CurrentCommunity.sympathy(m_CurrentCommunity.index());
}

const shared_str& CCharacterInfo::IconName() const
{
	R_ASSERT(m_SpecificCharacterId.size());
	return m_SpecificCharacter.IconName();
}

shared_str CCharacterInfo::StartDialog() const { return m_StartDialog; }
const DIALOG_ID_VECTOR& CCharacterInfo::ActorDialogs() const
{
	R_ASSERT(m_SpecificCharacterId.size());
	return m_SpecificCharacter.data()->m_ActorDialogs;
}

void CCharacterInfo::load(IReader& stream) { stream.r_stringZ(m_StartDialog); }
void CCharacterInfo::save(NET_Packet& stream) { stream.w_stringZ(m_StartDialog); }
#endif

void CCharacterInfo::InitXmlIdToIndex()
{
	if (!id_to_index::tag_name)
		id_to_index::tag_name = "character";
	if (!id_to_index::file_str)
		id_to_index::file_str = pSettings->r_string("profiles", "files");
}
