////////////////////////////////////////////////////////////////////////////
//  Created	 : 19.06.2018
//  Authors	 : Xottab_DUTY (OpenXRay project)
//				FozeSt
//				Unfainthful
//
//  Copyright (C) GSC Game World - 2018
////////////////////////////////////////////////////////////////////////////
#pragma once

class ILoadingScreen
{
public:

	virtual ~ILoadingScreen() = default;
	xrCriticalSection Lock;

	virtual void Initialize() = 0;

	virtual void Update(u16 stagesCompleted, u16 stagesTotal) = 0;
	virtual void ForceFinish() = 0;

	virtual void ChangeVisibility(bool state) = 0;
	virtual const bool IsVisibility() = 0;

	virtual void SetLevelLogo(pcstr name) = 0;
	virtual void SetStageTitle(pcstr title) = 0;
	virtual void SetStageTip(pcstr header, pcstr tipNumber, pcstr tip, pcstr level_name) = 0;
};
