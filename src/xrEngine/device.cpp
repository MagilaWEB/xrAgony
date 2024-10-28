#include "stdafx.h"
#include "xrCDB/Frustum.h"

#include "x_ray.h"
#include "Render.h"

// must be defined before include of FS_impl.h
#define INCLUDE_FROM_ENGINE
#include "xrCore/FS_impl.h"

#include "xrSASH.h"
#include "IGame_Persistent.h"
#include "xrScriptEngine/ScriptExporter.hpp"
#include "xrScriptEngine/script_process.hpp"
#include "xrScriptEngine/script_engine.hpp"
#include "xr_input.h"
#include "splash.h"

using namespace std;

ENGINE_API CRenderDevice Device;
ENGINE_API CLoadScreenRenderer load_screen_renderer;
ENGINE_API xr_list<fastdelegate::FastDelegate<bool()>> g_loading_events;
ENGINE_API BOOL g_bRendering = FALSE;

extern int g_process_priority;
extern int g_GlobalFPSlimit;	//Limit fps global.
extern int g_PausedFPSlimit;	//Limit fps to Pause.
extern int g_MainFPSlimit;		//Limit fps to main.
extern int r_scope_fps_limit;

void CRenderDevice::Run()
{
	Log("Starting engine...");

	// Startup timers and calculate timer delta
	dwTimeGlobal = 0;

	seqAppStart.Process();

	mt_global_update.Init([this]() { GlobalUpdate(); });

	mt_frame.Init([this]() { OnFrame(); }, xrThread::sParalelRender);
	mt_frame2.Init([this]() { OnFrame2(); }, xrThread::sParalelRender);

	// Message cycle
	::Render->ClearTarget();
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
	if (::Render->GetDeviceState() == DeviceState::Lost)
	{
		// If the device was lost, do not render until we get it back
		this_thread::sleep_for(33ms);
		return FALSE;
	}

	if (::Render->GetDeviceState() == DeviceState::NeedReset)
		Reset();

	::Render->Begin();
	g_bRendering = TRUE;

	return TRUE;
}

void CRenderDevice::Clear() { ::Render->Clear(); }

bool CRenderDevice::ActiveMain()
{
	return b_is_Active.load() && g_pGamePersistent && g_pGamePersistent->m_pMainMenu && g_pGamePersistent->m_pMainMenu->IsActive();
}

void CRenderDevice::End(void)
{
	if (dwPrecacheFrame)
	{
		dwPrecacheFrame--;
		if (!dwPrecacheFrame)
		{
			::Render->updateGamma();
			::Render->ResourcesDestroyNecessaryTextures();
			Memory.mem_compact();
			Msg("* MEMORY USAGE: %d K", Memory.mem_usage() / 1024);
			Msg("* End of synchronization A[%d] R[%d]", b_is_Active.load(), b_is_Ready.load());
#ifdef FIND_CHUNK_BENCHMARK_ENABLE
			g_find_chunk_counter.flush();
#endif

			//if (g_pGamePersistent->GameType() == 1) // haCk
			//{
			//	WINDOWINFO wi;
			//	GetWindowInfo(m_hWnd, &wi);
			//	if (wi.dwWindowStatus != WS_ACTIVECAPTION)
			//		Pause(TRUE, TRUE, FALSE, "application start");
			//}
		}
	}

	g_bRendering = FALSE;
	// end scene
	// Present goes here, so call OA Frame end.
	if (g_SASH.IsBenchmarkRunning())
		g_SASH.DisplayFrame(Device.fTimeGlobal);
	::Render->End();
}

#include "IGame_Level.h"
void CRenderDevice::PreCache(u32 amount, bool b_draw_loadscreen, bool b_wait_user_input)
{
	if (::Render->GetForceGPU_REF())
		amount = 0;

	dwPrecacheFrame = dwPrecacheTotal = amount;
	if (amount && b_draw_loadscreen && !load_screen_renderer.b_registered)
	{
		load_screen_renderer.start(b_wait_user_input);
	}
}

void CRenderDevice::CalcFrameStats()
{
	stats.RenderTotal.FrameEnd();
	// calc FPS & TPS
	if (fTimeDelta > EPS_S)
	{
		float fps = 1.f / fTimeDelta;
		float fOne = 0.3f;
		float fInv = 1.0f - fOne;
		stats.fFPS = fInv * stats.fFPS + fOne * fps;

		LIMIT_UPDATE(fps_update, 0.3,
			FPS = u16(FPS + stats.fFPS)/2;
		)

		if (stats.RenderTotal.result > EPS_S)
		{
			u32 renderedPolys = ::Render->GetCacheStatPolys();
			stats.fTPS = fInv * stats.fTPS + fOne * float(renderedPolys) / (stats.RenderTotal.result * 1000.f);
			stats.fRFPS = fInv * stats.fRFPS + fOne * 1000.f / stats.RenderTotal.result;
		}
	}
	stats.RenderTotal.FrameStart();
}

