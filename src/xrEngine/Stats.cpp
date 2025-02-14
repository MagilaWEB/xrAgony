#include "stdafx.h"
#include "GameFont.h"
#include "Stats.h"

#include "IGame_Level.h"
#include "IGame_Persistent.h"
#include "Render.h"
#include "xr_object.h"
#include "xrCDB/ISpatial.h"

#include "Include/xrRender/DrawUtils.h"
#include "PerformanceAlert.hpp"
#include "xr_input.h"
#include "xrCore/cdecl_cast.hpp"
#include "xrPhysics/IPHWorld.h"

int g_ErrorLineCount = 15;
Flags32 g_stats_flags = { 0 };

class optimizer
{
	float average_;
	BOOL enabled_;

public:
	optimizer()
	{
		average_ = 30.f;
		//  enabled_ = TRUE;
		//  disable ();
		// because Engine is not exist
		enabled_ = FALSE;
	}

	BOOL enabled() { return enabled_; }
	void enable()
	{
		if (!enabled_)
		{
			Engine.External.tune_resume();
			enabled_ = TRUE;
		}
	}
	void disable()
	{
		if (enabled_)
		{
			Engine.External.tune_pause();
			enabled_ = FALSE;
		}
	}
	void update(float value)
	{
		if (value < average_ * 0.7f)
		{
			// 25% deviation
			enable();
		}
		else
		{
			disable();
		};
		average_ = 0.99f * average_ + 0.01f * value;
	};
};
static optimizer vtune;

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
BOOL g_bDisableRedText = FALSE;
CStats::CStats()
{
	statsFont = nullptr;
	fMem_calls = 0;
	Device.seqRender.Add(this, REG_PRIORITY_LOW - 1000);
}

CStats::~CStats()
{
	Device.seqRender.Remove(this);
	xr_delete(statsFont);
}

static void DumpSpatialStatistics(IGameFont& font, IPerformanceAlert* alert, ISpatial_DB& db, float engineTotal)
{
#ifdef DEBUG
	auto& stats = db.Stats;
	stats.FrameEnd();
#define PPP(a) (100.f * float(a) / engineTotal)
	font.OutNext("%s:", db.Name);
	font.OutNext("- query:	  %.2fms, %u", stats.Query.result, stats.Query.count);
	font.OutNext("- nodes/obj:  %u/%u", stats.NodeCount, stats.ObjectCount);
	font.OutNext("- insert:	 %.2fms, %2.1f%%", stats.Insert.result, PPP(stats.Insert.result));
	font.OutNext("- remove:	 %.2fms, %2.1f%%", stats.Remove.result, PPP(stats.Remove.result));
#undef PPP
	stats.FrameStart();
#endif
}

void CStats::Show()
{
	float memCalls = float(Memory.stat_calls);
	if (memCalls > fMem_calls)
		fMem_calls = memCalls;
	else
		fMem_calls = 0.9f * fMem_calls + 0.1f * memCalls;
	Memory.stat_calls = 0;

	auto& font = *statsFont;
	auto engineTotal = Device.GetStats().EngineTotal.result;
	PerformanceAlert alertInstance(font.GetHeight(), { 300, 300 });
	auto alertPtr = g_bDisableRedText ? nullptr : &alertInstance;
	if (vtune.enabled())
	{
		float sz = font.GetHeight();
		font.SetHeightI(0.02f);
		font.SetColor(0xFFFF0000);
		font.OutSet(Device.dwWidth / 2.0f + (font.SizeOf_("--= tune =--") / 2.0f), Device.dwHeight / 2.0f);
		font.OutNext("--= tune =--");
		font.SetHeight(sz);
	}
	// Show them
	if (psDeviceFlags.test(rsStatistic))
	{
		font.SetColor(0xFFFFFFFF);
		font.OutSet(0, 0);
#if defined(FS_DEBUG)
		font.OutNext("Mapped:		%d", g_file_mapped_memory);
#endif
		Device.DumpStatistics(font, alertPtr);
		font.OutNext("Memory:		%2.2f", fMem_calls);
		if (g_pGameLevel)
			g_pGameLevel->DumpStatistics(font, alertPtr);
		Engine.Sheduler.DumpStatistics(font, alertPtr);
		g_pGamePersistent->DumpStatistics(font, alertPtr);
		DumpSpatialStatistics(font, alertPtr, *g_SpatialSpace, engineTotal);
		DumpSpatialStatistics(font, alertPtr, *g_SpatialSpacePhysic, engineTotal);
		if (physics_world())
			physics_world()->DumpStatistics(font, alertPtr);
		font.OutSet(200, 0);
		::Render->DumpStatistics(font, alertPtr);
		font.OutSkip();
		::Sound->DumpStatistics(font, alertPtr);
		font.OutSkip();
		pInput->DumpStatistics(font, alertPtr);
		font.OutSkip();
		font.OutNext("QPC: %u", CPU::qpc_counter);
		CPU::qpc_counter = 0;
	}
	else
	{

		if (psDeviceFlags.test(rsXrThreadStatistic))
			ShowXrThread();
	}

	if (psDeviceFlags.test(rsCameraPos))
	{
		float refHeight = font.GetHeight();
		font.SetHeightI(0.02f);
		font.SetColor(0xffffffff);
		font.Out(10, 600, "CAMERA POSITION:  [%3.2f,%3.2f,%3.2f]", VPUSH(Device.vCameraPosition));
		font.SetHeight(refHeight);
	}
#ifdef DEBUG
	if (!g_bDisableRedText && errors.size())
	{
		font.SetColor(color_rgba(255, 16, 16, 191));
		font.OutSet(400, 0);
		for (u32 it = (u32)_max(int(0), (int)errors.size() - g_ErrorLineCount); it < errors.size(); it++)
			font.OutNext("%s", errors[it].c_str());
	}
#endif
	font.OnRender();

	if (psDeviceFlags.test(rsShowFPS))
	{
		const u32 fps = Device.FPS;
		if (fps >= 121)
			fpsFont->SetColor(color_rgba(255, 0, 255, 255));
		else if (fps >= 60)
			fpsFont->SetColor(color_rgba(0, 255, 0, 255));
		else if (fps > 30)
			fpsFont->SetColor(color_rgba(255, 255, 0, 255));
		else
			fpsFont->SetColor(color_rgba(255, 51, 51, 255));

		fpsFont->Out(float(Device.dwWidth - 40), 5, "%3d", fps);
		fpsFont->OnRender();
	}
}

