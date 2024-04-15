#include "stdafx.h"
#include "xrCDB/Frustum.h"

#pragma warning(push)
#pragma warning(disable : 4995)
// mmsystem.h
#define MMNOSOUND
#define MMNOMIDI
#define MMNOAUX
#define MMNOMIXER
#define MMNOJOY
#include <mmsystem.h>
#pragma warning(pop)

#include "x_ray.h"
#include "Render.h"

// must be defined before include of FS_impl.h
#define INCLUDE_FROM_ENGINE
#include "xrCore/FS_impl.h"

#include "Include/editor/ide.hpp"
#include "engine_impl.hpp"

#include "xrSASH.h"
#include "IGame_Persistent.h"
#include "xrScriptEngine/ScriptExporter.hpp"
#include "xr_input.h"
#include "splash.h"

ENGINE_API CRenderDevice Device;
ENGINE_API CLoadScreenRenderer load_screen_renderer;
ENGINE_API xr_list<LOADING_EVENT> g_loading_events;
ENGINE_API BOOL g_bRendering = FALSE;


extern int g_process_priority;
extern int g_GlobalFPSlimit;//Limit fps global.
extern int g_PausedFPSlimit;//Limit fps to Pause.
extern int g_MainFPSlimit;//Limit fps to main.

ref_light precache_light = 0;
u32 g_dwFPSlimit = 60;

void CRenderDevice::Run()
{
	Log("Starting engine...");

	// Startup timers and calculate timer delta
	dwTimeGlobal = 0;
	Timer_MM_Delta = 0;
	{
		u32 time_mm = timeGetTime();
		while (timeGetTime() == time_mm)
			; // wait for next tick
		u32 time_system = timeGetTime();
		u32 time_local = TimerAsync();
		Timer_MM_Delta = time_system - time_local;
	}

	seqAppStart.Process();

	mt_global_update.Init([this]() {
		xrCriticalSection::raii mt{ ResetRender };
		if ((!RunFunctionPointer()) && (!b_restart))
		{
			ProcessPriority();
			GlobalUpdate();
			FpsCalc();
		}
	});

	mt_frame.Init([this]() {
		OnFrame();
	}, xrThread::sParalelRender);

	mt_frame2.Init([this]() {
		while (!seqParallel2.empty())
		{
			seqParallel2.front()();
			seqParallel2.pop_front();
		}
	}, xrThread::sParalelRender);

	// Message cycle
	GEnv.Render->ClearTarget();
	splash::hide();
	message_loop();
	seqAppEnd.Process();
}

//Sets the priority of the process from the parameter in real time.
void CRenderDevice::ProcessPriority()
{
	static int set_id = 0;
	if (set_id != g_process_priority)
	{
		switch (g_process_priority)
		{
		case 1:
			SetPriorityClass(GetCurrentProcess(), REALTIME_PRIORITY_CLASS);
			Log("- The process priority (real-time) has been successfully set.");
			break;
		case 2:
			SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS);
			Log("- The process priority (high) has been successfully set.");
			break;
		case 3:
			SetPriorityClass(GetCurrentProcess(), ABOVE_NORMAL_PRIORITY_CLASS);
			Log("- The process priority (above normal) has been successfully set.");
			break;
		case 4:
			SetPriorityClass(GetCurrentProcess(), NORMAL_PRIORITY_CLASS);
			Log("- The process priority (normal) has been successfully set.");
			break;
		case 5:
			SetPriorityClass(GetCurrentProcess(), BELOW_NORMAL_PRIORITY_CLASS);
			Log("- The process priority (below normal) has been successfully set.");
			break;
		case 6:
			SetPriorityClass(GetCurrentProcess(), IDLE_PRIORITY_CLASS);
			Log("- The process priority (low) has been successfully set.");
			break;
		default:
			g_process_priority = 4;
			SetPriorityClass(GetCurrentProcess(), NORMAL_PRIORITY_CLASS);
			Log("~ The priority of the process was not selected or was selected incorrectly, set by default (normal).");
		}

		set_id = g_process_priority;
	}
}

