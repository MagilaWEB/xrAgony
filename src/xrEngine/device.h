#pragma once
#include "IDevice.hpp"
#include "pure.h"
#include "xrCore/FTimer.h"
#include "Stats.h"
#include "xrCommon/xr_list.h"
#include "xrCore/fastdelegate.h"
#include "xrCore/ModuleLookup.hpp"

extern ENGINE_API float VIEWPORT_NEAR;
extern ENGINE_API float VIEWPORT_NEAR_HUD;

#define DEVICE_RESET_PRECACHE_FRAME_COUNT 10

#include "Include/xrRender/FactoryPtr.h"
#include "Render.h"

struct RenderDeviceStatictics final
{
	CStatTimer RenderTotal; // pureRender
	CStatTimer EngineTotal; // pureFrame
	float fFPS, fRFPS, fTPS; // FPS, RenderFPS, TPS

	RenderDeviceStatictics()
	{
		fFPS = 30.f;
		fRFPS = 30.f;
		fTPS = 0;
	}
};

// refs
class ENGINE_API CRenderDevice final : public IRenderDevice
{
	// Engine flow-control
	size_t dwFrame			{ 0 };
	float m_time_delta_sec	{ 0.f };
	float m_time_global_sec	{ 0.f };
	size_t m_time_delta_ms	{ 0 };
	size_t m_time_global_ms	{ 0 };
	size_t dwTimeContinual	{ 0 };

public:
	constexpr static float UI_BASE_WIDTH = 1024.f;
	constexpr static float UI_BASE_HEIGHT = 768.f;

	// Rendering resolution
	u32 dwWidth;
	u32 dwHeight;

	float aspect_ratio{ 1.f };
	float screen_magnitude{ 1.f };

	// Real application window resolution
	RECT m_rcWindowBounds;

	// Real game window resolution 
	RECT m_rcWindowClient;

	u32 dwPrecacheFrame;
	std::atomic_bool b_is_Ready;
	std::atomic_bool b_is_Active;

	// Engine flow-control
	Fvector vCameraPosition;
	Fvector vCameraDirection;
	Fvector vCameraTop;
	Fvector vCameraRight;

	Fmatrix mView;
	Fmatrix mInvView;
	Fmatrix mProject;
	Fmatrix mFullTransform;

	// Copies of corresponding members. Used for synchronization.
	Fvector vCameraPositionSaved;
	Fvector vCameraDirectionSaved;
	Fvector vCameraTopSaved;
	Fvector vCameraRightSaved;

	Fmatrix mViewSaved;
	Fmatrix mProjectSaved;
	Fmatrix mFullTransformSaved;

	CFrustum ViewFromMatrix;

	float fFOV;
	float fASPECT;
	float iZoomSqr;

	float gFOV = 75.f;
	float gAimFOV = 36.f;
	float gAimFOVTan = 0.72654252800536088589546675748062f;

protected:
	CTimer_paused Timer;
	CTimer_paused TimerGlobal;
	CStats* Statistic;

public:
	// Registrators
	MessageRegistry<pureRender> seqRender;
	MessageRegistry<pureAppActivate> seqAppActivate;
	MessageRegistry<pureAppDeactivate> seqAppDeactivate;
	MessageRegistry<pureAppStart> seqAppStart;
	MessageRegistry<pureAppEnd> seqAppEnd;
	MessageRegistry<pureFrame> seqFrame;
	MessageRegistry<pureScreenResolutionChanged> seqResolutionChanged;

	HWND m_hWnd;
public:

	struct ENGINE_API CScopeVP final
	{
		bool m_bIsActive		{ false };	// Флаг активации рендера во второй вьюпорт
		bool m_bStartRender		{ false };	// Флаг начала рендеринга.
		float m_zoom			= 1.f;
		float m_izoom_sqr		= 1.f;
		float m_fov				= 1.f;
		
	public:
		Fvector		m_vPosition;
		Fvector		m_vDirection;
		Fvector		m_vNormal;
		Fvector		m_vRight;