void CStats::ShowXrThread()
{
	pFontXrThread->OutSet(3.f, 1.f);
	xrThread::ForThreads([&](xrThread* thread)
	{
		if (thread->DebugInfo())
		{
			float color_result = thread->ms_time/15;
			clamp(color_result, 0.f, 1.f);

			const float revers_color_result = 1.f - color_result;

			pFontXrThread->SetColor(color_rgba(u32(255 * color_result), u32(255 * revers_color_result), 0, 255));

			pFontXrThread->OutNext("Thread: Name[%s], ID[%d], time[%0.4f ms]", thread->Name(), thread->ID(), thread->ms_time);
			pFontXrThread->OnRender();
		}
		return false;
	});
}


void CStats::OnDeviceCreate()
{
	g_bDisableRedText = !!strstr(Core.Params, "-xclsx");

	statsFont = new CGameFont("stat_font", CGameFont::fsDeviceIndependent);

	fpsFont = new CGameFont("hud_font_di", CGameFont::fsDeviceIndependent);
	fpsFont->SetHeightI(0.03f);
	fpsFont->SetColor(color_rgba(250, 250, 15, 180));

	pFontCPU = new CGameFont("hud_font_di", CGameFont::fsDeviceIndependent);
	pFontCPU->SetHeightI(0.018f);

	pFontXrThread = new CGameFont("hud_font_di", CGameFont::fsDeviceIndependent);
	pFontXrThread->SetHeightI(0.02f);
	pFontXrThread->SetColor(color_rgba(255, 255, 0, 255));

#ifdef DEBUG
	if (!g_bDisableRedText)
	{
		auto cb = [](void* ctx, const char* s) {
			auto& stats = *static_cast<CStats*>(ctx);
			stats.FilteredLog(s);
		};
		SetLogCB(LogCallback(cdecl_cast(cb), this));
	}
#endif
}

void CStats::OnDeviceDestroy()
{
	SetLogCB(nullptr);
	xr_delete(statsFont);
	xr_delete(fpsFont);
	xr_delete(pFontCPU);
	xr_delete(pFontXrThread);
}

void CStats::FilteredLog(const char* s)
{
	if (s && s[0] == '!' && s[1] == ' ')
		errors.push_back(shared_str(s));
}

void CStats::OnRender()
{
	// XXX: move to xrSound
#ifdef DEBUG
	if (g_stats_flags.is(st_sound))
	{
		CSound_stats_ext snd_stat_ext;
		::Sound->statistic(0, &snd_stat_ext);
		auto _I = snd_stat_ext.items.begin();
		auto _E = snd_stat_ext.items.end();
		for (; _I != _E; _I++)
		{
			const CSound_stats_ext::SItem& item = *_I;
			if (item._3D)
			{
				::DU->DrawCross(item.params.position, 0.5f, 0xFF0000FF, true);
				if (g_stats_flags.is(st_sound_min_dist))
					::DU->DrawSphere(
						Fidentity, item.params.position, item.params.min_distance, 0x400000FF, 0xFF0000FF, true, true);
				if (g_stats_flags.is(st_sound_max_dist))
					::DU->DrawSphere(
						Fidentity, item.params.position, item.params.max_distance, 0x4000FF00, 0xFF008000, true, true);

				xr_string out_txt = (out_txt.size() && g_stats_flags.is(st_sound_info_name)) ? item.name.c_str() : "";

				if (item.game_object)
				{
					if (g_stats_flags.is(st_sound_ai_dist))
						::DU->DrawSphere(Fidentity, item.params.position, item.params.max_ai_distance,
							0x80FF0000, 0xFF800000, true, true);
					if (g_stats_flags.is(st_sound_info_object))
					{
						out_txt += " (";
						out_txt += item.game_object->cNameSect().c_str();
						out_txt += ")";
					}
				}
				if (g_stats_flags.is_any(st_sound_info_name | st_sound_info_object) && item.name.size())
					::DU->OutText(item.params.position, out_txt.c_str(), 0xFFFFFFFF, 0xFF000000);
			}
		}
	}
#endif
}
