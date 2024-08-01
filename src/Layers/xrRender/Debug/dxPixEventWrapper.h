#pragma once

#if defined(DEBUG)

class CScopedPixEvent
{
public:
	CScopedPixEvent(LPCWSTR eventName);
	~CScopedPixEvent();
};

#define PIX_EVENT(Name) CScopedPixEvent pixEvent##Name(L#Name)

// You shouldn't use this macro more than one time in one scope.
#define PIX_EVENT_TEXT(evtName) CScopedPixEvent pixLocalEvt(evtName)

void dxPixSetDebugName(ID3DDeviceChild* resource, const shared_str& name);

#define SET_DEBUG_NAME(resource, name) dxPixSetDebugName(resource, name)

#else //	DEBUG

#define PIX_EVENT(Name) {}

// You shouldn't use this macro more than one time in one scope.
#define PIX_EVENT_TEXT(evtName) {}

#define SET_DEBUG_NAME(resource, name) {}

#endif //	DEBUG
