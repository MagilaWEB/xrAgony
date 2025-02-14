#include "pch_script.h"
#include "xr_level_controller.h"
#include "xrEngine/xr_input.h"
#include "xrScriptEngine/ScriptExporter.hpp"
#include <dinput.h>

static int dik_to_bind(int dik) { return get_binded_action(dik); }
static bool key_state(int key) { return !!pInput->iGetAsyncKeyState(key); }
struct EnumGameActions {};
struct KeyBindingRegistrator {};

SCRIPT_EXPORT(KeyBindings, (),
{
	module(luaState)
	[
		def("dik_to_bind", &dik_to_bind),
		def("bind_to_dik", &get_action_dik),
		def("key_state", &key_state),
		class_<EnumGameActions>("key_bindings")
			.enum_("commands")
			[
				value("kFWD",						kFWD),
				value("kARTEFACT",					kARTEFACT),
				value("kBACK",						kBACK),
				value("kL_STRAFE",					kL_STRAFE),
				value("kR_STRAFE",					kR_STRAFE),
				value("kL_LOOKOUT",					kL_LOOKOUT),
				value("kR_LOOKOUT",					kR_LOOKOUT),
				value("kLEFT",						kLEFT),
				value("kRIGHT",						kRIGHT),
				value("kUP",						kUP),
				value("kDOWN",						kDOWN),
				value("kJUMP",						kJUMP),
				value("kCROUCH",					kCROUCH),
				value("kACCEL",						kACCEL),
				value("kSPRINT_TOGGLE",				kSPRINT_TOGGLE),
				value("kCAM_1",						kCAM_1),
				value("kCAM_2",						kCAM_2),
				value("kCAM_3",						kCAM_3),
				value("kCAM_ZOOM_IN",				kCAM_ZOOM_IN),
				value("kCAM_ZOOM_OUT",				kCAM_ZOOM_OUT),
				value("kQUICK_SAVE",				kQUICK_SAVE),
				value("kQUICK_LOAD",				kQUICK_LOAD),
				value("kCAM_AUTOAIM",				kCAM_AUTOAIM),
				value("kCUSTOM1",					kCUSTOM1),
				value("kCUSTOM2",					kCUSTOM2),
				value("kCUSTOM3",					kCUSTOM3),
				value("kCUSTOM4",					kCUSTOM4),
				value("kCUSTOM5",					kCUSTOM5),
				value("kCUSTOM6",					kCUSTOM6),
				value("kCUSTOM7",					kCUSTOM7),
				value("kCUSTOM8",					kCUSTOM8),
				value("kCUSTOM9",					kCUSTOM9),
				value("kCUSTOM10",					kCUSTOM10),
				value("kCUSTOM11",					kCUSTOM11),
				value("kCUSTOM12",					kCUSTOM12),
				value("kCUSTOM13",					kCUSTOM13),
				value("kCUSTOM14",					kCUSTOM14),
				value("kCUSTOM15",					kCUSTOM15),
				value("kCUSTOM16",					kCUSTOM16),
				value("kCUSTOM17",					kCUSTOM17),
				value("kCUSTOM18",					kCUSTOM18),
				value("kCUSTOM19",					kCUSTOM19),
				value("kCUSTOM20",					kCUSTOM20),
				value("kPDA_TAB1",					kPDA_TAB1),
				value("kPDA_TAB2",					kPDA_TAB2),
				value("kPDA_TAB3",					kPDA_TAB3),
				value("kPDA_TAB4",					kPDA_TAB4),
				value("kPDA_TAB5",					kPDA_TAB5),
				value("kPDA_TAB6",					kPDA_TAB6),
				value("kPDA_TAB7",					kPDA_TAB7),
				value("kTORCH",						kTORCH),
				value("kTORCH_MODE",				kTORCH_MODE),
				value("kNIGHT_VISION",				kNIGHT_VISION),
				value("kWPN_1",						kWPN_1),
				value("kWPN_2",						kWPN_2),
				value("kWPN_3",						kWPN_3),
				value("kWPN_4",						kWPN_4),
				value("kWPN_5",						kWPN_5),
				value("kWPN_6",						kWPN_6),
				value("kWPN_NEXT",					kWPN_NEXT),
				value("kWPN_FIRE",					kWPN_FIRE),
				value("kWPN_RELOAD",				kWPN_RELOAD),
				value("kWPN_ZOOM",					kWPN_ZOOM),
				value("kWPN_FUNC",					kWPN_FUNC),
				value("kWPN_FIREMODE_PREV", 		kWPN_FIREMODE_PREV),
				value("kWPN_FIREMODE_NEXT", 		kWPN_FIREMODE_NEXT),
				value("kWPN_SAFEMODE",				kWPN_SAFEMODE),
				value("kUSE",						kUSE),
				value("kDROP",						kDROP),
				value("kKICK",						kKICK),
				value("kHEADROTATION",				kHEADROTATION),
				value("kSCORES",					kSCORES),
				value("kCHAT",						kCHAT),
				value("kSCREENSHOT",				kSCREENSHOT),
				value("kQUIT",						kQUIT),
				value("kCONSOLE",					kCONSOLE),
				value("kINVENTORY",					kINVENTORY),
				value("kBUY",						kBUY),
				value("kSKIN",						kSKIN),
				value("kTEAM",						kTEAM)
			],
		class_<KeyBindingRegistrator>("DIK_keys")
			.enum_("dik_keys")
			[
				value("DIK_ESCAPE",					int(DIK_ESCAPE)),
				value("DIK_2",						int(DIK_2)),
				value("DIK_4",						int(DIK_4)),
				value("DIK_6",						int(DIK_6)),
				value("DIK_8",						int(DIK_8)),
				value("DIK_0",						int(DIK_0)),
				value("DIK_EQUALS",					int(DIK_EQUALS)),
				value("DIK_TAB",					int(DIK_TAB)),
				value("DIK_W",						int(DIK_W)),
				value("DIK_R",						int(DIK_R)),
				value("DIK_Y",						int(DIK_Y)),
				value("DIK_I",						int(DIK_I)),
				value("DIK_P",						int(DIK_P)),
				value("DIK_RBRACKET",				int(DIK_RBRACKET)),
				value("DIK_LCONTROL",				int(DIK_LCONTROL)),
				value("DIK_S",						int(DIK_S)),
				value("DIK_F",						int(DIK_F)),
				value("DIK_H",						int(DIK_H)),
				value("DIK_K",						int(DIK_K)),
				value("DIK_SEMICOLON",				int(DIK_SEMICOLON)),
				value("DIK_GRAVE",					int(DIK_GRAVE)),
				value("DIK_BACKSLASH",				int(DIK_BACKSLASH)),
				value("DIK_X",						int(DIK_X)),
				value("DIK_V",						int(DIK_V)),
				value("DIK_N",						int(DIK_N)),
				value("DIK_COMMA",					int(DIK_COMMA)),
				value("DIK_SLASH",					int(DIK_SLASH)),
				value("DIK_MULTIPLY",				int(DIK_MULTIPLY)),
				value("DIK_SPACE",					int(DIK_SPACE)),
				value("DIK_F1",						int(DIK_F1)),
				value("DIK_F3",						int(DIK_F3)),
				value("DIK_F5",						int(DIK_F5)),
				value("DIK_F7",						int(DIK_F7)),
				value("DIK_F9",						int(DIK_F9)),
				value("DIK_NUMLOCK",				int(DIK_NUMLOCK)),
				value("DIK_NUMPAD7",				int(DIK_NUMPAD7)),
				value("DIK_NUMPAD9",				int(DIK_NUMPAD9)),
				value("DIK_NUMPAD4",				int(DIK_NUMPAD4)),
				value("DIK_NUMPAD6",				int(DIK_NUMPAD6)),
				value("DIK_NUMPAD1",				int(DIK_NUMPAD1)),
				value("DIK_NUMPAD3",				int(DIK_NUMPAD3)),
				value("DIK_DECIMAL",				int(DIK_DECIMAL)),
				value("DIK_F12",					int(DIK_F12)),
				value("DIK_F14",					int(DIK_F14)),
				value("DIK_KANA",					int(DIK_KANA)),
				value("DIK_NOCONVERT",				int(DIK_NOCONVERT)),
				value("DIK_NUMPADEQUALS",			int(DIK_NUMPADEQUALS)),
				value("DIK_AT",						int(DIK_AT)),
				value("DIK_UNDERLINE",				int(DIK_UNDERLINE)),
				value("DIK_STOP",					int(DIK_STOP)),
				value("DIK_UNLABELED",				int(DIK_UNLABELED)),
				value("DIK_RCONTROL",				int(DIK_RCONTROL)),
				value("DIK_DIVIDE",					int(DIK_DIVIDE)),
				value("DIK_RMENU",					int(DIK_RMENU)),
				value("DIK_UP",						int(DIK_UP)),
				value("DIK_LEFT",					int(DIK_LEFT)),
				value("DIK_END",					int(DIK_END)),
				value("DIK_NEXT",					int(DIK_NEXT)),
				value("DIK_DELETE",					int(DIK_DELETE)),
				value("DIK_RWIN",					int(DIK_RWIN)),
				value("DIK_PAUSE",					int(DIK_PAUSE)),
				value("MOUSE_2",					int(MOUSE_2)),
				value("DIK_1",						int(DIK_1)),
				value("DIK_3",						int(DIK_3)),
				value("DIK_5",						int(DIK_5)),
				value("DIK_7",						int(DIK_7)),
				value("DIK_9",						int(DIK_9)),
				value("DIK_MINUS",					int(DIK_MINUS)),
				value("DIK_BACK",					int(DIK_BACK)),
				value("DIK_Q",						int(DIK_Q)),
				value("DIK_E",						int(DIK_E)),
				value("DIK_T",						int(DIK_T)),
				value("DIK_U",						int(DIK_U)),
				value("DIK_O",						int(DIK_O)),
				value("DIK_LBRACKET",				int(DIK_LBRACKET)),
				value("DIK_RETURN",					int(DIK_RETURN)),
				value("DIK_A",						int(DIK_A)),
				value("DIK_D",						int(DIK_D)),
				value("DIK_G",						int(DIK_G)),
				value("DIK_J",						int(DIK_J)),
				value("DIK_L",						int(DIK_L)),
				value("DIK_APOSTROPHE",				int(DIK_APOSTROPHE)),
				value("DIK_LSHIFT",					int(DIK_LSHIFT)),
				value("DIK_Z",						int(DIK_Z)),
				value("DIK_C",						int(DIK_C)),
				value("DIK_B",						int(DIK_B)),
				value("DIK_M",						int(DIK_M)),
				value("DIK_PERIOD",					int(DIK_PERIOD)),
				value("DIK_RSHIFT",					int(DIK_RSHIFT)),
				value("DIK_LMENU",					int(DIK_LMENU)),
				value("DIK_CAPITAL",				int(DIK_CAPITAL)),
				value("DIK_F2",						int(DIK_F2)),
				value("DIK_F4",						int(DIK_F4)),
				value("DIK_F6",						int(DIK_F6)),
				value("DIK_F8",						int(DIK_F8)),
				value("DIK_F10",					int(DIK_F10)),
				value("DIK_SCROLL",					int(DIK_SCROLL)),
				value("DIK_NUMPAD8",				int(DIK_NUMPAD8)),
				value("DIK_SUBTRACT",				int(DIK_SUBTRACT)),
				value("DIK_NUMPAD5",				int(DIK_NUMPAD5)),
				value("DIK_ADD",					int(DIK_ADD)),
				value("DIK_NUMPAD2",				int(DIK_NUMPAD2)),
				value("DIK_NUMPAD0",				int(DIK_NUMPAD0)),
				value("DIK_F11",					int(DIK_F11)),
				value("DIK_F13",					int(DIK_F13)),
				value("DIK_F15",					int(DIK_F15)),
				value("DIK_CONVERT",				int(DIK_CONVERT)),
				value("DIK_YEN",					int(DIK_YEN)),
				value("DIK_CIRCUMFLEX",				int(DIK_CIRCUMFLEX)),
				value("DIK_COLON",					int(DIK_COLON)),
				value("DIK_KANJI",					int(DIK_KANJI)),
				value("DIK_AX",						int(DIK_AX)),
				value("DIK_NUMPADENTER",			int(DIK_NUMPADENTER)),
				value("DIK_NUMPADCOMMA",			int(DIK_NUMPADCOMMA)),
				value("DIK_SYSRQ",					int(DIK_SYSRQ)),
				value("DIK_HOME",					int(DIK_HOME)),
				value("DIK_PRIOR",					int(DIK_PRIOR)),
				value("DIK_RIGHT",					int(DIK_RIGHT)),
				value("DIK_DOWN",					int(DIK_DOWN)),
				value("DIK_INSERT",					int(DIK_INSERT)),
				value("DIK_LWIN",					int(DIK_LWIN)),
				value("DIK_APPS",					int(DIK_APPS)),
				value("MOUSE_1",					int(MOUSE_1)),
				value("MOUSE_3",					int(MOUSE_3)),
				value("MOUSE_4",					int(MOUSE_4)),
				value("MOUSE_5",					int(MOUSE_5)),
				value("MOUSE_6",					int(MOUSE_6)),
				value("MOUSE_7",					int(MOUSE_7)),
				value("MOUSE_8",					int(MOUSE_8)),
				value("DIK_RETURN",					int(DIK_RETURN)),
				value("DIK_NUMPADENTER",			int(DIK_NUMPADENTER)),
				value("DIK_NUMPADPERIOD",			int(DIK_NUMPADPERIOD)),
				value("DIK_POWER",					int(DIK_POWER)),
				value("DIK_MUTE",                   int(DIK_MUTE)),
				value("DIK_VOLUMEUP",				int(DIK_VOLUMEUP)),
				value("DIK_VOLUMEDOWN",				int(DIK_VOLUMEDOWN)),
				value("DIK_NUMPADCOMMA",			int(DIK_NUMPADCOMMA))
			]
	];
});