		IC void SetRender(bool bState)
		{
			m_bStartRender = bState;
		}

		IC bool IsSVPActive()
		{
			return m_bIsActive;
		}

		IC void SetSVPActive(bool bState)
		{
			m_bIsActive = bState;
		}

		bool IsSVPRender()
		{
			return IsSVPActive() && m_bStartRender;
		}

		float getIZoomSqr() const { return m_izoom_sqr; }
		float getZoom() const { return m_zoom; }
		float getFOV() const { return m_fov; }

		void setZoom(float val) { m_zoom = val; m_izoom_sqr = _sqr(1.f / val); }
		void setFOV(float val) { m_fov = val; }
	};

private:
	MSG msg;

	// Main objects used for creating and rendering the 3D scene
	DWORD m_dwWindowStyle;
	CTimer TimerMM;
	RenderDeviceStatictics stats;

	std::atomic_bool b_restart{ false };
	std::atomic_bool b_cursor_on_window{ false };
	std::atomic_bool b_alt_tab{ false };

	void _SetupStates();

public:
	u16 FPS = 30;

	u32 dwPrecacheTotal;
	float fWidth_2, fHeight_2;
	void OnWM_Activate(WPARAM wParam, LPARAM lParam);

	BOOL m_bNearer;
	void SetNearer(BOOL enabled)
	{
		if (enabled && !m_bNearer)
		{
			m_bNearer = TRUE;
			mProject._43 -= EPS_L;
		}
		else if (!enabled && m_bNearer)
		{
			m_bNearer = FALSE;
			mProject._43 += EPS_L;
		}
		::Render->SetCacheXform(mView, mProject);
	}

	void DumpResourcesMemoryUsage() { ::Render->ResourcesDumpMemoryUsage(); }

	MessageRegistry<pureFrame>				seqFrameMT;
	MessageRegistry<pureDeviceReset>		seqDeviceReset;
	xr_list<fastdelegate::FastDelegate<>>	seqParallel;
	xr_list<fastdelegate::FastDelegate<>>	seqParallel2;
	xr_list<fastdelegate::FastDelegate<>>	segParallelLoad;
	xr_list<std::function<void()>>			functionPointer;

	CScopeVP m_ScopeVP;

	Fmatrix mInvFullTransform;

	CRenderDevice() : m_dwWindowStyle(0), fWidth_2(0), fHeight_2(0)
	{
		::IDevice = this;
		m_hWnd = nullptr;
		b_is_Active.store(false);
		b_is_Ready.store(false);
		Timer.Start();
		m_bNearer = FALSE;
		m_ScopeVP.SetSVPActive(false);
		xrThread::id_main_thread(GetCurrentThreadId());
	};

	~CRenderDevice()
	{
		Statistic = nullptr;
	}

	void Pause(BOOL bOn, BOOL bTimer, BOOL bSound, LPCSTR reason);
	bool Paused() const;

private:
	//Threding
	xrThread mt_global_update		{ "GlobalUpdate", true };
	xrThread mt_frame				{ "Frame", true, true };
	xrThread mt_frame2				{ "Frame2", true, true };
	xrCriticalSection				ResetRender;

public:
	// Scene control
	void PreCache(u32 amount, bool b_draw_loadscreen, bool b_wait_user_input);
	BOOL Begin();
	void Clear();
	bool ActiveMain();
	void End();
	void FrameMove();

	void overdrawBegin();
	void overdrawEnd();

	// Mode control
	void DumpFlags();
	IC CTimer_paused* GetTimerGlobal() override { return &TimerGlobal; }
	// Creation & Destroying
	void Create();
	void Run();
	void Destroy();
	void Reset();
	bool IsReset() const;
	void ResetStart();
	void Initialize();

	const RenderDeviceStatictics& GetStats() const override { return stats; }
	void DumpStatistics(class IGameFont& font, class IPerformanceAlert* alert);

