#include "core.h"
#include "creature.h"

using namespace nwnx::core;
using namespace nwnx::creature;

namespace nwnx { namespace combat_stat {

unsigned char d_ret_code_getbab[0x20];
CNWSStats_Level* (*GetLevelStats)(CNWSCreatureStats*, uint8_t) = (CNWSStats_Level* (*)(CNWSCreatureStats*, uint8_t))0x08163ec8;
int (*GetBAB_Org)(CNWSCreatureStats*, int)  = (int (*)(CNWSCreatureStats*, int))&d_ret_code_getbab;
int GetBAB_HookProc(CNWSCreatureStats* stats, int pre_epic_only)
{
	int nBAB = GetBAB_Org(stats, pre_epic_only);
	if (stats->cs_is_pc && !pre_epic_only)
	{
		uint8_t nMaxLevel = GetLevel(stats, 0);
		//if (pre_epic_only && nMaxLevel > 20) nMaxLevel = 20;
		uint8_t nBGLevel = 0;
		for (uint8_t nLevel=0; nLevel<nMaxLevel; nLevel++)
		{
			CNWSStats_Level* level = GetLevelStats(stats, nLevel);
			if (level != NULL && level->ls_class == CLASS_TYPE_BLACKGUARD) nBGLevel++;
		}
		if (nBGLevel)
		{
			nBAB += 1;
			nBAB += nBGLevel / 10;
		}
	}
	return nBAB;
}

void init()
{
	hook_function(0x08142054, (unsigned long)GetBAB_HookProc, d_ret_code_getbab, 12);
}
REGISTER_INIT(init);
	
}
}