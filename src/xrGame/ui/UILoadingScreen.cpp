////////////////////////////////////////////////////////////////////////////
//  Created	 : 19.06.2018
//  Authors	 : Xottab_DUTY (OpenXRay project)
//				FozeSt
//				Unfainthful
//
//  Copyright (C) GSC Game World - 2018
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "UILoadingScreen.h"
#include "UILoadingScreenHardcoded.h"

#include "xrEngine/x_ray.h"
#include "xrEngine/GameFont.h"
#include "UIHelper.h"
#include <GamePersistent.h>

UILoadingScreen::UILoadingScreen()
{
	UILoadingScreen::Initialize();
}

UILoadingScreen::~UILoadingScreen()
{
	xr_delete(sound);
}

void UILoadingScreen::Initialize()
{
	bool result = uiXml.Load(CONFIG_PATH, UI_PATH, UI_PATH_DEFAULT, "ui_mm_loading_screen.xml");

	if (!result) // Robustness? Yes!
	{
		if (UI().is_widescreen())
			uiXml.Set(LoadingScreenXML16x9);
		else
			uiXml.Set(LoadingScreenXML);
	}

	CUIXmlInit::InitWindow(uiXml, "background", 0, this);

	loadingLogo = UIHelper::CreateStatic(uiXml, "loading_logo", this);
	loadingLogo->Show(false);

	loadingProgress = UIHelper::CreateProgressBar(uiXml, "loading_progress", this);
	loadingProgressPercent = UIHelper::CreateStatic(uiXml, "loading_progress_percent", this);
	loadingStage = UIHelper::CreateStatic(uiXml, "loading_stage", this);

	levelName = UIHelper::CreateStatic(uiXml, "level_name", this);
	levelName->Show(false);

	loadingHeader = UIHelper::CreateStatic(uiXml, "loading_header", this);
	loadingHeader->Show(false);

	loadingTipNumber = UIHelper::CreateStatic(uiXml, "loading_tip_number", this);
	loadingTipNumber->Show(false);

	loadingTip = UIHelper::CreateStatic(uiXml, "loading_tip", this);
	loadingTip->Show(false);
}

void UILoadingScreen::Update(u16 stagesCompleted, u16 stagesTotal)
{
	xrCriticalSection::raii mt{ Lock };

	const float progress = float(stagesCompleted) / stagesTotal * loadingProgress->GetRange_max();
	if (loadingProgress->GetProgressPos() < progress)
		loadingProgress->SetProgressPos(progress);

	string16 buf;
	xr_sprintf(buf, "%.0f%%", loadingProgress->GetProgressPos());
	loadingProgressPercent->TextItemControl()->SetText(buf);

	CUIWindow::Update();
	Draw();

	if(sound)
		sound->music_Update();

	if (m_start_show_timer)
	{
		if (levelName->IsShown() == false && timer_show.GetElapsed_sec() > 1.f)
		{
			levelName->ResetColorAnimation();
			levelName->Show(true);
		}

		if (loadingHeader->IsShown() == false && timer_show.GetElapsed_sec() > 3.f)
		{
			loadingHeader->ResetColorAnimation();
			loadingHeader->Show(true);
		}

		if (loadingTipNumber->IsShown() == false && timer_show.GetElapsed_sec() > 4.f)
		{
			loadingTipNumber->ResetColorAnimation();
			loadingTipNumber->Show(true);
		}

		if (loadingTip->IsShown() == false && timer_show.GetElapsed_sec() > 5.f)
		{
			loadingTip->ResetColorAnimation();
			loadingTip->Show(true);
		}
	}
}

void UILoadingScreen::OnRender()
{
	Update(pApp->GetLoadState(), pApp->GetLoadStateMax());
}

void UILoadingScreen::ForceFinish()
{
	xrCriticalSection::raii mt{ Lock };
	loadingProgress->SetProgressPos(loadingProgress->GetRange_max());
	loadingProgress->Show(false);
	loadingProgressPercent->Show(false);
}

void UILoadingScreen::ChangeVisibility(bool state)
{
	xrCriticalSection::raii mt{ Lock };
	Show(state);

	if (state)
	{
		if (!sound)
		{
			sound = new CMMSound();
			sound->Init(uiXml, "loading_sound");
		}

		loadingProgress->SetProgressPos(0.f);
		loadingProgress->Show(true);
		loadingProgressPercent->Show(true);

		sound->music_Update();

		Device.seqRender.Add(this, 0);

		m_is_visibility = true;

		Device.ThreadLoad()->State(xrThread::dsOK);

		MainMenu()->DestroyInternal();
	}
	else
	{
		Device.ThreadLoad()->State(xrThread::dsSleep);
		Device.seqRender.Remove(this);

		levelName->Show(false);
		levelName->TextItemControl()->SetText("");
		loadingLogo->Show(false);
		loadingHeader->Show(false);
		loadingTipNumber->Show(false);
		loadingTip->Show(false);
		m_start_show_timer = false;

		if(sound)
			sound->all_Stop();

		m_is_visibility = false;
	}
}

const bool UILoadingScreen::IsVisibility()
{
	return m_is_visibility;
}

void UILoadingScreen::SetLevelLogo(pcstr name)
{
	xrCriticalSection::raii mt{ Lock };
	if (loadingLogo->IsShown() == false)
	{
		loadingLogo->InitTexture(name);
		loadingLogo->Show(true);
		loadingLogo->ResetColorAnimation();
	}
}

void UILoadingScreen::SetStageTitle(pcstr title)
{
	xrCriticalSection::raii mt{ Lock };
	if (title)
	{
		string256 buff;
		xr_sprintf(buff, "%s %s", StringTable().translate(title).c_str(), "...");
		loadingStage->TextItemControl()->SetText(buff);
	}
	else
		loadingStage->TextItemControl()->SetText("");
}

void UILoadingScreen::SetStageTip(pcstr header, pcstr tipNumber, pcstr tip, pcstr level_name)
{
	xrCriticalSection::raii mt{ Lock };
	if (!m_start_show_timer)
	{
		m_start_show_timer = true;
		timer_show.Start();
	}

	loadingHeader->TextItemControl()->SetText(header);
	loadingTipNumber->TextItemControl()->SetText(tipNumber);
	loadingTip->TextItemControl()->SetText(tip);
	levelName->TextItemControl()->SetText(level_name);
}
