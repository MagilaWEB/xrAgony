#include "pch.hpp"
#include "FontManager.h"
#include "xrEngine/GameFont.h"
#include <xrCore\Text\MbHelpers.h>

CFontManager::CFontManager()
{
	//Device.seqDeviceReset.Add(this, REG_PRIORITY_HIGH);

	InitializeFonts();
}

CFontManager::~CFontManager()
{
	for (auto& [FontName, FontObj] : pFonts)
		xr_delete(FontObj);

	pFonts.clear();
	//Device.seqDeviceReset.Remove(this);
}

void CFontManager::InitializeFonts()
{
	const auto& section_list = pSettings->r_section("fonts_section");

	for (const auto& [pr_name, pr_content] : section_list.Data)
	{
		if (pSettings->section_exist(pr_name))
		{
			const pcstr font_name = pSettings->r_string(pr_name, "name");

			New(font_name, pr_name.c_str());
			FontsData.emplace(font_name, pr_name.c_str());
		}
		else
			Msg("WARNING: The font section [%s] does not exist, check the fonts.ltx file.", pr_name.c_str());
	}

	pFontMedium = Get("Medium");
	pFontDI = Get("DI");

	pFontArial14 = Clone("arial", "14", 14);

	pFontGraffiti19Russian = Clone("graffiti", "19", 19);

	pFontGraffiti22Russian = Clone("graffiti", "22", 22);

	pFontGraffiti32Russian = Clone("graffiti", "32", 32);

	pFontGraffiti50Russian = Clone("graffiti", "50", 50);

	pFontLetterica16Russian = Clone("letterica", "16", 16);

	pFontLetterica18Russian = Clone("letterica", "18", 18);

	pFontLetterica25 = Clone("letterica", "25", 25);

	pFontStat = Get("statistic");
}

CGameFont* CFontManager::New(LPCSTR name_id, LPCSTR section, u32 flags)
{
	auto it = pFonts.emplace(name_id, new CGameFont(section, flags));
	return std::move(it.first->second);
}

CGameFont* CFontManager::Get(LPCSTR name_id)
{
	for (auto & [FontName, FontObj] : pFonts)
		if (!xr_strcmp(FontName, name_id))
			return std::move(FontObj);

	return nullptr;
}

CGameFont* CFontManager::Clone(LPCSTR name_id, LPCSTR id_clone, u16 size)
{
	shared_str newName;
	newName.printf("%s%s", name_id, id_clone);

	for (const auto & [FontName, FonstObj] : pFonts)
	{
		if (!xr_strcmp(FontName, name_id))
		{
			if (xr_strcmp(FontName, newName))
				return pFonts.emplace(newName.c_str(), new CGameFont(FonstObj->GetSection(), FonstObj->GetFlags(), size)).first->second;
			else
			{
				FATAL("You cannot clone a font with the same cloning id two or more times!");
				return nullptr;
			}
		}
	}

	FATAL("Failed to clone font which was'nt initialized");
	return nullptr;
}

bool CFontManager::IsData(LPCSTR name_id)
{
	for (const auto & [name, section] : FontsData)
		if (!xr_strcmp(name, name_id))
			return true;

	return false;
}

std::pair<LPCSTR, LPCSTR> CFontManager::GetDataFont(const LPCSTR name_id) const
{
	for (const auto it : FontsData)
		if (!xr_strcmp(it.first, name_id))
			return it;

	return std::pair<LPCSTR, LPCSTR>(nullptr, nullptr);
}

CFontManager::DATA_FONT CFontManager::GetData() const
{
	return FontsData;
}

void CFontManager::Render()
{
	if (Device.m_ScopeVP.IsSVPRender())
		return;

	for (auto & iter : pFonts)
		iter.second->OnRender();
}

//void CFontManager::OnDeviceReset()
//{
	//InitializeFonts();
//}
