#include "stdafx.h"
#include "xrEngine\x_ray.h"
#include "xrEngine\xr_input.h"

bool CRenderDevice::on_message(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT& result)
{
	switch (uMsg)
	{
		case WM_SYSKEYDOWN:
		{
			return false;
		}
		case WM_SETFOCUS:
		{
			pInput->ShowCursor(false);
			AltTab(false);
			::ShowWindow(m_hWnd, SW_SHOWNORMAL);
			return true;
		}
		case WM_KILLFOCUS:
		{
			pInput->ShowCursor(true);
			return false;
		}
		case WM_ACTIVATE:
		{
			OnWM_Activate(wParam, lParam);
			return false;
		}
		case WM_SETCURSOR:
		{
			return true;
		}
		case WM_NCLBUTTONDOWN:
		{
			if (wParam == HTCLOSE)
				SendMessage(m_hWnd, WM_CLOSE, 0, 0);
			else if (wParam == HTMINBUTTON)
				ShowWindow(m_hWnd, SW_MINIMIZE);
			else if (wParam == HTMAXBUTTON)
				ShowWindow(m_hWnd, SW_MAXIMIZE);
		
			return true;
		}
		case WM_SYSCOMMAND:
		{
			switch (wParam)
			{
				case SC_MOVE: return true;
				case SC_SIZE: return true;
				case SC_MAXIMIZE:
				case SC_MONITORPOWER: return true;
			}
			return false;
		}
		case WM_CLOSE:
		{
			ILoadingScreen* screen = pApp->LoadScreen();
			if (screen && !screen->IsVisibility())
			{
				seqParallel.clear();
				seqParallel2.clear();
				xrThread::GlobalState(xrThread::dsOK);
				Engine.Event.Defer("KERNEL:disconnect");
				Engine.Event.Defer("KERNEL:quit");
			}

			return true;
		}
		case WM_CANCELMODE:
		{
			AltTab(true);
			ShowWindow(m_hWnd, SW_MINIMIZE);
			return true;
		}
	}

	return (false);
}
//-----------------------------------------------------------------------------
// Name: WndProc()
// Desc: Static msg handler which passes messages to the application class.
//-----------------------------------------------------------------------------
LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	LRESULT result{ 0 };
	if (Device.on_message(hWnd, uMsg, wParam, lParam, result))
		return (result);

	return (DefWindowProc(hWnd, uMsg, wParam, lParam));
}
