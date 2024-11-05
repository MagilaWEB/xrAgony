// GameObject.h: interface for the CGameObject class.
//
//////////////////////////////////////////////////////////////////////

#pragma once
#if !defined(AFX_GAMEOBJECT_H__3DA72D03_C759_4688_AEBB_89FA812AA873__INCLUDED_)
#define AFX_GAMEOBJECT_H__3DA72D03_C759_4688_AEBB_89FA812AA873__INCLUDED_

#include "stdafx.h"
#include "xrEngine/xr_object.h"
#include "xrServer_Space.h"
#include "alife_space.h"
#include "xrScriptEngine/script_space_forward.hpp"
#include "xrScriptEngine/DebugMacros.hpp" // XXX: move debug macros to xrCore
#include "script_binder.h"
#include "Hit.h"
#include "game_object_space.h"

class CScriptGameObject;
class CPhysicsShell;
class CSE_Abstract;
class CPHSynchronize;
class CInventoryItem;
class CEntity;
class CEntityAlive;
class CInventoryOwner;
class CActor;
class CPhysicsShellHolder;
class CParticlesPlayer;
class CCustomZone;
class IInputReceiver;
class CArtefact;
class CCustomMonster;
class CAI_Stalker;
class CScriptEntity;
class CAI_ObjectLocation;
class CWeapon;
class CExplosive;
class CHolderCustom;
class CAttachmentOwner;
class CBaseMonster;
class CSpaceRestrictor;
class CAttachableItem;
class animation_movement_controller;
class CBlend;
class ai_obstacle;
class IKinematics;

template <typename _return_type>
class CScriptCallbackEx;

