#include "pch_script.h"
#include "ScriptXmlInit.h"
#include "ui\UIXmlInit.h"
#include "xrUICore/XML/UItextureMaster.h"
#include "xrUICore/Buttons/UICheckButton.h"
#include "xrUICore/SpinBox/UISpinNum.h"
#include "xrUICore/SpinBox/UISpinText.h"
#include "xrUICore/ComboBox/UIComboBox.h"
#include "xrUICore/TabControl/UITabControl.h"
#include "xrUICore/Windows/UIFrameWindow.h"
#include "ui\UIKeyBinding.h"
#include "xrUICore/EditBox/UIEditBox.h"
#include "xrUICore/Static/UIAnimatedStatic.h"
#include "ui/UISleepStatic.h"
#include "xrUICore/TrackBar/UITrackBar.h"
#include "ui\UIMMShniaga.h"
#include "xrUICore/ScrollView/UIScrollView.h"
#include "xrUICore/ProgressBar/UIProgressBar.h"
#include "xrUICore/Hint/UIHint.h"
#include "ui\UIHelper.h"
#include "xrScriptEngine/ScriptExporter.hpp"

using namespace luabind;

void _attach_child(CUIWindow* _child, CUIWindow* _parent)
{
	if (!_parent)
		return;

	_child->SetAutoDelete(true);
	CUIScrollView* _parent_scroll = smart_cast<CUIScrollView*>(_parent);
	if (_parent_scroll)
		_parent_scroll->AddWindow(_child, true);
	else
		_parent->AttachChild(_child);
}

void CScriptXmlInit::ParseFile(LPCSTR xml_file) { m_xml.Load(CONFIG_PATH, UI_PATH, UI_PATH_DEFAULT, xml_file); }

bool CScriptXmlInit::NodeExists(LPCSTR path, int index)
{
	XML_NODE node = m_xml.NavigateToNode(path, index);
	if (!node)
		return false;
	return true;
}

void CScriptXmlInit::InitWindow(LPCSTR path, int index, CUIWindow* pWnd)
{
	CUIXmlInit::InitWindow(m_xml, path, index, pWnd);
}

UIHint* CScriptXmlInit::InitHint(LPCSTR path, CUIWindow* parent)
{
	UIHint * pWnd = UIHelper::CreateHint(m_xml, path);
	return pWnd;
}

CUIFrameWindow* CScriptXmlInit::InitFrame(LPCSTR path, CUIWindow* parent)
{
	CUIFrameWindow* pWnd = new CUIFrameWindow();
	CUIXmlInit::InitFrameWindow(m_xml, path, 0, pWnd);
	_attach_child(pWnd, parent);
	return pWnd;
}

CUIFrameLineWnd* CScriptXmlInit::InitFrameLine(LPCSTR path, CUIWindow* parent)
{
	CUIFrameLineWnd* pWnd = new CUIFrameLineWnd();
	CUIXmlInit::InitFrameLine(m_xml, path, 0, pWnd);
	_attach_child(pWnd, parent);
	return pWnd;
}

CUIEditBox* CScriptXmlInit::InitEditBox(LPCSTR path, CUIWindow* parent)
{
	CUIEditBox* pWnd = new CUIEditBox();
	CUIXmlInit::InitEditBox(m_xml, path, 0, pWnd);
	_attach_child(pWnd, parent);
	return pWnd;
}

CUIStatic* CScriptXmlInit::InitStatic(LPCSTR path, CUIWindow* parent)
{
	CUIStatic* pWnd = new CUIStatic();
	CUIXmlInit::InitStatic(m_xml, path, 0, pWnd);
	_attach_child(pWnd, parent);
	return pWnd;
}

CUITextWnd* CScriptXmlInit::InitTextWnd(LPCSTR path, CUIWindow* parent)
{
	CUITextWnd* pWnd = new CUITextWnd();
	CUIXmlInit::InitTextWnd(m_xml, path, 0, pWnd);
	_attach_child(pWnd, parent);
	return pWnd;
}

CUIStatic* CScriptXmlInit::InitAnimStatic(LPCSTR path, CUIWindow* parent)
{
	CUIAnimatedStatic* pWnd = new CUIAnimatedStatic();
	CUIXmlInit::InitAnimatedStatic(m_xml, path, 0, pWnd);
	_attach_child(pWnd, parent);
	return pWnd;
}

CUIStatic* CScriptXmlInit::InitSleepStatic(LPCSTR path, CUIWindow* parent)
{
	CUISleepStatic* pWnd = new CUISleepStatic();
	CUIXmlInit::InitSleepStatic(m_xml, path, 0, pWnd);
	_attach_child(pWnd, parent);
	return pWnd;
}

CUIScrollView* CScriptXmlInit::InitScrollView(LPCSTR path, CUIWindow* parent)
{
	CUIScrollView* pWnd = new CUIScrollView();
	CUIXmlInit::InitScrollView(m_xml, path, 0, pWnd);
	_attach_child(pWnd, parent);
	return pWnd;
}

CUIListBox* CScriptXmlInit::InitListBox(LPCSTR path, CUIWindow* parent)
{
	CUIListBox* pWnd = new CUIListBox();
	CUIXmlInit::InitListBox(m_xml, path, 0, pWnd);
	_attach_child(pWnd, parent);
	return pWnd;
}

