#include "core.h"
#include "creature.h"
#include "nwscript.h"

using namespace nwnx::core;
using namespace nwnx::creature;
using namespace nwnx::nwscript;

namespace nwnx { namespace mode {

int (*SetCombatMode)(CNWSCreature*, uint8_t, int) = (int (*)(CNWSCreature*, uint8_t, int))0x080f982c;
uint8_t (*GetMode)(CNWSCreature*, uint8_t) = (uint8_t (*)(CNWSCreature*, uint8_t))0x0812c078;
void OnMove_SetCombatMode(CNWSCreature* creature, uint8_t p2, int p3)
{
	if (GetMode(creature, 0xc)) //defensive stance
	{
		SetCombatMode(creature, p2, p3);
	}
}

void init()
{
	//do not un-toggle modes when moving
	enable_write(0x08190127);
	*(uint32_t*)(0x08190127) = ((uint32_t)OnMove_SetCombatMode-(uint32_t)0x0819012b);
	enable_write(0x0818f998);
	*(uint32_t*)(0x0818f998) = ((uint32_t)OnMove_SetCombatMode-(uint32_t)0x0818f99c);
	enable_write(0x080e1a55);
	*(uint32_t*)(0x080e1a55) = ((uint32_t)OnMove_SetCombatMode-(uint32_t)0x080e1a59);
	//in AIActionAttackObject
	enable_write(0x080e59a9);
	*(uint32_t*)(0x080e59a9) = ((uint32_t)OnMove_SetCombatMode-(uint32_t)0x080e59ad);
	enable_write(0x080e5a0f);
	*(uint32_t*)(0x080e5a0f) = ((uint32_t)OnMove_SetCombatMode-(uint32_t)0x080e5a13);
	enable_write(0x080e6142);
	*(uint32_t*)(0x080e6142) = ((uint32_t)OnMove_SetCombatMode-(uint32_t)0x080e6146);
	enable_write(0x080e61b2);
	*(uint32_t*)(0x080e61b2) = ((uint32_t)OnMove_SetCombatMode-(uint32_t)0x080e61b6);
	enable_write(0x080e6337);
	*(uint32_t*)(0x080e6337) = ((uint32_t)OnMove_SetCombatMode-(uint32_t)0x080e633b);
	enable_write(0x080e660e);
	*(uint32_t*)(0x080e660e) = ((uint32_t)OnMove_SetCombatMode-(uint32_t)0x080e6612);
	enable_write(0x080e6b5a);
	*(uint32_t*)(0x080e6b5a) = ((uint32_t)OnMove_SetCombatMode-(uint32_t)0x080e6b5e);
	enable_write(0x080e6bf9);
	*(uint32_t*)(0x080e6bf9) = ((uint32_t)OnMove_SetCombatMode-(uint32_t)0x080e6bfd);
}
REGISTER_INIT(init);
	
}
}