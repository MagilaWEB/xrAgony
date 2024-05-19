#pragma once
#ifndef xr_device
#define xr_device

// Note:
// ZNear - always 0.0f
// ZFar - always 1.0f

// class ENGINE_API CResourceManager;
// class ENGINE_API CGammaControl;

#include "pure.h"

#include "xrCore/FTimer.h"
#include "Stats.h"
#include "xrCommon/xr_list.h"
#include "xrCore/fastdelegate.h"
#include "xrCore/ModuleLookup.hpp"

extern u32 g_dwFPSlimit;

#define VIEWPORT_NEAR 0.1f

#define DEVICE_RESET_PRECACHE_FRAME_COUNT 10

#include "Include/xrRender/FactoryPtr.h"
#include "Render.h"

#pragma pack(push, 4)

#pragma pack(pop)

class CRenderDeviceBase
{
public:
	// Rendering resolution
	u32 dwWidth;
	u32 dwHeight;

	// Real application window resolution
	RECT m_rcWindowBounds;

	// Real game window resolution 
	RECT m_rcWindowClient;

	u32 dwPrecacheFrame;
	BOOL b_is_Ready;
	BOOL b_is_Active;

	// Engine flow-control
	u32 dwFrame;

	float fTimeDelta;
	float fTimeGlobal;
	u32 dwTimeDelta;
	u32 dwTimeGlobal;
	u32 dwTimeContinual;

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

	float fFOV;
	float fASPECT;

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

	virtual const RenderDeviceStatictics& GetStats() const = 0;

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
};

// refs
class ENGINE_API CRenderDevice final : public CRenderDeviceBase
{
public:

	struct ENGINE_API CScopeVP final
	{
		bool m_bIsActive		{ false };	// Флаг активации рендера во второй вьюпорт
		bool m_bStartRender		{ false };	// Флаг начала рендеринга.
		
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
	};

private:
	MSG msg;

	// Main objects used for creating and rendering the 3D scene
	u64 m_dwWindowStyle;
	CTimer TimerMM;
	RenderDeviceStatictics stats;

	bool b_restart{ false };

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
	xr_list<fastdelegate::FastDelegate<void()>>	seqParallel;
	xr_list<fastdelegate::FastDelegate<void()>>	seqParallel2;
	xr_list<std::function<void()>>			functionPointer;

	CScopeVP m_ScopeVP;

	Fmatrix mInvFullTransform;

	CRenderDevice() : m_dwWindowStyle(0), fWidth_2(0), fHeight_2(0)
	{
		m_hWnd = NULL;
		b_is_Active = FALSE;
		b_is_Ready = FALSE;
		Timer.Start();
		m_bNearer = FALSE;
		m_ScopeVP.SetSVPActive(false);
	};

	~CRenderDevice()
	{
		Statistic = nullptr;
	}

	void Pause(BOOL bOn, BOOL bTimer, BOOL bSound, LPCSTR reason);
	BOOL Paused();

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
	IC CTimer_paused* GetTimerGlobal() { return &TimerGlobal; }
	u32 TimerAsync() { return TimerGlobal.GetElapsed_ms(); }
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

	void time_factor(const float& time_factor); //--#SM+#--

	IC const float time_factor() const
	{
		VERIFY(Timer.time_factor() == TimerGlobal.time_factor());
		return (Timer.time_factor());
	}

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

	bool on_message(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT& result);

private:
	void ProcessPriority();
	void CalcFrameStats();
	void OnFrame();
	void OnFrame2();
	void d_Render();
	void d_SVPRender();
	void GlobalUpdate();
	void FpsCalc();

	void message_loop();
	virtual void AddSeqFrame(pureFrame* f, bool mt);
	virtual void RemoveSeqFrame(pureFrame* f);

	ICF const bool RunFunctionPointer()
	{
		bool fPointerStart;
		while (fPointerStart = !functionPointer.empty())
		{
			functionPointer.front()();
			functionPointer.pop_front();
		}

		return fPointerStart;
	}
};

extern ENGINE_API CRenderDevice Device;
extern ENGINE_API bool g_bBenchmark;

typedef fastdelegate::FastDelegate<bool()> LOADING_EVENT;
extern ENGINE_API xr_list<LOADING_EVENT> g_loading_events;

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

#endif