BOOL CRenderDevice::Begin()
{
	if (GEnv.isDedicatedServer)
		return TRUE;

	if (GEnv.Render->GetDeviceState() == DeviceState::Lost)
	{
		// If the device was lost, do not render until we get it back
		Sleep(33);
		return FALSE;
	}

	if (GEnv.Render->GetDeviceState() == DeviceState::NeedReset)
		Reset();

	GEnv.Render->Begin();
	FPU::m24r();
	g_bRendering = TRUE;

	return TRUE;
}

void CRenderDevice::Clear() { GEnv.Render->Clear(); }

bool CRenderDevice::ActiveMain()
{
	return b_is_Active && g_pGamePersistent && g_pGamePersistent->m_pMainMenu && g_pGamePersistent->m_pMainMenu->IsActive();
}

void CRenderDevice::End(void)
{
	if (GEnv.isDedicatedServer)
		return;

	bool load_finished = false;
	if (dwPrecacheFrame)
	{
		GEnv.Sound->set_master_volume(0.f);
		dwPrecacheFrame--;
		if (!dwPrecacheFrame)
		{
			load_finished = true;
			GEnv.Render->updateGamma();
			if (precache_light)
			{
				precache_light->set_active(false);
				precache_light.destroy();
			}
			GEnv.Sound->set_master_volume(1.f);
			GEnv.Render->ResourcesDestroyNecessaryTextures();
			Memory.mem_compact();
			Msg("* MEMORY USAGE: %d K", Memory.mem_usage() / 1024);
			Msg("* End of synchronization A[%d] R[%d]", b_is_Active, b_is_Ready);
#ifdef FIND_CHUNK_BENCHMARK_ENABLE
			g_find_chunk_counter.flush();
#endif

			if (g_pGamePersistent->GameType() == 1) // haCk
			{
				WINDOWINFO wi;
				GetWindowInfo(m_hWnd, &wi);
				if (wi.dwWindowStatus != WS_ACTIVECAPTION)
					Pause(TRUE, TRUE, TRUE, "application start");
			}
		}
	}
	g_bRendering = FALSE;
	// end scene
	// Present goes here, so call OA Frame end.
	if (g_SASH.IsBenchmarkRunning())
		g_SASH.DisplayFrame(Device.fTimeGlobal);
	GEnv.Render->End();

	if (load_finished && m_editor)
		m_editor->on_load_finished();
}

#include "IGame_Level.h"
void CRenderDevice::PreCache(u32 amount, bool b_draw_loadscreen, bool b_wait_user_input)
{
	if (GEnv.isDedicatedServer)
		amount = 0;
	else if (GEnv.Render->GetForceGPU_REF())
		amount = 0;

	dwPrecacheFrame = dwPrecacheTotal = amount;
	if (amount && !precache_light && g_pGameLevel && g_loading_events.empty())
	{
		precache_light = GEnv.Render->light_create();
		precache_light->set_shadow(false);
		precache_light->set_position(vCameraPosition);
		precache_light->set_color(255, 255, 255);
		precache_light->set_range(5.0f);
		precache_light->set_active(true);
	}
	if (amount && b_draw_loadscreen && !load_screen_renderer.b_registered)
	{
		load_screen_renderer.start(b_wait_user_input);
	}
}

//Frame per second counter, updated every 333 ms
void CRenderDevice::FpsCalc()
{
	//the inaccuracy is about ~0.1%
	static u16 HFPS = 0;
	static u16 HFPSViewport = 0;
	static CTimer timer;

	if (m_SecondViewport.IsSVPFrame())
		HFPSViewport++;
	else
		HFPS++;

	if ((timer.GetElapsed_sec() * 1000) >= float(1000 / 3))
	{
		FPS = HFPS > 3 ? (HFPS * 3) - 3 : HFPS;
		FPSViewport = HFPSViewport > 3 ? (HFPSViewport * 3) - 3 : HFPSViewport;
		HFPS = 0;
		HFPSViewport = 0;
		timer.Start();
	}
}