#pragma pack(push, 4)
class CGameObject : public IGameObject,
	public FactoryObjectBase,
	public SpatialBase,
	public ScheduledBase,
	public RenderableBase,
	public CollidableBase
{
	BENCH_SEC_SCRAMBLEMEMBER1
		BENCH_SEC_SCRAMBLEVTBL2
		// Some property variables
		GameObjectProperties Props;
	shared_str NameObject;
	shared_str NameSection;
	shared_str NameVisual;

protected:
	// Parentness
	IGameObject* Parent;
	// Geometric (transformation)
	svector<GameObjectSavedPosition, 4> PositionStack;
#ifdef DEBUG
	u32 dbg_update_cl;
#endif
	u32 dwFrame_UpdateCL;

private:
	shared_str m_sTipText{};
	bool m_bNonscriptUsable;
	bool m_spawned;
	Flags32 m_server_flags;
	CAI_ObjectLocation* m_ai_location;
	ALife::_STORY_ID m_story_id;
	animation_movement_controller* m_anim_mov_ctrl;
	bool m_bCrPr_Activated;
	u32 m_dwCrPr_ActivationStep;
	mutable CScriptGameObject* m_lua_game_object;
	int m_script_clsid;
	u32 m_spawn_time;
	using CALLBACK_MAP = xr_map<GameObject::ECallbackType, CScriptCallbackExVoid>;
	CALLBACK_MAP* m_callbacks;
	ai_obstacle* m_ai_obstacle;
	Fmatrix m_previous_matrix;
	CALLBACK_VECTOR m_visual_callback;

protected:
	CScriptBinder scriptBinder;
	bool m_bObjectRemoved;
	CInifile* m_ini_file;

public:
	CGameObject();
	virtual ~CGameObject();
	// XXX: review
#ifdef DEBUG
	u32 GetDbgUpdateFrame() const override { return dbg_update_cl; }
	void SetDbgUpdateFrame(u32 value) override { dbg_update_cl = value; }
	void DBGGetProps(GameObjectProperties& p) const override { p = Props; }
#endif
	// Network
	BOOL Local() const override { return Props.net_Local; }
	BOOL Remote() const override { return !Props.net_Local; }
	u16 ID() const override { return Props.net_ID; }
	void setID(u16 _ID) override { Props.net_ID = _ID; }
	BOOL GetTmpPreDestroy() const override { return Props.bPreDestroy; }
	void SetTmpPreDestroy(BOOL b) override { Props.bPreDestroy = b; }
	float shedule_Scale() override { return Device.vCameraPosition.distance_to(Position()) / 200.f; }
	bool shedule_Needed() override;
	void shedule_Update(u32 dt) override;
	// Parentness
	IGameObject* H_Parent() override { return Parent; }
	const IGameObject* H_Parent() const override { return Parent; }
	IGameObject* H_Root() override { return Parent ? Parent->H_Root() : this; }
	const IGameObject* H_Root() const override { return Parent ? Parent->H_Root() : this; }
	IGameObject* H_SetParent(IGameObject* O, bool just_before_destroy = false) override;
	// Geometry xform
	void Center(Fvector& C) const override;
	const Fmatrix& XFORM() const override
	{
		VERIFY(_valid(renderable.xform));
		return renderable.xform;
	}
	Fmatrix& XFORM() override { return renderable.xform; }
	void spatial_register() override;
	void spatial_unregister() override;
	void spatial_move() override;
	void spatial_update(float eps_P, float eps_R) override;
	IGameObject* dcast_GameObject() override { return this; }
	IRenderable* dcast_Renderable() override { return this; }
	Fvector& Direction() override { return renderable.xform.k; }
	const Fvector& Direction() const override { return renderable.xform.k; }
	Fvector& Position() override { return renderable.xform.c; }
	const Fvector& Position() const override { return renderable.xform.c; }
	float Radius() const override;
	const Fbox& BoundingBox() const override;
	IRender_Sector* Sector() override { return H_Root()->GetSpatialData().sector; }
	IRender_ObjectSpecific* ROS() override { return RenderableBase::renderable_ROS(); }
	BOOL renderable_ShadowGenerate() override { return TRUE; }
	BOOL renderable_ShadowReceive() override { return TRUE; }
	// Accessors and converters
	IRenderVisual* Visual() const override { return renderable.visual; }
	IPhysicsShell* physics_shell() override { return nullptr; }
	const IObjectPhysicsCollision* physics_collision() override { return nullptr; }
	// Name management
	shared_str cName() const override { return NameObject; }
	void cName_set(shared_str N) override;
	shared_str cNameSect() const override { return NameSection; }
	LPCSTR cNameSect_str() const override { return NameSection.c_str(); }
	void cNameSect_set(shared_str N) override;
	shared_str cNameVisual() const override { return NameVisual; }
	void cNameVisual_set(shared_str N) override;
	shared_str shedule_Name() const override { return cName(); };
	// Properties
	void processing_activate() override; // request to enable UpdateCL
	void processing_deactivate() override; // request to disable UpdateCL
	bool processing_enabled() override { return !!Props.bActiveCounter; }
	void setVisible(BOOL _visible) override;
	BOOL getVisible() const override { return Props.bVisible; }
	void setEnabled(BOOL _enabled) override;
	BOOL getEnabled() const override { return Props.bEnabled; }
	void setDestroy(BOOL _destroy) override;
	BOOL getDestroy() const override { return Props.bDestroy; }
	void setLocal(BOOL _local) override { Props.net_Local = _local ? 1 : 0; }
	BOOL getLocal() const override { return Props.net_Local; }
	void setSVU(BOOL _svu) override { Props.net_SV_Update = _svu ? 1 : 0; }
	BOOL getSVU() const override { return Props.net_SV_Update; }
	void setReady(BOOL _ready) override { Props.net_Ready = _ready ? 1 : 0; }
	BOOL getReady() const override { return Props.net_Ready; }
	// functions used for avoiding most of the smart_cast
	CAttachmentOwner* cast_attachment_owner() override { return nullptr; }
	CInventoryOwner* cast_inventory_owner() override { return nullptr; }
	CInventoryItem* cast_inventory_item() override { return nullptr; }
	CEntity* cast_entity() override { return nullptr; }
	CEntityAlive* cast_entity_alive() override { return nullptr; }
	CActor* cast_actor() override { return nullptr; }
	CGameObject* cast_game_object() override { return this; }
	CCustomZone* cast_custom_zone() override { return nullptr; }
	CPhysicsShellHolder* cast_physics_shell_holder() override { return nullptr; }
	IInputReceiver* cast_input_receiver() override { return nullptr; }
	CParticlesPlayer* cast_particles_player() override { return nullptr; }
	CArtefact* cast_artefact() override { return nullptr; }
	CCustomMonster* cast_custom_monster() override { return nullptr; }
	CAI_Stalker* cast_stalker() override { return nullptr; }
	CScriptEntity* cast_script_entity() override { return nullptr; }
	CWeapon* cast_weapon() override { return nullptr; }
	CExplosive* cast_explosive() override { return nullptr; }
	CSpaceRestrictor* cast_restrictor() override { return nullptr; }
	CAttachableItem* cast_attachable_item() override { return nullptr; }
	CHolderCustom* cast_holder_custom() override { return nullptr; }
	CBaseMonster* cast_base_monster() override { return nullptr; }
	CShellLauncher* cast_shell_launcher() override { return nullptr; }
	bool feel_touch_on_contact(IGameObject*) override { return TRUE; }
	// Utilities
	// XXX: move out
	static void u_EventGen(NET_Packet& P, u32 type, u32 dest);
	static void u_EventSend(NET_Packet& P);
	// Methods
	void Load(LPCSTR section) override;
	void PostLoad(LPCSTR section) override; //--#SM+#--
	void OnChangeVisual() override;
	// object serialization
	void net_Save(NET_Packet& packet) override;
	void net_Load(IReader& reader) override;
	BOOL net_SaveRelevant() override;
	void net_Export(NET_Packet& packet) override {} // export to server
	void net_Import(NET_Packet& packet) override {} // import from server
	BOOL net_Spawn(CSE_Abstract* entity) override;
	void net_Destroy() override;
	BOOL net_Relevant() override { return getLocal(); } // send messages only if active and local
	void net_MigrateInactive(NET_Packet& packet) override { Props.net_Local = FALSE; }
	void net_MigrateActive(NET_Packet& packet) override { Props.net_Local = TRUE; }
	void net_Relcase(IGameObject* O) override; // destroy all links to another objects
	void save(NET_Packet& output_packet) override;
	void load(IReader& input_packet);
	// Position stack
	u32 ps_Size() const override { return PositionStack.size(); }
	GameObjectSavedPosition ps_Element(u32 ID) const override;
	void ForceTransform(const Fmatrix& m) override {}
	void OnHUDDraw(CCustomHUD* hud) override {}
	void OnRenderHUD(IGameObject* pCurViewEntity) override {} //--#SM+#--
	void OnOwnedCameraMove(CCameraBase* pCam, float fOldYaw, float fOldPitch) override {} //--#SM+#--
	BOOL Ready() override { return getReady(); } // update only if active and fully initialized by/for network
	void renderable_Render() override;
	void OnEvent(NET_Packet& P, u16 type) override;
	void Hit(SHit* pHDS) override {}
	void SetHitInfo(IGameObject* who, IGameObject* weapon, s16 element, Fvector Pos, Fvector Dir) override {}
	BOOL BonePassBullet(int boneID) override { return FALSE; }
	//игровое имя объекта
	LPCSTR Name() const override;
	// Active/non active
	void OnH_B_Chield() override; // before
	void OnH_B_Independent(bool just_before_destroy) override;
	void OnH_A_Chield() override; // after
	void OnH_A_Independent() override;
	void On_SetEntity() override {}
	void On_LostEntity() override {}
	bool register_schedule() const override { return true; }
	Fvector get_new_local_point_on_mesh(u16& bone_id) const override;
	Fvector get_last_local_point_on_mesh(const Fvector& last_point, u16 bone_id) const override;
	bool IsVisibleForZones() override { return true; }
	bool NeedToDestroyObject() const override;
	void DestroyObject() override;
	// animation_movement_controller
	void create_anim_mov_ctrl(CBlend* b, Fmatrix* start_pose, bool local_animation) override;
	void destroy_anim_mov_ctrl() override;
	void update_animation_movement_controller();
	bool animation_movement_controlled() const override;
	const animation_movement_controller* animation_movement() const override { return m_anim_mov_ctrl; }
	animation_movement_controller* animation_movement() override { return m_anim_mov_ctrl; }
	// Game-specific events
	BOOL UsedAI_Locations() override;
	BOOL TestServerFlag(u32 Flag) const override;
	bool can_validate_position_on_spawn() override { return true; }
#ifdef DEBUG
	void OnRender() override;
#endif
	void reinit() override;
	void reload(LPCSTR section) override;
	///////////////////// network /////////////////////////////////////////
	bool object_removed() const override { return m_bObjectRemoved; }
	void make_Interpolation() override {} // interpolation from last visible to corrected position/rotation
	void PH_B_CrPr() override {} // actions & operations before physic correction-prediction steps
	void PH_I_CrPr() override {} // actions & operations after correction before prediction steps
#ifdef DEBUG
	void PH_Ch_CrPr() override {}
	void dbg_DrawSkeleton() override;
#endif
	void PH_A_CrPr() override {} // actions & operations after phisic correction-prediction steps
	void CrPr_SetActivationStep(u32 Step) override { m_dwCrPr_ActivationStep = Step; }
	u32 CrPr_GetActivationStep() override { return m_dwCrPr_ActivationStep; }
	void CrPr_SetActivated(bool Activate) override { m_bCrPr_Activated = Activate; }
	bool CrPr_IsActivated() override { return m_bCrPr_Activated; };
	///////////////////////////////////////////////////////////////////////
	const SRotation Orientation() const override
	{
		SRotation rotation;
		float h, p, b;
		XFORM().getHPB(h, p, b);
		rotation.yaw = h;
		rotation.pitch = p;
		return rotation;
	}
	bool use_parent_ai_locations() const override { return true; }
	void add_visual_callback(visual_callback callback) override;
	void remove_visual_callback(visual_callback callback) override;
	CALLBACK_VECTOR& visual_callbacks() override { return m_visual_callback; }
	CScriptGameObject* lua_game_object() const override;
	int clsid() const override
	{
		if (getDestroy())
			return 0;
#ifdef DEBUG
		THROW(m_script_clsid >= 0);
#endif
		return m_script_clsid;
	}
	CInifile* spawn_ini() override { return m_ini_file; }
	CAI_ObjectLocation& ai_location() const override
	{
		VERIFY(m_ai_location);
		return *m_ai_location;
	}
	u32 spawn_time() const override
	{
		VERIFY(m_spawned);
		return m_spawn_time;
	}
	const ALife::_STORY_ID& story_id() const override { return m_story_id; }
	u32 ef_creature_type() const override;
	u32 ef_equipment_type() const override;
	u32 ef_main_weapon_type() const override;
	u32 ef_anomaly_type() const override;
	u32 ef_weapon_type() const override;
	u32 ef_detector_type() const override;
	bool natural_weapon() const override { return true; }
	bool natural_detector() const override { return true; }
	bool use_center_to_aim() const override { return false; }
	// [12.11.07] Alexander Maniluk: added this method for moving object
	void MoveTo(const Fvector& position) override {}
	// the only usage: aimers::base::fill_bones
	CScriptCallbackExVoid& callback(GameObject::ECallbackType type) const override;
	LPCSTR visual_name(CSE_Abstract* server_entity) override;
	void On_B_NotCurrentEntity() override {}
	bool is_ai_obstacle() const override;
	ai_obstacle& obstacle() const override
	{
		VERIFY(m_ai_obstacle);
		return *m_ai_obstacle;
	}
	void on_matrix_change(const Fmatrix& previous) override;

	// UsableScriptObject functions
	bool use(IGameObject* obj) override;

	//строчка появляющаяся при наведении на объект (если nullptr, то нет)
	LPCSTR tip_text() override;
	void set_tip_text(LPCSTR new_text) override;
	void set_tip_text_default() override;

	//можно ли использовать объект стандартным (не скриптовым) образом
	bool nonscript_usable() override;
	void set_nonscript_usable(bool usable) override;
	CScriptBinderObject* GetScriptBinderObject() override { return scriptBinder.object(); }
	void SetScriptBinderObject(CScriptBinderObject* object) override { scriptBinder.set_object(object); }

protected:
	virtual void spawn_supplies();

private: // XXX: move to GameObjectBase
	void init();
	void setup_parent_ai_locations(bool assign_position = true);
	void validate_ai_locations(bool decrement_reference = true);
	u32 new_level_vertex_id() const;
	void update_ai_locations(bool decrement_reference);
	void SetKinematicsCallback(bool set);

private:
	enum
	{
		eNonVisible,
		eMainViewport,
		eSecondViewport
	};

private:
	inline static float					s_update_radius_1;
	inline static float					s_update_radius_2;
	inline static float					s_update_delta_radius;
	inline static float					s_update_time;
	inline static float					s_update_radius_invisible_k;

	u8									b_visibility_status = 0;
	u8									b_visibility_status_next = 0;
	float								m_next_update_time = 0.f;
	float								fDeltaTime = 0.f;
	size_t								dwDeltaTime = 0;
	float								m_last_update_time_f = 0.f;
	size_t								m_last_update_time_dw = 0;

	void								calc_next_update_time();

	void								on_distance_update() override;

protected:
	void								UpdateCL() override;

public:
	static void							loadStaticData();

	float								fDeltaT() const { return fDeltaTime; };
	size_t								dwDeltaT() const { return dwDeltaTime; };
	void								fSetDeltaT(float fDelta) { fDeltaTime = fDelta; };
	void								dwSetDeltaT(size_t dwDelta) { dwDeltaTime = dwDelta; };

	void								update() override;
	bool								queryUpdateCL() override;

	virtual bool						alwaysUpdateCL() { return false; }
};
#pragma pack(pop)

#endif // !defined(AFX_GAMEOBJECT_H__3DA72D03_C759_4688_AEBB_89FA812AA873__INCLUDED_)
