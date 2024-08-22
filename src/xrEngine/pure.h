#pragma once
#include "xrCommon/xr_vector.h"

// messages
#define REG_PRIORITY_LOW 0x11111111ul
#define REG_PRIORITY_NORMAL 0x22222222ul
#define REG_PRIORITY_HIGH 0x33333333ul
#define REG_PRIORITY_CAPTURE 0x7ffffffful
#define REG_PRIORITY_INVALID 0xfffffffful
#define DECLARE_MESSAGE(name)\
struct pure##name\
{\
    virtual void On##name() = 0;\
    ICF void __stdcall OnPure() { On##name(); }\
}

DECLARE_MESSAGE(Frame); // XXX: rename to FrameStart
DECLARE_MESSAGE(FrameEnd);
DECLARE_MESSAGE(Render);
DECLARE_MESSAGE(AppActivate);
DECLARE_MESSAGE(AppDeactivate);
DECLARE_MESSAGE(AppStart);
DECLARE_MESSAGE(AppEnd);
DECLARE_MESSAGE(DeviceReset);
DECLARE_MESSAGE(ScreenResolutionChanged);

template<class T>
class MessageRegistry
{
	struct MessageObject
	{
		T* Object;
		int Prio;
	};

	bool changed{ false };
	xr_vector<MessageObject> messages;

public:
	MessageRegistry() = default;

	void Clear()
	{
		messages.clear();
	}

	constexpr void Add(T* object, const int priority = REG_PRIORITY_NORMAL)
	{
		VERIFY(object);

		messages.emplace_back(object, priority);

		changed = true;
	}

	void Remove(T* object)
	{
		for (MessageObject& msg : messages)
		{
			if (msg.Object == object)
			{
				msg.Prio = REG_PRIORITY_INVALID;
				break;
			}
		}

		changed = true;
	}

	void Process()
	{
		if (changed)
			Resort();

		if (messages.empty())
			return;


		if (messages.front().Prio == REG_PRIORITY_CAPTURE)
			messages.front().Object->OnPure();
		else
		{
			for (size_t i = 0; i < messages.size(); ++i)
			{
				auto& message = messages[i];
				if (message.Prio != REG_PRIORITY_INVALID)
					message.Object->OnPure();
			}
		}
	}

private:
	void Resort()
	{
		if (!messages.empty())
		{
			messages.sort([](MessageObject& msg, MessageObject& msg2) {
				return msg.Prio > msg2.Prio;
			});
		}

		while (!messages.empty() && messages.back().Prio == REG_PRIORITY_INVALID)
			messages.pop_back();

		if (messages.empty())
			messages.shrink_to_fit();

		changed = false;
	}
};
