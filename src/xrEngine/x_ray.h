#ifndef __X_RAY_H__
#define __X_RAY_H__

#include "xrEngine/ILoadingScreen.h"

constexpr u16 elementary_load_stage_limit = 100;
constexpr u16 reset_load_stage_limit = 70;
constexpr u16 restart_load_stage_limit = 20;
// refs
class ENGINE_API CGameFont;

// definition
class ENGINE_API CApplication : public pureFrame, public IEventReceiver
{
	// levels
	struct sLevelInfo
	{
		char* folder;
		char* name;
	};

private:
	ILoadingScreen* loadingScreen;

	u16 max_load_stage;
	u16 load_stage;

	bool loaded{ false };
	bool m_new_game{ false };

private:
	EVENT eQuit;
	EVENT eStart;
	//EVENT eStartLoad;
	EVENT eDisconnect;
	EVENT eConsole;
	EVENT eQuickLoad;

	void Level_Append(LPCSTR lname);

public:
	CGameFont* pFontSystem;

	IC const bool IsLoaded() { return loaded; }
	// Levels
	xr_vector<sLevelInfo> Levels;
	u32 Level_Current;
	void Level_Scan();
	int Level_ID(LPCSTR name, LPCSTR ver, bool bSet);
	void Level_Set(u32 ID);
	void LoadAllArchives();
	static CInifile* GetArchiveHeader(LPCSTR name, LPCSTR ver);

	// Loading
	IC ILoadingScreen* LoadScreen()
	{
		if (loadingScreen)
			return loadingScreen;

		return nullptr;
	}

	void CheckMaxLoad();
	void LoadBegin();
	void LoadEnd();

	void load_draw_internal();

	IC const int GetLoadState()
	{
		return load_stage;
	}

	IC void SetLoadStateMax(u16 max_stage)
	{
		max_load_stage = max_stage;
	}

	IC const int GetLoadStateMax()
	{
		return max_load_stage;
	}

	void SetLoadStageTitle(pcstr ls_title = nullptr);

	IC const bool IsNewGame()
	{
		return m_new_game;
	}

	IC void NewGame(bool state)
	{
		m_new_game = state;
	}

	virtual void OnEvent(EVENT E, u64 P1, u64 P2);

	// Other
	CApplication();
	virtual ~CApplication();

	virtual void OnFrame();

	void SetLoadingScreen(std::function<void(ILoadingScreen*&)> func);
	void DestroyLoadingScreen();
	bool IsLoadingScreen();
};

extern ENGINE_API CApplication* pApp;

#endif //__XR_BASE_H__
