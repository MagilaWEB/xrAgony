#include "stdafx.h"
#include "Include/xrRender/DrawUtils.h"
#include "Render.h"
#include "IGame_Persistent.h"
#include "xr_IOConsole.h"
#include "xr_input.h"

void CRenderDevice::Destroy()
{
	if (!b_is_Ready.load())
		return;
	Log("Destroying Direct3D...");
	pInput->ClipCursor(false);
	pInput->ShowCursor(false);
	::Render->ValidateHW();
	::DU->OnDeviceDestroy();
	b_is_Ready.store(false);
	Statistic->OnDeviceDestroy();
	::Render->destroy();
	::Render->OnDeviceDestroy(false);
	Memory.mem_compact();
	::Render->DestroyHW();
	seqRender.Clear();
	seqAppActivate.Clear();
	seqAppDeactivate.Clear();
	seqAppStart.Clear();
	seqAppEnd.Clear();
	seqFrame.Clear();
	seqFrameMT.Clear();
	seqDeviceReset.Clear();
	functionPointer.clear();
	seqParallel.clear();
	seqParallel2.clear();
	xr_delete(Statistic);
}

#include "IGame_Level.h"
#include "CustomHUD.h"
extern BOOL bNeed_re_create_env;
void CRenderDevice::Reset()
{
	b_restart = true;
}

bool CRenderDevice::IsReset() const
{
	return b_restart;
}

void CRenderDevice::ResetStart()
{
	xrCriticalSection::raii mt{ ResetRender };
	const auto dwWidth_before = dwWidth;
	const auto dwHeight_before = dwHeight;
	//pInput->ClipCursor(false);

	::Render->Reset(m_hWnd, dwWidth, dwHeight, fWidth_2, fHeight_2);
	GetWindowRect(m_hWnd, &m_rcWindowBounds);
	GetClientRect(m_hWnd, &m_rcWindowClient);

	if (g_pGamePersistent)
		g_pGamePersistent->Environment().bNeed_re_create_env = true;
	_SetupStates();

	// TODO: Remove this! It may hide crash
	Memory.mem_compact();

	seqDeviceReset.Process();
	if (dwWidth_before != dwWidth || dwHeight_before != dwHeight)
		seqResolutionChanged.Process();

	//pInput->ClipCursor(true);

	b_restart = false;
}