	IC void incrementFrame() override { ++dwFrame; };
	size_t getFrame() const override;
	size_t TimeGlobal_ms() const override;
	size_t TimeDelta_ms() const override;
	float TimeDelta_sec() const override;
	float TimeGlobal_sec() const override;
	size_t TimeContinual() const override;
	void time_factor(float time_factor) override;
	float time_factor() const override;
	size_t TimerAsync_ms() const override;
	float TimerAsync_sec() const override;

	//Parallel execution in conjunction with the render.
	template<class  PAR, class PAR_X>
	ICF void add_parallel(PAR* pr_1, fastdelegate::FastDelegate<void()>::Parameters(PAR_X::* pr_2)())
	{
		const fastdelegate::FastDelegate<void()>& _delegate = fastdelegate::FastDelegate<void()>(pr_1, pr_2);
		if (!seqParallel.contain(_delegate))
			seqParallel.push_back(_delegate);
	}

	template<class  PAR, class PAR_X>
	ICF void remove_parallel(PAR* pr_1, fastdelegate::FastDelegate<void()>::Parameters(PAR_X::* pr_2)())
	{
		seqParallel.erase(std::remove(seqParallel.begin(), seqParallel.end(), fastdelegate::FastDelegate<void()>(pr_1, pr_2)), seqParallel.end());
	}

	//Parallel execution in conjunction with the render.
	template<class  PAR, class PAR_X>
	ICF void add_parallel2(PAR* pr_1, fastdelegate::FastDelegate<void()>::Parameters(PAR_X::* pr_2)())
	{
		const fastdelegate::FastDelegate<void()>& _delegate = fastdelegate::FastDelegate<void()>(pr_1, pr_2);
		if (!seqParallel2.contain(_delegate))
			seqParallel2.push_back(_delegate);
	}

	template<class  PAR, class PAR_X>
	ICF void remove_parallel2(PAR* pr_1, fastdelegate::FastDelegate<void()>::Parameters(PAR_X::* pr_2)())
	{
		seqParallel2.erase(std::remove(seqParallel2.begin(), seqParallel2.end(), fastdelegate::FastDelegate<void()>(pr_1, pr_2)), seqParallel2.end());
	}

	ICF void removes_parallel()
	{
		seqParallel.clear();
	}

	ICF void EventMessage(UINT Msg, WPARAM wParam, LPARAM lParam)
	{
		PostThreadMessage(xrThread::get_main_id(), Msg, wParam, lParam);
	}

	ICF const bool IsQUIT() const
	{
		return msg.message == WM_QUIT;
	}

	ICF const bool AltTab()
	{
		return b_alt_tab.load();
	};

	ICF void AltTab(bool b_alt_tab_)
	{
		return b_alt_tab.store(b_alt_tab_);
	};

	bool IsLoadingScreen() const;
	bool IsLoadingProsses() const;

	bool on_message(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT& result);

	void AddSeqFrame(pureFrame* f, bool mt);
	void RemoveSeqFrame(pureFrame* f);

private:
	void ProcessPriority();
	void CalcFrameStats();
	void OnFrame();
	void OnFrame2();
	void d_Render();
	void d_SVPRender();
	void GlobalUpdate();
	
	void message_loop();

	ICF const bool RunFunctionPointer()
	{
		bool fPointerStart;
		while ((fPointerStart = !functionPointer.empty()))
		{
			functionPointer.front()();
			functionPointer.pop_front();
		}

		return fPointerStart;
	}

public:
	bool								isLevelReady							() const;
	bool 								isGameProcess							() const override;
};

extern ENGINE_API CRenderDevice Device;
extern ENGINE_API bool g_bBenchmark;

extern ENGINE_API xr_list<fastdelegate::FastDelegate<bool()>> g_loading_events;

class ENGINE_API CLoadScreenRenderer : public pureRender
{
public:
	CLoadScreenRenderer();
	void start(bool b_user_input);
	void stop();
	virtual void OnRender();
	bool IsActive() const { return b_registered; }

	bool b_registered;
	bool b_need_user_input;
};
extern ENGINE_API CLoadScreenRenderer load_screen_renderer;