void CRenderDevice::CalcFrameStats()
{
	stats.RenderTotal.FrameEnd();
	// calc FPS & TPS
	if (fTimeDelta > EPS_S)
	{
		float fps = 1.f / fTimeDelta;
		// if (Engine.External.tune_enabled) vtune.update (fps);
		float fOne = 0.3f;
		float fInv = 1.0f - fOne;
		stats.fFPS = fInv * stats.fFPS + fOne * fps;
		if (stats.RenderTotal.result > EPS_S)
		{
			u32 renderedPolys = GEnv.Render->GetCacheStatPolys();
			stats.fTPS = fInv * stats.fTPS + fOne * float(renderedPolys) / (stats.RenderTotal.result * 1000.f);
			stats.fRFPS = fInv * stats.fRFPS + fOne * 1000.f / stats.RenderTotal.result;
		}
	}
	stats.RenderTotal.FrameStart();
}

void CRenderDevice::OnFrame()
{
	while (!seqParallel.empty())
	{
		seqParallel.front()();
		seqParallel.pop_front();
	}

	seqFrameMT.Process();
}

void CRenderDevice::GlobalUpdate()
{
	if (psDeviceFlags.test(rsStatistic))
		g_bEnableStatGather = TRUE; // XXX: why not use either rsStatistic or g_bEnableStatGather?
	else
		g_bEnableStatGather = FALSE;

	if (g_loading_events.size())
	{
		if (g_loading_events.front()())
			g_loading_events.pop_front();
		pApp->LoadDraw();
		return;
	}
	else
	{
		const u32 limit = ActiveMain() ? g_MainFPSlimit : Paused() ? g_PausedFPSlimit : g_GlobalFPSlimit;
		// FPS Lock
		if (limit > 0 && !m_SecondViewport.IsSVPFrame())
		{
			static CTimer dwTime;
			const float updateDelta = 1000.f / limit;
			const float elapsed = dwTime.GetElapsed_sec() * 1000;
			if (elapsed < updateDelta)
				Sleep(DWORD(updateDelta - elapsed));

			dwTime.Start();
		}
	}

	FrameMove();

	// Precache
	if (dwPrecacheFrame)
	{
		float factor = float(dwPrecacheFrame) / float(dwPrecacheTotal);
		float angle = PI_MUL_2 * factor;
		vCameraDirection.set(_sin(angle), 0, _cos(angle));
		vCameraDirection.normalize();
		vCameraTop.set(0, 1, 0);
		vCameraRight.crossproduct(vCameraTop, vCameraDirection);
		mView.build_camera_dir(vCameraPosition, vCameraDirection, vCameraTop);
	}
	// Matrices
	mFullTransform.mul(mProject, mView);
	GEnv.Render->SetCacheXform(mView, mProject);
	mInvFullTransform.invert_44(mFullTransform);

	vCameraPositionSaved = vCameraPosition;
	vCameraDirectionSaved = vCameraDirection;
	vCameraTopSaved = vCameraTop;
	vCameraRightSaved = vCameraRight;

	mFullTransformSaved = mFullTransform;
	mViewSaved = mView;
	mProjectSaved = mProject;

	xrThread::StartGlobal(xrThread::sParalelRender);
	// all rendering is done here
	CStatTimer renderTotalReal;
	renderTotalReal.FrameStart();
	renderTotalReal.Begin();
	if (b_is_Active && Begin())
	{
		seqRender.Process();
		CalcFrameStats();
		Statistic->Show();
		End(); // Present goes here
	}
	renderTotalReal.End();
	renderTotalReal.FrameEnd();
	stats.RenderTotal.accum = renderTotalReal.accum;

	xrThread::WaitGlobal();
}

