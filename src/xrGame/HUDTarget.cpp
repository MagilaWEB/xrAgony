#include "stdafx.h"
#include "hudtarget.h"
#include "xrEngine/GameMtlLib.h"

#include "xrEngine/Environment.h"
#include "xrEngine/CustomHUD.h"
#include "xrEngine/GameFont.h"
#include "Entity.h"
#include "Level.h"
#include "game_cl_base.h"
#include "xrEngine/IGame_Persistent.h"

#include "xrUICore/ui_base.h"
#include "InventoryOwner.h"
#include "relation_registry.h"
#include "character_info.h"

#include "string_table.h"
#include "entity_alive.h"

#include "inventory_item.h"
#include "inventory.h"

#include "ai/monsters/poltergeist/poltergeist.h"
#include <WeaponKnife.h>
#include <WeaponBinoculars.h>

u32 C_ON_ENEMY = color_rgba(0xff, 0, 0, 0x80);
u32 C_ON_NEUTRAL = color_rgba(0xff, 0xff, 0x80, 0x80);
u32 C_ON_FRIEND = color_rgba(0, 0xff, 0, 0x80);

#define C_DEFAULT color_rgba(0xff, 0xff, 0xff, 0x80)
#define C_SIZE 0.025f
#define NEAR_LIM 0.5f

#define SHOW_INFO_SPEED 0.5f
#define HIDE_INFO_SPEED 10.f

IC float recon_mindist() { return 2.f; }
IC float recon_maxdist() { return 50.f; }
IC float recon_minspeed() { return 0.5f; }
IC float recon_maxspeed() { return 10.f; }
CHUDTarget::CHUDTarget()
{
	fuzzyShowInfo = 0.f;
	PP.RQ.range = 0.f;
	hShader->create("hud\\cursor", "ui\\cursor");

	PP.RQ.set(nullptr, 0.f, -1);

	Load();
	m_bShowCrosshair = false;
}

CHUDTarget::~CHUDTarget() {}
void CHUDTarget::Load() { HUDCrosshair.Load(); }
void CHUDTarget::ShowCrosshair(bool b) { m_bShowCrosshair = b; }
//. fVisTransparencyFactor
float fCurrentPickPower;
ICF static BOOL pick_trace_callback(collide::rq_result& result, LPVOID params)
{
	SPickParam* pp = (SPickParam*)params;
	//	collide::rq_result* RQ	= pp->RQ;
	++pp->pass;

	if (result.O)
	{
		pp->RQ = result;
		return FALSE;
	}
	else
	{
		//получить треугольник и узнать его материал
		CDB::TRI* T = Level().ObjectSpace.GetStaticTris() + result.element;

		SGameMtl* mtl = GMLib.GetMaterialByIdx(T->material);
		pp->power *= mtl->fVisTransparencyFactor;
		if (pp->power > 0.34f)
		{
			return TRUE;
		}
		//.		if (mtl->Flags.is(SGameMtl::flPassable))
		//.			return TRUE;
	}
	pp->RQ = result;
	return FALSE;
}

void CHUDTarget::CursorOnFrame()
{
	Fvector p1, dir;

	p1 = ::IDevice->cast()->vCameraPosition;
	dir = ::IDevice->cast()->vCameraDirection;

	// Render cursor
	if (Level().CurrentEntity())
	{
		PP.RQ.O = 0;
		PP.RQ.range = g_pGamePersistent->Environment().CurrentEnv->far_plane * 0.99f;
		PP.RQ.element = -1;

		collide::ray_defs RD(p1, dir, PP.RQ.range, CDB::OPT_CULL, collide::rqtBoth);
		RQR.r_clear();
		VERIFY(!fis_zero(RD.dir.square_magnitude()));

		PP.power = 1.0f;
		PP.pass = 0;

		if (Level().ObjectSpace.RayQuery(RQR, RD, pick_trace_callback, &PP, nullptr, Level().CurrentEntity()))
			clamp(PP.RQ.range, NEAR_LIM, PP.RQ.range);
	}
}

