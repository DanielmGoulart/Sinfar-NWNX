#include "core.h"
#include "creature.h"
#include "nwscript.h"
#include "cached2da.h"

using namespace nwnx::core;
using namespace nwnx::creature;
using namespace nwnx::nwscript;
using namespace nwnx::cached2da;

namespace nwnx { namespace feat {

int HasFeat_ReturnFalse(CNWSCreatureStats* stats)
{
	return false;
}
CNWSStats_Level* (*GetLevelStats)(CNWSCreatureStats*, uint8_t) = (CNWSStats_Level* (*)(CNWSCreatureStats*, uint8_t))0x08163ec8;
char get_divine_save_bonus(CNWSCreatureStats* stats)
{
	char ret = 0;
	bool has_divine_grace = has_feat(stats, 217);
	bool has_dark_blessing = has_feat(stats, 473);
	if (has_divine_grace || has_dark_blessing)
	{
		uint8_t paladin_level = 0;
		uint8_t blackguard_level = 0;
		uint8_t max_level = GetLevel(stats, 0);
		for (uint8_t level=0; level<max_level; level++)
		{
			CNWSStats_Level* level_stats = GetLevelStats(stats, level);
			if (level_stats)
			{
				if (level_stats->ls_class == CLASS_TYPE_BLACKGUARD)
				{
					blackguard_level++;
				}
				else if(level_stats->ls_class == CLASS_TYPE_PALADIN)
				{
					paladin_level++;
				}
				else if (level_stats->ls_class == CLASS_TYPE_DIVINECHAMPION)
				{
					paladin_level++;
					blackguard_level++;
				}
			}
		}
		if (has_divine_grace)
		{
			uint8_t cap = 4 + (paladin_level / 4);
			ret += (stats->cs_cha_mod > cap ? cap : stats->cs_cha_mod);
		}
		if (has_dark_blessing)
		{
			uint8_t cap = 4 + (blackguard_level / 4);
			ret += (stats->cs_cha_mod > cap ? cap : stats->cs_cha_mod);
		}
	}
	return ret;
}
char (*GetBaseFort)(CNWSCreatureStats*) = (char (*)(CNWSCreatureStats*))0x81529F0;
char OnGetFortSave_GetGase(CNWSCreatureStats* stats)
{
	char ret = GetBaseFort(stats);
	ret += get_divine_save_bonus(stats);
	return ret;
}
char (*GetBaseWill)(CNWSCreatureStats*) = (char (*)(CNWSCreatureStats*))0x08152B4C;
char OnGetWillSave_GetGase(CNWSCreatureStats* stats)
{
	char ret = GetBaseWill(stats);
	ret += get_divine_save_bonus(stats);
	return ret;
}
char (*GetBaseReflex)(CNWSCreatureStats*) = (char (*)(CNWSCreatureStats*))0x08152CC0;
char OnGetReflexSave_GetGase(CNWSCreatureStats* stats)
{
	char ret = GetBaseReflex(stats);
	ret += get_divine_save_bonus(stats);
	return ret;
}

unsigned char d_ret_code_featremaininguses[0x20];
uint8_t (*GetFeatRemainingUses_Org)(CNWSCreatureStats*, uint16_t) = (uint8_t (*)(CNWSCreatureStats*, uint16_t))&d_ret_code_featremaininguses;
uint8_t GetFeatRemainingUses_Hook(CNWSCreatureStats* stats, uint16_t feat)
{
	C2da* feats_uses_2da = get_cached_2da("feats_uses");
	if (feats_uses_2da)
	{
		std::string feats_uses_str = feats_uses_2da->GetString("USES", feat);
		if (!feats_uses_str.empty())
		{
			return atoi(feats_uses_str.c_str());
		}
	}
	return GetFeatRemainingUses_Org(stats, feat);
}

void OnApplyDisarm_HookProc(void* effects_list, CNWSObject* target, CGameEffect* effect, int p3)
{
	run_script("s_ev_disarm", effect->eff_creator);
}

void init()
{
	//unlimited use for defensive stance
	hook_function(0x08153e00, (long)GetFeatRemainingUses_Hook, d_ret_code_featremaininguses, 12);

	//cap paladin/blackguard save bonus
	hook_call(0x08152ABB, (uint32_t)HasFeat_ReturnFalse);
	hook_call(0x08152C17, (uint32_t)HasFeat_ReturnFalse);
	hook_call(0x08152D8B, (uint32_t)HasFeat_ReturnFalse);
	hook_call(0x0816503E, (uint32_t)HasFeat_ReturnFalse);
	hook_call(0x08164FA2, (uint32_t)HasFeat_ReturnFalse);
	hook_call(0x0815DBBC, (uint32_t)HasFeat_ReturnFalse);
	hook_call(0x08164FBA, (uint32_t)OnGetFortSave_GetGase);
	hook_call(0x08165056, (uint32_t)OnGetWillSave_GetGase);
	hook_call(0x0815DBE0, (uint32_t)OnGetReflexSave_GetGase);
	
	//quilvering palm ignore level difference
	enable_write(0x080ea685);
	*(uint16_t*)(0x080ea685) = 0xE990;
	enable_write(0x080ed2f8);
	for (int i=0; i<6; i++) *(uint8_t*)(0x080ed2f8+i) = 0x90;
	
	//change disarm weapon mod bonus to *1 ab instead of *4
	enable_write(0x08149044);
	*(uint16_t*)0x08149044 = 0xdf89;
	for (int i=0; i<5; i++) *(uint8_t*)(0x08149044+2+i) = 0x90;
	
	hook_function(0x08172004, (unsigned long)OnApplyDisarm_HookProc, d_ret_code_nouse, 12);
}
REGISTER_INIT(init);	
	
}
}