void CRenderDevice::OnFrame()
{
	::Sound->update(Device.vCameraPosition, Device.vCameraDirection, Device.vCameraTop);

	if (Device.Paused() == FALSE && g_loading_events.empty())
	{
		if (g_pGameLevel && g_pGameLevel->bReady)
		{
			LIMIT_UPDATE_FPS_CODE(UniqueCallLimit, 60, ::ScriptEngine->UpdateUniqueCall();)
				::ScriptEngine->script_process(ScriptProcessor::Level)->update();
			lua_gc(::ScriptEngine->lua(), LUA_GCSTEP, 10);
		}
	}

	while (!seqParallel.empty())
	{
		seqParallel.front()();
		seqParallel.pop_front();
	}

	seqFrameMT.Process();
}

void CRenderDevice::OnFrame2()
{
	if (g_loading_events.empty())
	{
		while (!seqParallel2.empty())
		{
			seqParallel2.front()();
			seqParallel2.pop_front();
		}
	}
}

void CRenderDevice::d_Render()
{
	// all rendering is done here
	CStatTimer renderTotalReal;
	renderTotalReal.FrameStart();
	renderTotalReal.Begin();
	::Render->ResourcesWaitTexturesLoad();
	if (b_is_Active.load() && Begin())
	{
		seqRender.Process();
		CalcFrameStats();
		Statistic->Show();
		End(); // Present goes here
	}
	renderTotalReal.End();
	renderTotalReal.FrameEnd();
	stats.RenderTotal.accum = renderTotalReal.accum;
}

void CRenderDevice::d_SVPRender()
{
	if (m_ScopeVP.IsSVPActive() && !ActiveMain())
	{
		dwFrame++;
		Core.dwFrame = dwFrame;
		m_ScopeVP.SetRender(true);

		if (g_pGameLevel)
			g_pGameLevel->ActorApplyCamera();

		// Matrices
		mInvView.invert(mView);
		mFullTransform.mul(mProject, mView);
		::Render->SetCacheXform(mView, mProject);
		mInvFullTransform.invert_44(mFullTransform);

		vCameraPositionSaved = vCameraPosition;

		::Render->Begin();

		g_bRendering = TRUE;
		seqRender.Process();
		g_bRendering = FALSE;

		::Render->End();

		m_ScopeVP.SetRender(false);
	}
}

