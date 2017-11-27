#include "core.h"
#include "creature.h"
#include "nwscript.h"

using namespace nwnx::core;
using namespace nwnx::creature;
using namespace nwnx::nwscript;

namespace nwnx { namespace extra_stats {

unsigned char d_ret_code_totaleffectbonus[0x20];
int (*GetTotalEffectBonus_Org)(CNWSCreature*, uint8_t, CNWSObject*, int, int, uint8_t, uint8_t, uint8_t, uint8_t, int) =
	(int (*)(CNWSCreature*, uint8_t, CNWSObject*, int, int, uint8_t, uint8_t, uint8_t, uint8_t, int))&d_ret_code_totaleffectbonus;
int GetTotalEffectBonus_Hook(CNWSCreature* creature, uint8_t effect_type, CNWSObject* object, int p4, int p5, uint8_t p6, uint8_t p7, uint8_t skill, uint8_t ability, int p10)
{
	int result = GetTotalEffectBonus_Org(creature, effect_type, object, p4, p5, p6, p7, skill, ability, p10);
	CREATURE_EXTRA* creature_extra = GetCreatureExtra(creature);
	if (effect_type==4)
	{
		result += creature_extra->extra_ability_bonus[ability];
	}
	else if (effect_type==5)
	{
		auto iter = creature_extra->extra_skill_bonus.find(skill);
		if (iter != creature_extra->extra_skill_bonus.end())
		{
			result += iter->second;
		}
	}
	return result;
}
	
void init()
{
	hook_function(0x8132298, (unsigned long)GetTotalEffectBonus_Hook, d_ret_code_totaleffectbonus, 12);	
}
REGISTER_INIT(init);

VM_FUNC_NEW(AddExtraFeat, 8)
{
	uint32_t creature_id = vm_pop_object();
	int feat_id = vm_pop_int();
	CNWSCreature* creature = GetCreatureById(creature_id);
	if (creature)
	{
		if (GetCreatureExtra(creature)->extra_feats.insert(feat_id).second)
		{
			creature->cre_stats->cs_extra_feats.add(feat_id);
		}
	}
}
VM_FUNC_NEW(RemoveExtraFeat, 9)
{
	uint32_t creature_id = vm_pop_object();
	int feat_id = vm_pop_int();
	CNWSCreature* creature = GetCreatureById(creature_id);
	if (creature)
	{
		if (GetCreatureExtra(creature)->extra_feats.erase(feat_id))
		{
			creature->cre_stats->cs_extra_feats.delvalue(feat_id);
		}
	}
}
void (*SetSTRBase)(CNWSCreatureStats*, uint8_t) = (void (*)(CNWSCreatureStats*, uint8_t))0x0815193c;
void (*SetCONBase)(CNWSCreatureStats*, uint8_t, int) = (void (*)(CNWSCreatureStats*, uint8_t, int))0x081519f0;
void (*SetINTBase)(CNWSCreatureStats*, uint8_t) = (void (*)(CNWSCreatureStats*, uint8_t))0x08151afc;
void (*SetWISBase)(CNWSCreatureStats*, uint8_t) = (void (*)(CNWSCreatureStats*, uint8_t))0x08151bb0;
void (*SetCHABase)(CNWSCreatureStats*, uint8_t) = (void (*)(CNWSCreatureStats*, uint8_t))0x08151c64;
void (*SetDEXBase)(CNWSCreatureStats*, uint8_t) = (void (*)(CNWSCreatureStats*, uint8_t))0x0816446c;
VM_FUNC_NEW(ApplyExtraAbilityBonus, 10)
{
	uint32_t creature_id = vm_pop_object();
	int ability = vm_pop_int();
	int modifier = vm_pop_int();
	if (ability >=0 && ability <= 5)
	{
		CNWSCreature* creature = GetCreatureById(creature_id);
		if (creature)
		{
			CNWSCreatureStats* creature_stats = creature->cre_stats;
            GetCreatureExtra(creature_stats)->extra_ability_bonus[ability] += modifier;
            switch(ability)
            {
                case ABILITY_STRENGTH: SetSTRBase(creature_stats, creature_stats->cs_str); break;
                case ABILITY_DEXTERITY: SetDEXBase(creature_stats, creature_stats->cs_dex); break;
                case ABILITY_CONSTITUTION: SetCONBase(creature_stats, creature_stats->cs_con, true); break;
                case ABILITY_INTELLIGENCE: SetINTBase(creature_stats, creature_stats->cs_int); break;
                case ABILITY_WISDOM: SetWISBase(creature_stats, creature_stats->cs_wis); break;
                case ABILITY_CHARISMA: SetCHABase(creature_stats, creature_stats->cs_cha); break;
            }
			creature->cre_updatecombatinfo = true;
		}
	}
}
VM_FUNC_NEW(ResetExtraStats, 11)
{
	uint32_t creature_id = vm_pop_object();
	CNWSCreature* creature = GetCreatureById(creature_id);
	if (creature)
	{
		CREATURE_EXTRA* creature_extra = GetCreatureExtra(creature);
		for (int ability=0; ability<6; ability++)
		{
			creature_extra->extra_ability_bonus[ability] = 0;
		}
		for (auto& feat : creature_extra->extra_feats)
		{
			creature->cre_stats->cs_extra_feats.delvalue(feat);
		}
		creature_extra->extra_feats.clear();
		creature_extra->extra_skill_bonus.clear();
	}
}
VM_FUNC_NEW(ApplyExtraSkillBonus, 12)
{
	uint32_t creature_id = vm_pop_object();
	int skill = vm_pop_int();
	int modifier = vm_pop_int();
	CNWSCreature* creature = GetCreatureById(creature_id);
	if (creature)
	{
		GetCreatureExtra(creature)->extra_skill_bonus[skill] += modifier;
	}
}
	
}
}