CUICheckButton* CScriptXmlInit::InitCheck(LPCSTR path, CUIWindow* parent)
{
	CUICheckButton* pWnd = new CUICheckButton();
	CUIXmlInit::InitCheck(m_xml, path, 0, pWnd);
	_attach_child(pWnd, parent);
	return pWnd;
}

CUISpinNum* CScriptXmlInit::InitSpinNum(LPCSTR path, CUIWindow* parent)
{
	CUISpinNum* pWnd = new CUISpinNum();
	CUIXmlInit::InitSpin(m_xml, path, 0, pWnd);
	_attach_child(pWnd, parent);
	return pWnd;
}

CUISpinFlt* CScriptXmlInit::InitSpinFlt(LPCSTR path, CUIWindow* parent)
{
	CUISpinFlt* pWnd = new CUISpinFlt();
	CUIXmlInit::InitSpin(m_xml, path, 0, pWnd);
	_attach_child(pWnd, parent);
	return pWnd;
}

CUISpinText* CScriptXmlInit::InitSpinText(LPCSTR path, CUIWindow* parent)
{
	CUISpinText* pWnd = new CUISpinText();
	CUIXmlInit::InitSpin(m_xml, path, 0, pWnd);
	_attach_child(pWnd, parent);
	return pWnd;
}

CUIComboBox* CScriptXmlInit::InitComboBox(LPCSTR path, CUIWindow* parent)
{
	CUIComboBox* pWnd = new CUIComboBox();
	CUIXmlInit::InitComboBox(m_xml, path, 0, pWnd);
	_attach_child(pWnd, parent);
	return pWnd;
}

CUI3tButton* CScriptXmlInit::Init3tButton(LPCSTR path, CUIWindow* parent)
{
	CUI3tButton* pWnd = new CUI3tButton();
	CUIXmlInit::Init3tButton(m_xml, path, 0, pWnd);
	_attach_child(pWnd, parent);
	return pWnd;
}

CUITabControl* CScriptXmlInit::InitTab(LPCSTR path, CUIWindow* parent)
{
	CUITabControl* pWnd = new CUITabControl();
	CUIXmlInit::InitTabControl(m_xml, path, 0, pWnd);
	_attach_child(pWnd, parent);
	return pWnd;
}


CUIMMShniaga* CScriptXmlInit::InitMMShniaga(LPCSTR path, CUIWindow* parent)
{
	CUIMMShniaga* pWnd = new CUIMMShniaga();
	pWnd->InitShniaga(m_xml, path);
	_attach_child(pWnd, parent);
	return pWnd;
}

CUIWindow* CScriptXmlInit::InitKeyBinding(LPCSTR path, CUIWindow* parent){
	CUIKeyBinding* pWnd				= new CUIKeyBinding();
	pWnd->InitFromXml				(m_xml, path);	
	_attach_child					(pWnd, parent);
	return							pWnd;
}

CUITrackBar* CScriptXmlInit::InitTrackBar(LPCSTR path, CUIWindow* parent)
{
	CUITrackBar* pWnd = new CUITrackBar();
	CUIXmlInit::InitTrackBar(m_xml, path, 0, pWnd);
	_attach_child(pWnd, parent);
	return pWnd;
}

CUIProgressBar* CScriptXmlInit::InitProgressBar(LPCSTR path, CUIWindow* parent)
{
	CUIProgressBar* pWnd = new CUIProgressBar();
	CUIXmlInit::InitProgressBar(m_xml, path, 0, pWnd);
	_attach_child(pWnd, parent);
	return pWnd;
}



SCRIPT_EXPORT(CScriptXmlInit, (), {
	module(luaState)[class_<CScriptXmlInit>("CScriptXmlInit")
						 .def(constructor<>())
						 .def("ParseFile", &CScriptXmlInit::ParseFile)
						 .def("NodeExists", &CScriptXmlInit::NodeExists)
						 .def("InitWindow", &CScriptXmlInit::InitWindow)
						 .def("InitHint", &CScriptXmlInit::InitHint)
						 .def("InitFrame", &CScriptXmlInit::InitFrame)
						 .def("InitFrameLine", &CScriptXmlInit::InitFrameLine)
						 .def("InitEditBox", &CScriptXmlInit::InitEditBox)
						 .def("InitStatic", &CScriptXmlInit::InitStatic)
						 .def("InitTextWnd", &CScriptXmlInit::InitTextWnd)
						 .def("InitAnimStatic", &CScriptXmlInit::InitAnimStatic)
						 .def("InitSleepStatic", &CScriptXmlInit::InitSleepStatic)
						 .def("Init3tButton", &CScriptXmlInit::Init3tButton)
						 .def("InitCheck", &CScriptXmlInit::InitCheck)
						 .def("InitSpinNum", &CScriptXmlInit::InitSpinNum)
						 .def("InitSpinFlt", &CScriptXmlInit::InitSpinFlt)
						 .def("InitSpinText", &CScriptXmlInit::InitSpinText)
						 .def("InitComboBox", &CScriptXmlInit::InitComboBox)
						 .def("InitTab", &CScriptXmlInit::InitTab)
						 .def("InitTrackBar", &CScriptXmlInit::InitTrackBar)
						 .def("InitKeyBinding", &CScriptXmlInit::InitKeyBinding)
						 .def("InitMMShniaga", &CScriptXmlInit::InitMMShniaga)
						 .def("InitScrollView", &CScriptXmlInit::InitScrollView)
						 .def("InitListBox", &CScriptXmlInit::InitListBox)
						 .def("InitProgressBar", &CScriptXmlInit::InitProgressBar)];
});