void CRenderDevice::message_loop_weather_editor()
{
	m_editor->run();
	m_editor_finalize(m_editor);
	xr_delete(m_engine);
}

void CRenderDevice::message_loop()
{
	if (editor())
	{
		message_loop_weather_editor();
		return;
	}

	ShowWindow(m_hWnd, SW_SHOW);

	PeekMessage(&msg, NULL, 0U, 0U, PM_NOREMOVE);
	while (msg.message != WM_QUIT)
	{
		if (b_restart)
			ResetStart();

		if (PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
			continue;
		}

		Sleep(30);
	}

	xrThread::GlobalState(xrThread::dsExit);
}

u32 app_inactive_time = 0;
u32 app_inactive_time_start = 0;

void CRenderDevice::FrameMove()
{
	dwFrame++;
	Core.dwFrame = dwFrame;
	dwTimeContinual = TimerMM.GetElapsed_ms() - app_inactive_time;
	if (psDeviceFlags.test(rsConstantFPS))
	{
		// 20ms = 50fps
		// fTimeDelta = 0.020f;
		// fTimeGlobal += 0.020f;
		// dwTimeDelta = 20;
		// dwTimeGlobal += 20;
		// 33ms = 30fps
		fTimeDelta = 0.033f;
		fTimeGlobal += 0.033f;
		dwTimeDelta = 33;
		dwTimeGlobal += 33;
	}
	else
	{
		// Timer
		float fPreviousFrameTime = Timer.GetElapsed_sec();
		Timer.Start(); // previous frame
		fTimeDelta =
			0.1f * fTimeDelta + 0.9f * fPreviousFrameTime; // smooth random system activity - worst case ~7% error
		// fTimeDelta = 0.7f * fTimeDelta + 0.3f*fPreviousFrameTime; // smooth random system activity
		if (fTimeDelta > .1f)
			fTimeDelta = .1f; // limit to 15fps minimum
		if (fTimeDelta <= 0.f)
			fTimeDelta = EPS_S + EPS_S; // limit to 15fps minimum
		if (Paused())
			fTimeDelta = 0.0f;
		// u64 qTime = TimerGlobal.GetElapsed_clk();
		fTimeGlobal = TimerGlobal.GetElapsed_sec(); // float(qTime)*CPU::cycles2seconds;
		u32 _old_global = dwTimeGlobal;
		dwTimeGlobal = TimerGlobal.GetElapsed_ms();
		dwTimeDelta = dwTimeGlobal - _old_global;
	}

	xrThread::StartGlobal(xrThread::sParalelFrame);
	// Frame move
	stats.EngineTotal.FrameStart();
	stats.EngineTotal.Begin();

	seqFrame.Process();

	stats.EngineTotal.End();
	stats.EngineTotal.FrameEnd();
}

ENGINE_API BOOL bShowPauseString = TRUE;
#include "IGame_Persistent.h"

void CRenderDevice::Pause(BOOL bOn, BOOL bTimer, BOOL bSound, LPCSTR reason)
{
	static int snd_emitters_ = -1;
	if (g_bBenchmark || GEnv.isDedicatedServer)
		return;

	if (bOn)
	{
		if (!Paused())
			bShowPauseString =
			editor() ? FALSE :
#ifdef DEBUG
			!xr_strcmp(reason, "li_pause_key_no_clip") ? FALSE :
#endif // DEBUG
			TRUE;
		if (bTimer && (!g_pGamePersistent || g_pGamePersistent->CanBePaused()))
		{
			g_pauseMngr().Pause(TRUE);
#ifdef DEBUG
			if (!xr_strcmp(reason, "li_pause_key_no_clip"))
				TimerGlobal.Pause(FALSE);
#endif
		}
		if (bSound && GEnv.Sound)
			snd_emitters_ = GEnv.Sound->pause_emitters(true);
	}
	else
	{
		if (bTimer && g_pauseMngr().Paused())
		{
			fTimeDelta = EPS_S + EPS_S;
			g_pauseMngr().Pause(FALSE);
		}
		if (bSound)
		{
			if (snd_emitters_ > 0) // avoid crash
				snd_emitters_ = GEnv.Sound->pause_emitters(false);
			else
			{
#ifdef DEBUG
				Log("GEnv.Sound->pause_emitters underflow");
#endif
			}
		}
	}
}

