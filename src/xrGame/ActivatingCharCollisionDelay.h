#pragma once

#include "Common/Noncopyable.hpp"

class CCharacterPhysicsSupport;
class activating_character_delay : private Noncopyable
{
	CCharacterPhysicsSupport& char_support;
	u32 activate_time;
	static const u32 delay = 3000;

public:
	activating_character_delay(CCharacterPhysicsSupport* char_support_);
	void update();
	bool active();

private:
	bool do_position_correct();
};
