#include "StdAfx.h"

#include "MMSound.h"
#include "xrUICore/XML/xrUIXmlParser.h"

CMMSound::~CMMSound()
{
	all_Stop();
}

void CMMSound::Init(CUIXml& xml_doc, LPCSTR path)
{
	string256 _path;
	m_bRandom = xml_doc.ReadAttribInt(path, 0, "random") ? true : false;

	m_bIgnorePaused = xml_doc.ReadAttribInt(path, 0, "ignore_paused") ? true : false;

	int nodes_num = xml_doc.GetNodesNum(path, 0, "menu_music");

	XML_NODE tab_node = xml_doc.NavigateToNode(path, 0);
	xml_doc.SetLocalRoot(tab_node);
	for (int i = 0; i < nodes_num; ++i)
		m_play_list.push_back(xml_doc.Read("menu_music", i, ""));
	xml_doc.SetLocalRoot(xml_doc.GetRoot());

	strconcat(sizeof(_path), _path, path, ":whell_sound");
	if (check_file(xml_doc.Read(_path, 0, "")))
		m_whell.create(xml_doc.Read(_path, 0, ""), st_Effect, sg_SourceType);

	strconcat(sizeof(_path), _path, path, ":whell_click");
	if (check_file(xml_doc.Read(_path, 0, "")))
		m_whell_click.create(xml_doc.Read(_path, 0, ""), st_Effect, sg_SourceType);
}

bool CMMSound::check_file(LPCSTR fname)
{
	string_path _path;
	strconcat(sizeof(_path), _path, fname, ".ogg");
	return FS.exist("$game_sounds$", _path) ? true : false;
}

void CMMSound::whell_Play()
{
	if (m_whell._handle() && !m_whell._feedback())
		m_whell.play(nullptr, sm_Looped | sm_2D);
}

void CMMSound::whell_Stop()
{
	if (m_whell._feedback())
		m_whell.stop();
}

void CMMSound::whell_Click()
{
	if (m_whell_click._handle())
		m_whell_click.play(nullptr, sm_2D);
}

void CMMSound::whell_UpdateMoving(float frequency)
{
	m_whell.set_frequency(frequency);
}

void CMMSound::music_Play()
{
	if (m_play_list.empty())
		return;

	int i = Random.randI(m_play_list.size());

	string_path _path;
	strconcat(sizeof(_path), _path, m_play_list[i].c_str(), ".ogg");
	VERIFY(FS.exist("$game_sounds$", _path));

	m_music_stereo.create(_path, st_Music, sg_SourceType);
	m_music_stereo.play(nullptr, sm_2D);
	m_music_stereo.SetIgnorePaused(m_bIgnorePaused);
}

void CMMSound::music_Update()
{


	if (m_music_stereo.isPlaying() == false)
		music_Play();
}

void CMMSound::music_Stop()
{
	m_music_stereo.stop();
}

void CMMSound::all_Stop()
{
	music_Stop();
	m_whell.stop();
	m_whell_click.stop();
}