BOOL CRenderDevice::Paused() { return g_pauseMngr().Paused(); }
void CRenderDevice::OnWM_Activate(WPARAM wParam, LPARAM /*lParam*/)
{
	u16 fActive = LOWORD(wParam);
	const BOOL fMinimized = (BOOL)HIWORD(wParam);
	const BOOL isWndActive = (fActive != WA_INACTIVE && !fMinimized) ? TRUE : FALSE;
	
	if (isWndActive != b_is_Active)
	{
		b_is_Active = isWndActive;

		if (b_is_Active)
		{	
			if (!editor() && !GEnv.isDedicatedServer)
				pInput->ClipCursor(true);

			if (mt_global_update.IsInit() && GEnv.Render->GetDeviceState() == DeviceState::Lost)
				ResetStart();

			//ShowWindow(m_hWnd, SW_SHOW);
			xrThread::GlobalState(xrThread::dsOK);
		}
		else
		{
			pInput->ClipCursor(false);
			ShowWindow(m_hWnd, SW_MINIMIZE);
			xrThread::GlobalState(xrThread::dsSleep);
		}

		functionPointer.push_back([&]() {
			if (b_is_Active)
			{
				seqAppActivate.Process();
				app_inactive_time += TimerMM.GetElapsed_ms() - app_inactive_time_start;
			}
			else
			{
				app_inactive_time_start = TimerMM.GetElapsed_ms();
				seqAppDeactivate.Process();
			}

			seqParallel.clear();
			seqParallel2.clear();
		});
	}
}

void CRenderDevice::AddSeqFrame(pureFrame* f, bool mt)
{
	if (mt)
		seqFrameMT.Add(f, REG_PRIORITY_HIGH);
	else
		seqFrame.Add(f, REG_PRIORITY_LOW);
}

void CRenderDevice::RemoveSeqFrame(pureFrame* f)
{
	seqFrameMT.Remove(f);
	seqFrame.Remove(f);
}

CRenderDevice* get_device() { return &Device; }
u32 script_time_global() { return Device.dwTimeGlobal; }
u32 script_time_global_async() { return Device.TimerAsync_MMT(); }

SCRIPT_EXPORT(Device, (),
{
	module(luaState)
	[
		def("time_global", &script_time_global),
		def("time_global_async", &script_time_global_async),
		def("device", &get_device),
		def("is_enough_address_space_available", &is_enough_address_space_available)
	];
});

CLoadScreenRenderer::CLoadScreenRenderer() : b_registered(false), b_need_user_input(false) {}
void CLoadScreenRenderer::start(bool b_user_input)
{
	Device.seqRender.Add(this, 0);
	b_registered = true;
	b_need_user_input = b_user_input;
}

void CLoadScreenRenderer::stop()
{
	if (!b_registered)
		return;
	Device.seqRender.Remove(this);
	pApp->DestroyLoadingScreen();
	b_registered = false;
	b_need_user_input = false;
}

void CLoadScreenRenderer::OnRender() { pApp->load_draw_internal(); }

bool CRenderDevice::CSecondVPParams::IsSVPFrame() //--#SM+#-- +SecondVP+
{
	return IsSVPActive() && Device.dwFrame % frameDelay == 0;
}
void CRenderDevice::time_factor(const float& time_factor)
{
	Timer.time_factor(time_factor);
	TimerGlobal.time_factor(time_factor);
	psSoundTimeFactor = time_factor; //--#SM+#--
}
