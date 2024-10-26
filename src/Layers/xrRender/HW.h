#pragma once

#include "HWCaps.h"
#include "xrCore/ModuleLookup.hpp"

#if !defined(_MAYA_EXPORT)
#include "stats_manager.h"
#endif

class CHW : public pureAppActivate, public pureAppDeactivate
{
	//	Functions section
public:
	CHW();
	~CHW();

	void CreateD3D();
	void DestroyD3D();

	void CreateDevice(HWND hw, bool move_window);

	void DestroyDevice();

	void Reset(HWND hw);

	void selectResolution(u32& dwWidth, u32& dwHeight, BOOL bWindowed);
	D3DFORMAT selectDepthStencil(D3DFORMAT);
	u32 selectPresentInterval();
	u32 selectGPU();
	u32 selectRefresh(u32 dwWidth, u32 dwHeight, D3DFORMAT fmt);
	BOOL support(D3DFORMAT fmt, DWORD type, DWORD usage);

	void updateWindowProps(HWND hw);
	void Validate(void) {};

	//	Variables section
public:
	IDXGIFactory1* m_pFactory = nullptr;
	IDXGIAdapter1* m_pAdapter = nullptr; //	pD3D equivalent
	ID3D11Device* pDevice = nullptr; //	combine with DX9 pDevice via typedef
	ID3D11DeviceContext* pContext = nullptr; //	combine with DX9 pDevice via typedef
	IDXGISwapChain* m_pSwapChain = nullptr;
	ID3D11RenderTargetView* pBaseRT = nullptr; //	combine with DX9 pBaseRT via typedef
	ID3D11DepthStencilView* pBaseZB = nullptr;
#ifdef DEBUG
	ID3DUserDefinedAnnotation* UserDefinedAnnotation = nullptr;
#endif

	CHWCaps Caps;

	D3D_DRIVER_TYPE m_DriverType;
	DXGI_SWAP_CHAIN_DESC m_ChainDesc; //	DevPP equivalent
	D3D_FEATURE_LEVEL FeatureLevel;

	#if !defined(_MAYA_EXPORT)
		stats_manager stats_manager;
	#endif

	void UpdateViews();

	DXGI_RATIONAL selectRefresh(u32 dwWidth, u32 dwHeight, DXGI_FORMAT fmt);

	virtual void OnAppActivate();
	virtual void OnAppDeactivate();

	int maxRefreshRate = 200; //ECO_RENDER add
};

extern ECORE_API CHW HW;