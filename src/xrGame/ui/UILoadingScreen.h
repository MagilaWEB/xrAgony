////////////////////////////////////////////////////////////////////////////
//  Created	 : 19.06.2018
//  Authors	 : Xottab_DUTY (OpenXRay project)
//				FozeSt
//				Unfainthful
//
//  Copyright (C) GSC Game World - 2018
////////////////////////////////////////////////////////////////////////////
#pragma once

#include "xrEngine/ILoadingScreen.h"
#include "xrUICore/Static/UIStatic.h"
#include "xrUICore/Windows/UIWindow.h"
#include "MMSound.h"

class CApplication;

class UILoadingScreen : public pureRender, public ILoadingScreen, public CUIWindow
{
	CUIXml uiXml;
	CUIProgressBar* loadingProgress{ nullptr };
	CUIStatic* loadingLogo{ nullptr };
	CUIStatic* loadingProgressPercent{ nullptr };

	CUIStatic* levelName{ nullptr };
	CUIStatic* loadingStage{ nullptr };
	CUIStatic* loadingHeader{ nullptr };
	CUIStatic* loadingTipNumber{ nullptr };
	CUIStatic* loadingTip{ nullptr };
	CMMSound* sound{ nullptr };
	CTimer timer_show;

	bool m_is_visibility{ false };
	bool m_start_show_timer{ false };

public:
	xrCriticalSection Lock;
	UILoadingScreen();
	~UILoadingScreen();

	void Initialize() override;

	void Update(u16 stagesCompleted, u16 stagesTotal) override;
	void OnRender();
	void ForceFinish() override;
	void ChangeVisibility(bool state) override;
	const bool IsVisibility() override;

	void SetLevelLogo(pcstr name) override;
	void SetStageTitle(pcstr title) override;
	void SetStageTip(pcstr header, pcstr tipNumber, pcstr tip, pcstr level_name) override;
};
