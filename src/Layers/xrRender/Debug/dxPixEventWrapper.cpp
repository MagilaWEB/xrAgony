#include "stdafx.h"
#include "dxPixEventWrapper.h"

#ifdef DEBUG

CScopedPixEvent::CScopedPixEvent(LPCWSTR eventName)
{
	HW.UserDefinedAnnotation->BeginEvent(eventName);
}

CScopedPixEvent::~CScopedPixEvent()
{
	HW.UserDefinedAnnotation->EndEvent();
}

void dxPixSetDebugName(ID3DDeviceChild* resource, const shared_str& name)
{
	resource->SetPrivateData(WKPDID_D3DDebugObjectName, name.size(), name.c_str());
}

#endif //	DEBUG