extern ENGINE_API BOOL g_bRendering;
void CHUDTarget::Render()
{
	BOOL b_do_rendering = (psHUD_Flags.is(HUD_CROSSHAIR | HUD_CROSSHAIR_RT | HUD_CROSSHAIR_RT2));

	if (!b_do_rendering)
		return;

	VERIFY(g_bRendering);

	IGameObject* O = Level().CurrentEntity();
	if (!O)
		return;
	CEntity* E = smart_cast<CEntity*>(O);
	if (!E)
		return;

	Fvector p1 = ::IDevice->cast()->vCameraPosition;
	Fvector dir = ::IDevice->cast()->vCameraDirection;

	// Render cursor
	u32 C = C_DEFAULT;

	Fvector		p2;
	p2.mad(p1, dir, PP.RQ.range);
	Fvector4	pt;
	::IDevice->cast()->mFullTransform.transform(pt, p2);
	pt.y = -pt.y;
	float di_size = C_SIZE / powf(pt.w, .2f);

	Fvector2	scr_size2;
	scr_size2.set(float(::IDevice->cast()->dwWidth), float(::IDevice->cast()->dwHeight));
	float size_x2 = scr_size2.x * di_size;
	float size_y2 = scr_size2.y * di_size;

	size_y2 = size_x2;

	float w_3 = scr_size2.x / 2.0f;
	float h_3 = scr_size2.y / 2.0f;

	// Convert to screen coords
	float cx = w_3, cy = h_3;

	CGameFont* F = UI().Font().pFontGraffiti19Russian;
	F->SetAligment(CGameFont::alCenter);
	F->OutSetI(0.f, 0.05f);

	if (Actor())
	{
		CWeapon* weapon = smart_cast<CWeapon*>(Actor()->inventory().ActiveItem());
		if (weapon
			&& !smart_cast<CWeaponKnife*>(weapon)
			&& !Actor()->inventory().ActiveItem()->cast_missile()
			&& !smart_cast<CWeaponBinoculars*>(weapon)
			)
		{
			if (weapon->IsZoomed() && weapon->ZoomHideCrosshair() != false)
				return;
			Fvector			result;
			Fvector4		v_res;
			float			dist; dist = PP.RQ.range;

			result = weapon->get_LastFP();
			result.add(Fvector(weapon->get_LastFD()).mul(dist));

			::IDevice->cast()->mFullTransform.transform(v_res, result);

			if (v_res.z < 0 || v_res.w < 0)	return;
			if (v_res.x < -1.f || v_res.x > 1.f || v_res.y < -1.f || v_res.y>1.f) return;

			cx = (1.f + v_res.x) / 2.f * (::IDevice->cast()->dwWidth);
			cy = (1.f - v_res.y) / 2.f * (::IDevice->cast()->dwHeight);
			F->OutSet(cx, cy + 5);
		}
	}

	if (psHUD_Flags.test(HUD_CROSSHAIR_DIST))
		F->OutSkip();

	if (psHUD_Flags.test(HUD_INFO))
	{
		bool const is_poltergeist = PP.RQ.O && !!smart_cast<CPoltergeist*>(PP.RQ.O);

		if ((PP.RQ.O && PP.RQ.O->getVisible()) || is_poltergeist)
		{
			CEntityAlive* E = smart_cast<CEntityAlive*>(PP.RQ.O);
			CEntityAlive* pCurEnt = smart_cast<CEntityAlive*>(Level().CurrentEntity());
			PIItem l_pI = smart_cast<PIItem>(PP.RQ.O);

			CInventoryOwner* our_inv_owner = smart_cast<CInventoryOwner*>(pCurEnt);
			if (E && E->g_Alive() && E->cast_base_monster())
			{
				C = C_ON_ENEMY;
			}
			else if (E && E->g_Alive() && !E->cast_base_monster())
			{
				CInventoryOwner* others_inv_owner = smart_cast<CInventoryOwner*>(E);

				if (our_inv_owner && others_inv_owner)
				{
					switch (RELATION_REGISTRY().GetRelationType(others_inv_owner, our_inv_owner))
					{
					case ALife::eRelationTypeEnemy: C = C_ON_ENEMY; break;
					case ALife::eRelationTypeNeutral: C = C_ON_NEUTRAL; break;
					case ALife::eRelationTypeFriend: C = C_ON_FRIEND; break;
					}

					if (fuzzyShowInfo > 0.5f)
					{
						F->SetColor(subst_alpha(C, u8(iFloor(255.f * (fuzzyShowInfo - 0.5f) * 2.f))));
						F->OutNext("%s", *StringTable().translate(others_inv_owner->Name()));
						F->OutNext("%s", *StringTable().translate(others_inv_owner->CharacterInfo().Community().id()));
					}
				}

				fuzzyShowInfo += SHOW_INFO_SPEED * ::IDevice->TimeDelta_sec();
			}
			else if (l_pI && our_inv_owner && PP.RQ.range < 2.0f * 2.0f)
			{
				if (fuzzyShowInfo > 0.5f && l_pI->NameItem())
				{
					F->SetColor(subst_alpha(C, u8(iFloor(255.f * (fuzzyShowInfo - 0.5f) * 2.f))));
					F->OutNext("%s", l_pI->NameItem());
				}
				fuzzyShowInfo += SHOW_INFO_SPEED * ::IDevice->TimeDelta_sec();
			}
		}
		else
		{
			fuzzyShowInfo -= HIDE_INFO_SPEED * ::IDevice->TimeDelta_sec();
		}
		clamp(fuzzyShowInfo, 0.f, 1.f);
	}

	if (psHUD_Flags.test(HUD_CROSSHAIR_DIST))
	{
		F->OutSet(cx, cy);
		F->SetColor(C);
#ifdef DEBUG
		F->OutNext("%4.1f - %4.2f - %d", PP.RQ.range, PP.power, PP.pass);
#else
		F->OutNext("%4.1f", PP.RQ.range);
#endif
	}

	//отрендерить кружочек или крестик
	if (!m_bShowCrosshair)
	{
		::UIRender->StartPrimitive(6, IUIRender::ptTriList, UI().m_currentPointType);

		Fvector2 scr_size;
		scr_size.set(float(::IDevice->cast()->dwWidth), float(::IDevice->cast()->dwHeight));
		float size_x = scr_size.x * di_size;
		float size_y = scr_size.y * di_size;

		size_y = size_x;

		float w_4 = scr_size.x / 2.0f;
		float h_4 = scr_size.y / 2.0f;

		// Convert to screen coords
		float cx2 = (pt.x + 1) * w_4;
		float cy2 = (pt.y + 1) * h_4;

		//	TODO: return code back to indexed rendering since we use quads
		//	Tri 1
		::UIRender->PushPoint(cx2 - size_x, cy2 + size_y, 0, C, 0, 1);
		::UIRender->PushPoint(cx2 - size_x, cy2 - size_y, 0, C, 0, 0);
		::UIRender->PushPoint(cx2 + size_x, cy2 + size_y, 0, C, 1, 1);
		//	Tri 2
		::UIRender->PushPoint(cx2 + size_x, cy2 + size_y, 0, C, 1, 1);
		::UIRender->PushPoint(cx2 - size_x, cy2 - size_y, 0, C, 0, 0);
		::UIRender->PushPoint(cx2 + size_x, cy2 - size_y, 0, C, 1, 0);

		// unlock VB and Render it as triangle LIST
		::UIRender->SetShader(*hShader);
		::UIRender->FlushPrimitive();
	}
	else
	{
		//отрендерить прицел
		HUDCrosshair.cross_color = C;
		HUDCrosshair.OnRender();
	}
}

void CHUDTarget::net_Relcase(IGameObject* O)
{
	if (PP.RQ.O == O)
		PP.RQ.O = nullptr;

	RQR.r_clear();
}
