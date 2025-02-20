#include "stdafx.h"
#include "Include/xrRender/DrawUtils.h"
#include "Render.h"
#include "xrCDB/xrXRC.h"

extern XRCDB_API BOOL* cdb_bDebug;

void CRenderDevice::_SetupStates()
{
	// General Render States
	mView.identity();
	mProject.identity();
	mFullTransform.identity();
	vCameraPosition.set(0, 0, 0);
	vCameraDirection.set(0, 0, 1);
	vCameraTop.set(0, 1, 0);
	vCameraRight.set(1, 0, 0);
	::Render->SetupStates();
}

void CRenderDevice::Create()
{
	if (b_is_Ready)
		return; // prevent double call
	Statistic = new CStats();
	bool gpuSW = !!strstr(Core.Params, "-gpu_sw");
	bool gpuNonPure = !!strstr(Core.Params, "-gpu_nopure");
	bool gpuRef = !!strstr(Core.Params, "-gpu_ref");
	::Render->SetupGPU(gpuSW, gpuNonPure, gpuRef);
	Log("Starting RENDER device...");
#ifdef _EDITOR
	psCurrentVidMode[0] = dwWidth;
	psCurrentVidMode[1] = dwHeight;
#endif
	fFOV = 90.f;
	fASPECT = 1.f;

	::Render->Create(m_hWnd, dwWidth, dwHeight, fWidth_2, fHeight_2, true);
	GetWindowRect(m_hWnd, &m_rcWindowBounds);
	GetClientRect(m_hWnd, &m_rcWindowClient);
	Memory.mem_compact();
	b_is_Ready = TRUE;
	_SetupStates();
	string_path fname;
	FS.update_path(fname, "$game_data$", "shaders.xr");
	::Render->OnDeviceCreate(fname);
	Statistic->OnDeviceCreate();
	dwFrame = 0;
	PreCache(0, false, false);
}
