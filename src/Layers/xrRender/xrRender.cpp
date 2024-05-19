#include "stdafx.h"
#include "Layers/xrRender/dxRenderFactory.h"
#include "Layers/xrRender/dxUIRender.h"
#include "Layers/xrRender/dxDebugRender.h"
#include "Layers/xrAPI/xrAPI.h"

extern "C"
{
XR_EXPORT void SetupEnv()
{
    ::Render = &RImplementation;
    ::RenderFactory = &RenderFactoryImpl;
    ::DU = &DUImpl;
    ::UIRender = &UIRenderImpl;
#ifdef DEBUG
    ::DRender = &DebugRenderImpl;
#endif
    xrRender_initconsole();
}

XR_EXPORT bool CheckRendererSupport()
{
    return xrRender_test_hw() ? true : false;
}
}
