#include "stdafx.h"
#include "xrEngine\x_ray.h"

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
		return false;
	}
	case WM_KILLFOCUS:
	{
		result = 1;
		return true;
	}
	case WM_ACTIVATE:
	{
		OnWM_Activate(wParam, lParam);
		return false;
	}
	case WM_SETCURSOR:
	{
		result = 1;
		return true;
	}
	case WM_SYSCOMMAND:
	{
		// Prevent moving/sizing and power loss in fullscreen mode
		switch (wParam)
		{
		case SC_MOVE:
		case SC_SIZE:
		case SC_MAXIMIZE:
		case SC_MONITORPOWER: result = 1; return (true);
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
			result = 1;
			return false;
		}
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
	LRESULT result;
	if (Device.on_message(hWnd, uMsg, wParam, lParam, result))
		return (result);

	return (DefWindowProc(hWnd, uMsg, wParam, lParam));
}
