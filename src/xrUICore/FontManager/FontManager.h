#pragma once


struct XRUICORE_API CFontManager : public pureDeviceReset
{
	CFontManager();
	~CFontManager();

	struct DataInfo
	{
		LPCSTR section;
		u16 size_min;
		u16 size_max;
	};

	using DATA_FONT = xr_map<LPCSTR, LPCSTR>;

	// hud font
	CGameFont* pFontMedium;
	CGameFont* pFontDI;

	CGameFont* pFontArial14;
	CGameFont* pFontGraffiti19Russian;
	CGameFont* pFontGraffiti22Russian;
	CGameFont* pFontLetterica16Russian;
	CGameFont* pFontLetterica18Russian;
	CGameFont* pFontGraffiti32Russian;
	CGameFont* pFontGraffiti50Russian;
	CGameFont* pFontLetterica25;
	CGameFont* pFontStat;

	void InitializeFonts();

	CGameFont* New(LPCSTR name_id, LPCSTR section, u32 flags = 0);

	CGameFont* Get(LPCSTR name_id);
	CGameFont* Clone(LPCSTR name_id, LPCSTR id_clone, u16 size);
	bool IsData(LPCSTR name_id);
	std::pair<LPCSTR, LPCSTR> GetDataFont(const LPCSTR name_id) const;
	DATA_FONT GetData() const;
	void Render();
	virtual void OnDeviceReset();
	
private:
	DATA_FONT FontsData;
	xr_map<LPCSTR, CGameFont*> pFonts;
};