void CRenderDevice::GlobalUpdate()
{
	xrCriticalSection::raii mt{ ResetRender };
	if ((!RunFunctionPointer()) && (!b_restart))
	{
		ProcessPriority();

		if (psDeviceFlags.test(rsStatistic))
			g_bEnableStatGather = TRUE; // XXX: why not use either rsStatistic or g_bEnableStatGather?
		else
			g_bEnableStatGather = FALSE;

		const u32 limit = (ActiveMain() || IsLoadingScreen()) ? g_MainFPSlimit : Paused() ? g_PausedFPSlimit : g_GlobalFPSlimit;
		// FPS Lock
		if (limit > 0)
		{
			static CTimer dwTime;
			const float updateDelta = 1000.f / limit;
			const float elapsed = dwTime.GetElapsed_sec() * 1000;
			if (elapsed < updateDelta)
				this_thread::sleep_for(chrono::milliseconds(long long(updateDelta - elapsed)));

			dwTime.Start();
		}

		if (b_is_Active.load() && ::Render->GetDeviceState() != DeviceState::Lost)
		{
			if (r_scope_fps_limit)
				LIMIT_UPDATE_FPS_CODE(SecondViewportRenderFps, r_scope_fps_limit, d_SVPRender();)
			else
				d_SVPRender();
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
		mInvView.invert(mView);
		mFullTransform.mul(mProject, mView);
		::Render->SetCacheXform(mView, mProject);
		mInvFullTransform.invert_44(mFullTransform);

		ViewFromMatrix.CreateFromMatrix(mFullTransform, FRUSTUM_P_LRTB + FRUSTUM_P_FAR);

		vCameraPositionSaved = vCameraPosition;
		vCameraDirectionSaved = vCameraDirection;
		vCameraTopSaved = vCameraTop;
		vCameraRightSaved = vCameraRight;

		mFullTransformSaved = mFullTransform;
		mViewSaved = mView;
		mProjectSaved = mProject;

		xrThread::StartGlobal(xrThread::sParalelRender);

		d_Render();

		xrThread::WaitGlobal();
	}
}

void CRenderDevice::message_loop()
{
	PeekMessage(&msg, nullptr, 0U, 0U, PM_NOREMOVE);
	while (msg.message != WM_QUIT)
	{
		if (PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
			continue;
		}

		extern ENGINE_API u32 state_screen_mode;
		if (state_screen_mode != 1)
		{
			RECT rect;
			GetClientRect(m_hWnd, &rect);

			POINT ul;
			ul.x = rect.left;
			ul.y = rect.top;

			POINT lr;
			lr.x = rect.right;
			lr.y = rect.bottom;

			MapWindowPoints(m_hWnd, nullptr, &ul, 1);
			MapWindowPoints(m_hWnd, nullptr, &lr, 1);

			rect.left = ul.x;
			rect.top = ul.y;

			rect.right = lr.x;
			rect.bottom = lr.y;

			static LPPOINT point = new POINT();
			::GetCursorPos(point);

			if (point->x < rect.left || point->x > rect.right || point->y < rect.top || point->y > rect.bottom)
			{
				if (b_cursor_on_window.load() && !AltTab())
				{
					b_cursor_on_window.store(false);
					pInput->ShowCursor(true);
					::ShowWindow(m_hWnd, SW_FORCEMINIMIZE);
					::ShowWindow(m_hWnd, SHOW_OPENNOACTIVATE);
					::SetCursorPos(point->x, point->y);
				}
			}
			else if (!b_cursor_on_window.load())
				b_cursor_on_window.store(true);
		}

		if ((!IsQUIT()) && b_restart)
			ResetStart();

		this_thread::sleep_for(40ms);
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

	Timer.Start(); // previous frame

	// floating point
	float che_fTimeDelta = fTimeGlobal;
	fTimeGlobal = TimerGlobal.GetElapsed_sec(); //float(qTime)*CPU::cycles2seconds;
	fTimeDelta = fTimeGlobal - che_fTimeDelta;

	if (fTimeDelta > .1f)
		fTimeDelta = .1f; // limit to 15fps minimum

	if (fTimeDelta <= 0.f)
		fTimeDelta = EPS_S + EPS_S; // limit to 15fps minimum

	if (Paused())
		fTimeDelta = 0.0f;
	// integer
	u32 _old_global = dwTimeGlobal;
	dwTimeGlobal = TimerGlobal.GetElapsed_ms();
	dwTimeDelta = dwTimeGlobal - _old_global;

	xrThread::StartGlobal(xrThread::sParalelFrame);
	// Frame move
	stats.EngineTotal.FrameStart();
	stats.EngineTotal.Begin();

	seqFrame.Process();

	stats.EngineTotal.End();
	stats.EngineTotal.FrameEnd();
}

ENGINE_API BOOL bShowPauseString = TRUE;

void CRenderDevice::Pause(BOOL bOn, BOOL bTimer, BOOL bSound, LPCSTR reason)
{
	static int snd_emitters_ = -1;
	if (g_bBenchmark)
		return;

	if (bOn)
	{
#ifdef DEBUG
		if (!Paused())
			bShowPauseString = !xr_strcmp(reason, "li_pause_key_no_clip");
#endif // DEBUG

		if (bTimer && (!g_pGamePersistent || g_pGamePersistent->CanBePaused()))
		{
			g_pauseMngr().Pause(TRUE);
#ifdef DEBUG
			if (!xr_strcmp(reason, "li_pause_key_no_clip"))
				TimerGlobal.Pause(FALSE);
#endif
}
		if (bSound && ::Sound)
			snd_emitters_ = ::Sound->pause_emitters(true);
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
				snd_emitters_ = ::Sound->pause_emitters(false);
			else
			{
#ifdef DEBUG
				Log("::Sound->pause_emitters underflow");
#endif
		}
	}
		}
	}

BOOL CRenderDevice::Paused() { return g_pauseMngr().Paused(); }

const bool CRenderDevice::IsLoadingScreen()
{
	return pApp->IsLoadingScreen();
}

const bool CRenderDevice::IsLoadingProsses()
{
	return !g_loading_events.empty();
}

void CRenderDevice::OnWM_Activate(WPARAM wParam, LPARAM /*lParam*/)
{
	u16 fActive = LOWORD(wParam);
	const bool fMinimized = (bool)HIWORD(wParam);
	const bool isWndActive = fActive != WA_INACTIVE && !fMinimized;

	if (isWndActive != b_is_Active.load())
	{
		b_is_Active.store(isWndActive);

		if (isWndActive)
		{
			//Reset();
			ShowWindow(m_hWnd, SW_SHOWNORMAL);
			xrThread::GlobalState(xrThread::dsOK);
		}

		functionPointer.push_back([&]() {
			seqParallel.clear();
			seqParallel2.clear();

			if (b_is_Active.load())
			{
				xrThread::GlobalState(xrThread::dsOK);
				seqAppActivate.Process();
				app_inactive_time += TimerMM.GetElapsed_ms() - app_inactive_time_start;
			}
			else
			{
				xrThread::GlobalState(xrThread::dsSleep);
				app_inactive_time_start = TimerMM.GetElapsed_ms();
				seqAppDeactivate.Process();
			}
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

SCRIPT_EXPORT(Device, (),
	{
		module(luaState)
		[
			def("time_global", &script_time_global),
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

	if (pApp->LoadScreen())
		pApp->LoadScreen()->ChangeVisibility(false);

	Device.seqRender.Remove(this);

	b_registered = false;
	b_need_user_input = false;
}

void CLoadScreenRenderer::OnRender()
{
	pApp->load_draw_internal();
}

void CRenderDevice::time_factor(const float& time_factor)
{
	Timer.time_factor(time_factor);
	TimerGlobal.time_factor(time_factor);
	psSoundTimeFactor = time_factor; //--#SM+#--
}
