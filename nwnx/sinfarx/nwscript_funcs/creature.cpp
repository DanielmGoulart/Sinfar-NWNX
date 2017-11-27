#include "nwscript_funcs.h"
#include <cmath>

namespace {

VM_FUNC_NEW(GetKnowsFeat, 426)
{
	CNWSCreature* creature = GetCreatureById(vm_pop_object());
	uint32_t feat_id = vm_pop_int();
	if (creature)
	{
		uint16_t* cs_feats_data = creature->cre_stats->cs_feats.data;
		uint16_t* cs_feats_data_last = cs_feats_data + creature->cre_stats->cs_feats.len;
		while (cs_feats_data < cs_feats_data_last)
		{
			if (*cs_feats_data == feat_id)
			{
				vm_push_int(1);
				return;
			}
			cs_feats_data++;
		}
	}
	vm_push_int(0);
}


VM_FUNC_NEW(RemoveFeatFromLevelStats, 13)
{
	uint32_t creature_id = vm_pop_object();
	uint16_t feat = vm_pop_int();
	int level = vm_pop_int();
	CNWSCreature* creature = GetCreatureById(creature_id);
	if (creature)
	{
		if (level > 0)
		{
			uint8_t level_index = level-1;
			if (level_index < creature->cre_stats->cs_levelstat.len)
			{
				CNWSStats_Level* level_stats = static_cast<CNWSStats_Level*>(creature->cre_stats->cs_levelstat.data[level_index]);
				level_stats->ls_featlist.delvalue(feat);
			}
		}
		else if (level == -1)
		{
			for (uint8_t level_index=0; level_index<creature->cre_stats->cs_levelstat.len; level_index++)
			{
				CNWSStats_Level* level_stats = static_cast<CNWSStats_Level*>(creature->cre_stats->cs_levelstat.data[level_index]);
				level_stats->ls_featlist.delvalue(feat);
			}
		}
	}
}

void (*SetSTRBase)(CNWSCreatureStats*, uint8_t) = (void (*)(CNWSCreatureStats*, uint8_t))0x0815193c;
void (*SetCONBase)(CNWSCreatureStats*, uint8_t, int) = (void (*)(CNWSCreatureStats*, uint8_t, int))0x081519f0;
void (*SetINTBase)(CNWSCreatureStats*, uint8_t) = (void (*)(CNWSCreatureStats*, uint8_t))0x08151afc;
void (*SetWISBase)(CNWSCreatureStats*, uint8_t) = (void (*)(CNWSCreatureStats*, uint8_t))0x08151bb0;
void (*SetCHABase)(CNWSCreatureStats*, uint8_t) = (void (*)(CNWSCreatureStats*, uint8_t))0x08151c64;
void (*SetDEXBase)(CNWSCreatureStats*, uint8_t) = (void (*)(CNWSCreatureStats*, uint8_t))0x0816446c;
VM_FUNC_NEW(SetAbilityScore, 428)
{
	CNWSCreature* creature = GetCreatureById(vm_pop_object());
	int ability = vm_pop_int();
	int val = vm_pop_int();
	if (creature)
	{
		switch (ability) {
			case ABILITY_STRENGTH:
				SetSTRBase(creature->cre_stats, val);
				break;
			case ABILITY_DEXTERITY:
				SetDEXBase(creature->cre_stats, val);
				break;
			case ABILITY_CONSTITUTION:
				SetCONBase(creature->cre_stats, val, 0);
				break;
			case ABILITY_INTELLIGENCE:
				SetINTBase(creature->cre_stats, val);
				break;
			case ABILITY_WISDOM:
				SetWISBase(creature->cre_stats, val);
				break;
			case ABILITY_CHARISMA:
				SetCHABase(creature->cre_stats, val);
				break;
		}
	}
}
VM_FUNC_NEW(SetSkillRank, 430)
{
    CNWSCreature* creature = GetCreatureById(vm_pop_object());
	uint32_t skill = vm_pop_int();
	int val = vm_pop_int();
    if (creature && skill < nwn_rules->ru_skills_len)
    {
        creature->cre_stats->cs_skills[skill] = val;
    }
}
VM_FUNC_NEW(GetACNaturalBase, 431)
{
    CNWSCreature* creature = GetCreatureById(vm_pop_object());
    vm_push_int(creature ? creature->cre_stats->cs_ac_natural_base : 0);
}
VM_FUNC_NEW(SetACNaturalBase, 432)
{
    CNWSCreature* creature = GetCreatureById(vm_pop_object());
    int value = vm_pop_int();
    if (creature)
    {
        creature->cre_stats->cs_ac_natural_base = value;
    }
}
CNWSStats_Level* get_level_status(CNWSCreatureStats* stats, uint32_t level)
{
	level--;
    if (level < stats->cs_levelstat.len)
	{
    	return (CNWSStats_Level*)stats->cs_levelstat.data[level];
	}
	else
	{
		return NULL;
	}
}
void (*AddFeat_ToLevel)(CNWSStats_Level*, uint16_t) = (void (*)(CNWSStats_Level*, uint16_t))0x080bfa84;
void (*AddFeat_ToCreature)(CNWSCreatureStats*, uint16_t) = (void (*)(CNWSCreatureStats*, uint16_t))0x08153b14;
VM_FUNC_NEW(AddKnownFeat, 433)
{
    CNWSCreature* creature = GetCreatureById(vm_pop_object());
    uint16_t feat = vm_pop_int();
    uint32_t level_index = vm_pop_int();
    if (creature)
    {
        AddFeat_ToCreature(creature->cre_stats, feat);
        CNWSStats_Level* level = get_level_status(creature->cre_stats, level_index);
        if (level)
        {
            AddFeat_ToLevel(level, feat);
        }
    }
}
void (*RemoveFeat_FromCreature)(CNWSCreatureStats*, uint16_t) = (void (*)(CNWSCreatureStats*, uint16_t))0x08164788;
VM_FUNC_NEW(RemoveKnownFeat, 434)
{
    CNWSCreature* creature = GetCreatureById(vm_pop_object());
    uint16_t feat = vm_pop_int();
    uint32_t level_index = vm_pop_int();
    if (creature)
    {
        RemoveFeat_FromCreature(creature->cre_stats, feat);
        CNWSStats_Level* level = get_level_status(creature->cre_stats, level_index);
        if (level)
        {
            level->ls_featlist.delvalue(feat);
        }
    }
}
VM_FUNC_NEW(GetTotalKnownFeats, 435)
{
    CNWSCreature* creature = GetCreatureById(vm_pop_object());
    uint32_t level_index = vm_pop_int();
    uint32_t result = 0;
    if (creature)
    {
        CNWSStats_Level* level = get_level_status(creature->cre_stats, level_index);
        if (level)
        {
            result = level->ls_featlist.len;
        }
        else
        {
            result = creature->cre_stats->cs_feats.len;
        }
    }
    vm_push_int(result);
}
VM_FUNC_NEW(GetKnownFeatByIndex, 436)
{
    CNWSCreature* creature = GetCreatureById(vm_pop_object());
    uint32_t feat_index = vm_pop_int();
    uint32_t level_index = vm_pop_int();
    uint16_t result = 0;
    if (creature)
    {
        CNWSStats_Level* level = get_level_status(creature->cre_stats, level_index);
        if (level)
        {
            if (feat_index < level->ls_featlist.len)
            {
                result = level->ls_featlist.data[feat_index];
            }
        }
        else
        {
            if (feat_index < creature->cre_stats->cs_feats.len)
            {
                result = creature->cre_stats->cs_feats.data[feat_index];
            }
        }
    }
    vm_push_int(result);
}
int (*GetFeatRemainingUses)(CNWSCreatureStats*, uint16_t) = (int (*)(CNWSCreatureStats*, uint16_t))0x08153e00;
VM_FUNC_NEW(GetFeatRemainingUses, 437)
{
    CNWSCreature* creature = GetCreatureById(vm_pop_object());
    uint16_t feat = vm_pop_int();
    vm_push_int(creature ? GetFeatRemainingUses(creature->cre_stats, feat) : 0);
}
int (*GetFeatTotalUses)(CNWSCreatureStats*, uint16_t) = (int (*)(CNWSCreatureStats*, uint16_t))0x0815479c;
VM_FUNC_NEW(GetFeatTotalUses, 438)
{
    CNWSCreature* creature = GetCreatureById(vm_pop_object());
    uint16_t feat = vm_pop_int();
    vm_push_int(creature ? GetFeatTotalUses(creature->cre_stats, feat) : 0);
}
void vm_func_new_get_level_info(std::function<void(CNWSStats_Level*)> push_level_info)
{
    CNWSCreature* creature = GetCreatureById(vm_pop_object());
    uint32_t level_index = vm_pop_int();
    if (creature)
    {
        push_level_info(get_level_status(creature->cre_stats, level_index));
    }
	else
	{
    	push_level_info(NULL);
	}
}
VM_FUNC_NEW(GetClassByLevel, 443)
{
    vm_func_new_get_level_info([](CNWSStats_Level* level) {
        vm_push_int(level ? level->ls_class : -1);
    });
}
VM_FUNC_NEW(GetAbilityIncreaseByLevel, 444)
{
    vm_func_new_get_level_info([](CNWSStats_Level* level) {
        vm_push_int(level ? level->ls_ability : -1);
    });
}
VM_FUNC_NEW(GetSkillIncreaseByLevel, 445)
{
    vm_func_new_get_level_info([](CNWSStats_Level* level) {
        uint32_t skill = vm_pop_int();
        if (level && skill < nwn_rules->ru_skills_len)
        {
            vm_push_int(level ? level->ls_skilllist[skill] : 0);
        }
    });
}
VM_FUNC_NEW(GetMaxHitPointsByLevel, 448)
{
    vm_func_new_get_level_info([](CNWSStats_Level* level) {
        vm_push_int(level ? level->ls_hp : -1);
    });
}
VM_FUNC_NEW(SetMaxHitPointsByLevel, 449)
{
    vm_func_new_get_level_info([](CNWSStats_Level* level) {
        int value = vm_pop_int();
        if (level)
        {
            level->ls_hp = value;
        }
    });
}

VM_FUNC_NEW(GetSavingThrowBonus, 446)
{
    CNWSCreature* creature = GetCreatureById(vm_pop_object());
    int save = vm_pop_int();
    int result = 0;
    if (creature)
    {
        switch (save) {
            case SAVING_THROW_FORT:
                result = creature->cre_stats->cs_save_fort;
                break;
            case SAVING_THROW_REFLEX:
                result = creature->cre_stats->cs_save_reflex;
                break;
            case SAVING_THROW_WILL:
                result = creature->cre_stats->cs_save_will;
                break;
        }
    }
    vm_push_int(result);
}
VM_FUNC_NEW(SetSavingThrowBonus, 447)
{
    CNWSCreature* creature = GetCreatureById(vm_pop_object());
    int save = vm_pop_int();
    int value = vm_pop_int();
    if (creature)
    {
        switch (save) {
            case SAVING_THROW_FORT:
                creature->cre_stats->cs_save_fort = value;
                break;
            case SAVING_THROW_REFLEX:
                 creature->cre_stats->cs_save_reflex = value;
                break;
            case SAVING_THROW_WILL:
                creature->cre_stats->cs_save_will = value;
                break;
        }
    }
}

VM_FUNC_NEW(SetCreatureSize, 450)
{
    CNWSCreature* creature = GetCreatureById(vm_pop_object());
    int8_t size = vm_pop_int();
    if (creature && size >= 0)
    {
        creature->cre_size = size;
    }
}

VM_FUNC_NEW(GetPCSkillPoints, 451)
{
    CNWSCreature* creature = GetCreatureById(vm_pop_object());
    vm_push_int(creature ? creature->cre_stats->cs_skill_points : 0);
}
VM_FUNC_NEW(SetPCSkillPoints, 452)
{
    CNWSCreature* creature = GetCreatureById(vm_pop_object());
    int8_t skill_points = vm_pop_int();
    if (creature)
    {
        creature->cre_stats->cs_skill_points = skill_points;
    }
}

VM_FUNC_NEW(GetBodyBag, 453)
{
    CNWSCreature* creature = GetCreatureById(vm_pop_object());
    vm_push_int(creature ? creature->cre_bodybag : -1);
}
VM_FUNC_NEW(SetBodyBag, 454)
{
    CNWSCreature* creature = GetCreatureById(vm_pop_object());
    uint8_t body_bag = vm_pop_int();
    if (creature)
    {
        creature->cre_bodybag = body_bag;
    }
}

VM_FUNC_NEW(SetAlignmentGoodEvil, 455)
{
    CNWSCreature* creature = GetCreatureById(vm_pop_object());
    uint32_t value = vm_pop_int();
    if (creature && value <= 100)
    {
        creature->cre_stats->cs_al_goodevil = value;
    }
}
VM_FUNC_NEW(SetAlignmentLawChaos, 456)
{
    CNWSCreature* creature = GetCreatureById(vm_pop_object());
    uint32_t value = vm_pop_int();
    if (creature && value <= 100)
    {
        creature->cre_stats->cs_al_lawchaos = value;
    }
}

CNWSCreatureClass* get_stats_class(CNWSCreatureStats* stats, int class_index)
{
	for (uint32_t i=0; i<stats->cs_classes_len; i++)
	{
		if (stats->cs_classes[i].cl_class == class_index)
		{
			return &stats->cs_classes[i];
		}
	}
	return NULL;
}
CNWSCreatureClass* get_stats_class(CNWSCreature* creature, int class_index)
{
	return get_stats_class(creature->cre_stats, class_index);
}
void vm_func_get_stats_class(std::function<void(CNWSCreatureClass*)> handle_stats)
{
	CNWSCreature* creature = GetCreatureById(vm_pop_object());
    int class_index = vm_pop_int();
	handle_stats(creature ? get_stats_class(creature, class_index) : NULL);
}

VM_FUNC_NEW(GetKnowsSpell, 459)
{
    CNWSCreature* creature = GetCreatureById(vm_pop_object());
    uint32_t spell = vm_pop_int();
    int class_index = vm_pop_int();
	int result = 0;
    if (creature)
    {
        for (uint32_t i=0; i<creature->cre_stats->cs_classes_len; i++)
        {
            if (class_index != -1 && creature->cre_stats->cs_classes[i].cl_class != class_index) continue;

            for (uint32_t sp_level=0; sp_level<=9; sp_level++)
            {
				if (creature->cre_stats->cs_classes[i].cl_spells_known[sp_level].contains(spell))
				{
					result = true;
					break;
				}
            }
        }
    }
    vm_push_int(result);
}
VM_FUNC_NEW(GetKnowSpell, 460)
{
    CNWSCreature* creature = GetCreatureById(vm_pop_object());
    int class_index = vm_pop_int();
    uint32_t sp_level = vm_pop_int();
    uint32_t sp_index = vm_pop_int();
	int result = -1;
    if (creature && sp_level <= 9)
    {
		CNWSCreatureClass* class_stats = get_stats_class(creature, class_index);
		if (class_stats)
		{
			if (sp_index < class_stats->cl_spells_known[sp_level].len)
            {
                result = class_stats->cl_spells_known[sp_level].data[sp_index];
            }
		}
    }
    vm_push_int(result);
}
VM_FUNC_NEW(SetKnownSpell, 461)
{
    CNWSCreature* creature = GetCreatureById(vm_pop_object());
    int class_index = vm_pop_int();
    uint32_t sp_level = vm_pop_int();
    uint32_t sp_index = vm_pop_int();
    uint32_t sp_id = vm_pop_int();
    if (creature && sp_level <= 9)
    {
		CNWSCreatureClass* class_stats = get_stats_class(creature, class_index);
		if (class_stats)
		{
			if (sp_index < class_stats->cl_spells_known[sp_level].len)
            {
                class_stats->cl_spells_known[sp_level].data[sp_index] = sp_id;
            }
		}
    }
}
VM_FUNC_NEW(GetNumKnownSpells, 462)
{
    CNWSCreature* creature = GetCreatureById(vm_pop_object());
    int class_index = vm_pop_int();
    uint32_t sp_level = vm_pop_int();
	int result = 0;
    if (creature && sp_level <= 9)
    {
		CNWSCreatureClass* class_stats = get_stats_class(creature, class_index);
		if (class_stats)
		{
            result = class_stats->cl_spells_known[sp_level].len;
        }
    }
	vm_push_int(result);
}
VM_FUNC_NEW(AddKnownSpell, 463)
{
    CNWSCreature* creature = GetCreatureById(vm_pop_object());
    int class_index = vm_pop_int();
    uint32_t sp_level = vm_pop_int();
    uint32_t sp_id = vm_pop_int();
    if (creature && sp_level <= 9)
    {
		CNWSCreatureClass* class_stats = get_stats_class(creature, class_index);
		if (class_stats)
		{
            class_stats->cl_spells_known[sp_level].add(sp_id);
        }
    }
}
VM_FUNC_NEW(RemoveKnownSpell, 464)
{
    CNWSCreature* creature = GetCreatureById(vm_pop_object());
    int class_index = vm_pop_int();
    uint32_t sp_level = vm_pop_int();
    uint32_t sp_id = vm_pop_int();
    if (creature && sp_level <= 9)
    {
		CNWSCreatureClass* class_stats = get_stats_class(creature, class_index);
		if (class_stats)
		{
	        class_stats->cl_spells_known[sp_level].delvalue(sp_id);
        }
    }
}
VM_FUNC_NEW(GetMemorizedSpellCount, 465)
{
	CNWSCreature* creature = GetCreatureById(vm_pop_object());
    int class_index = vm_pop_int();
    uint32_t sp_level = vm_pop_int();
	int result = 0;
    if (creature && sp_level <= 9)
    {
		CNWSCreatureClass* class_stats = get_stats_class(creature, class_index);
		if (class_stats)
		{
	        result = class_stats->cl_spells_mem[sp_level].len;
        }
    }
	vm_push_int(result);
}
void vm_func_get_memorised_spell_info(std::function<void(CNWSStats_Spell*)> push_result)
{
	vm_func_get_stats_class([&](CNWSCreatureClass* cre_class){
		uint32_t sp_level = vm_pop_int();
		uint32_t sp_index = vm_pop_int();
		if (cre_class && sp_level <= 9 && sp_index < cre_class->cl_spells_mem[sp_level].len)
		{
			push_result((CNWSStats_Spell*)cre_class->cl_spells_mem[sp_level].data[sp_index]);
		}
		else
		{
			push_result(NULL);
		}
	});
}
VM_FUNC_NEW(GetMemorizedSpellId, 466)
{
	vm_func_get_memorised_spell_info([](CNWSStats_Spell* memorised_spell){
		vm_push_int(memorised_spell ? memorised_spell->sp_id : -1);
	});
}
VM_FUNC_NEW(GetMemorizedSpellReady, 467)
{
	vm_func_get_memorised_spell_info([](CNWSStats_Spell* memorised_spell){
		vm_push_int(memorised_spell ? memorised_spell->sp_ready : 0);
	});
}
VM_FUNC_NEW(GetMemorizedSpellMetaMagic, 468)
{
	vm_func_get_memorised_spell_info([](CNWSStats_Spell* memorised_spell){
		vm_push_int(memorised_spell ? memorised_spell->sp_meta : 0);
	});
}
VM_FUNC_NEW(GetMemorizedSpellDomain, 469)
{
	vm_func_get_memorised_spell_info([](CNWSStats_Spell* memorised_spell){
		vm_push_int(memorised_spell ? memorised_spell->sp_domain : 0);
	});
}
VM_FUNC_NEW(SetMemorizedSpellId, 470)
{
	vm_func_get_memorised_spell_info([](CNWSStats_Spell* memorised_spell){
		uint32_t sp_id = vm_pop_int();
		if (memorised_spell)
		{
			memorised_spell->sp_id = sp_id;
		}
	});
}
VM_FUNC_NEW(SetMemorizedSpellReady, 471)
{
	vm_func_get_memorised_spell_info([](CNWSStats_Spell* memorised_spell){
		int sp_ready = vm_pop_int();
		if (memorised_spell)
		{
			memorised_spell->sp_ready = sp_ready;
		}
	});
}
VM_FUNC_NEW(SetMemorizedSpellMetaMagic, 472)
{
	vm_func_get_memorised_spell_info([](CNWSStats_Spell* memorised_spell){
		uint32_t sp_meta = vm_pop_int();
		if (memorised_spell)
		{
			memorised_spell->sp_meta = sp_meta;
		}
	});
}
VM_FUNC_NEW(SetMemorizedSpellDomain, 473)
{
	vm_func_get_memorised_spell_info([](CNWSStats_Spell* memorised_spell){
		uint32_t sp_domain = vm_pop_int();
		if (memorised_spell)
		{
			memorised_spell->sp_domain = sp_domain;
		}
	});
}
VM_FUNC_NEW(SetMemorizedSpell, 474)
{
	vm_func_get_memorised_spell_info([](CNWSStats_Spell* memorised_spell){
		uint32_t sp_id = vm_pop_int();
		int sp_ready = vm_pop_int();
		uint32_t sp_meta = vm_pop_int();
		uint32_t sp_domain = vm_pop_int();
		if (memorised_spell)
		{
			memorised_spell->sp_id = sp_id;
			memorised_spell->sp_ready = sp_ready;
			memorised_spell->sp_meta = sp_meta;
			memorised_spell->sp_domain = sp_domain;
		}
	});
}

VM_FUNC_NEW(GetClericDomain, 475)
{
	CNWSCreature* creature = GetCreatureById(vm_pop_object());
	uint32_t domain_pos = vm_pop_int();
	int result = -1;
	if (creature)
	{
		CNWSCreatureClass* class_stats = get_stats_class(creature, CLASS_TYPE_CLERIC);
		if (class_stats)
		{
			result = (domain_pos == 1 ? class_stats->cl_domain_1 : class_stats->cl_domain_2);
		}
	}
	vm_push_int(result);
}
VM_FUNC_NEW(SetClericDomain, 476)
{
	CNWSCreature* creature = GetCreatureById(vm_pop_object());
	uint32_t domain_pos = vm_pop_int();
	int value = vm_pop_int();
	if (creature)
	{
		CNWSCreatureClass* class_stats = get_stats_class(creature, CLASS_TYPE_CLERIC);
		if (class_stats)
		{
			if (domain_pos == 1)
			{
				class_stats->cl_domain_1 = value;
			}
			else
			{
				class_stats->cl_domain_2 = value;
			}
		}
	}
}
VM_FUNC_NEW(GetWizardSpecialization, 477)
{
	CNWSCreature* creature = GetCreatureById(vm_pop_object());
	int result = -1;
	if (creature)
	{
		CNWSCreatureClass* class_stats = get_stats_class(creature, CLASS_TYPE_CLERIC);
		if (class_stats)
		{
			result = class_stats->cl_specialist;
		}
	}
	vm_push_int(result);
}
VM_FUNC_NEW(SetWizardSpecialization, 478)
{
	CNWSCreature* creature = GetCreatureById(vm_pop_object());
	uint32_t value = vm_pop_int();
	if (creature && value <= 8)
	{
		CNWSCreatureClass* class_stats = get_stats_class(creature, CLASS_TYPE_CLERIC);
		if (class_stats)
		{
			class_stats->cl_specialist = value;
		}
	}
}
VM_FUNC_NEW(GetSpecialAbilityCount, 479)
{
	CNWSCreature* creature = GetCreatureById(vm_pop_object());
	vm_push_int(creature && creature->cre_stats->cs_specabil ? creature->cre_stats->cs_specabil->len : 0);
}
void vm_func_get_sa_info(std::function<void(CNWSStats_SpecAbil*)> push_result)
{
	CNWSCreature* creature = GetCreatureById(vm_pop_object());
	uint32_t index = vm_pop_int();
	if (creature && creature->cre_stats->cs_specabil && index < creature->cre_stats->cs_specabil->len)
	{
		push_result(&creature->cre_stats->cs_specabil->data[index]);
	}
	else
	{
		push_result(NULL);
	}
}
VM_FUNC_NEW(GetSpecialAbilitySpellId, 480)
{
	vm_func_get_sa_info([](CNWSStats_SpecAbil* sa){
		vm_push_int(sa ? sa->sa_id : -1);
	});
}
VM_FUNC_NEW(GetSpecialAbilityFlags, 481)
{
	vm_func_get_sa_info([](CNWSStats_SpecAbil* sa){
		vm_push_int(sa ? sa->sa_flags : 0);
	});
}
VM_FUNC_NEW(GetSpecialAbilityLevel, 482)
{
	vm_func_get_sa_info([](CNWSStats_SpecAbil* sa){
		vm_push_int(sa ? sa->sa_level : 0);
	});
}
VM_FUNC_NEW(SetSpecialAbilitySpellId, 483)
{
	vm_func_get_sa_info([](CNWSStats_SpecAbil* sa){
		uint32_t value = vm_pop_int();
		if (sa) sa->sa_id = value;
	});
}
VM_FUNC_NEW(SetSpecialAbilityFlags, 484)
{
	vm_func_get_sa_info([](CNWSStats_SpecAbil* sa){
		uint32_t value = vm_pop_int();
		if (sa) sa->sa_flags = value;
	});
}
VM_FUNC_NEW(SetSpecialAbilityLevel, 485)
{
	vm_func_get_sa_info([](CNWSStats_SpecAbil* sa){
		uint32_t value = vm_pop_int();
		if (sa) sa->sa_level = value;
	});
}
VM_FUNC_NEW(RemoveSpecialAbility, 486)
{
	CNWSCreature* creature = GetCreatureById(vm_pop_object());
	uint32_t index = vm_pop_int();
	if (creature && creature->cre_stats->cs_specabil && index < creature->cre_stats->cs_specabil->len)
	{
		creature->cre_stats->cs_specabil->delindex(index);
	}
}
VM_FUNC_NEW(AddSpecialAbility, 487)
{
	CNWSCreature* creature = GetCreatureById(vm_pop_object());
	uint32_t sa_id = vm_pop_int();
	uint32_t sa_flags = vm_pop_int();
	uint32_t sa_level = vm_pop_int();
	if (creature && creature->cre_stats->cs_specabil)
	{
		CNWSStats_SpecAbil sa;
		sa.sa_id = sa_id;
		sa.sa_flags = sa_flags;
		sa.sa_level = sa_level;
		creature->cre_stats->cs_specabil->add(sa);
	}
}

VM_FUNC_NEW(GetPortrait, 488)
{
	CNWSCreature* creature = GetCreatureById(vm_pop_object());
	vm_push_string(creature ? creature->cre_stats->cs_portrait.to_str() : "");
}
VM_FUNC_NEW(SetPortrait, 489)
{
	CNWSCreature* creature = GetCreatureById(vm_pop_object());
	std::string portrait = vm_pop_string();
	if (creature)
	{
		strncpy(creature->cre_stats->cs_portrait.value, portrait.c_str(), 16);
	}
}

VM_FUNC_NEW(GetSoundset, 490)
{
	CNWSCreature* creature = GetCreatureById(vm_pop_object());
	vm_push_int(creature ? creature->cre_soundset : -1);
}
VM_FUNC_NEW(SetSoundset, 491)
{
	CNWSCreature* creature = GetCreatureById(vm_pop_object());
	uint32_t soundset = vm_pop_int();
	if (creature)
	{
		creature->cre_soundset = soundset;
	}
}

VM_FUNC_NEW(GetConversation, 493)
{
	CNWSCreature* creature = GetCreatureById(vm_pop_object());
	vm_push_string(creature ? creature->cre_stats->cs_conv.to_str() : "");
}
VM_FUNC_NEW(SetConversation, 494)
{
	CNWSCreature* creature = GetCreatureById(vm_pop_object());
	std::string conversation = vm_pop_string();
	if (creature)
	{
		strncpy(creature->cre_stats->cs_conv.value, conversation.c_str(), 16);
	}
}

void (*SetMovementRate)(CNWSCreatureStats*, int) = (void (*)(CNWSCreatureStats*, int))0x0815d81c;
VM_FUNC_NEW(SetMovementRate, 513)
{
	CNWSCreature* creature = GetCreatureById(vm_pop_object());
	uint32_t rate = vm_pop_int();
	if (creature)
	{
		SetMovementRate(creature->cre_stats, rate);
	}
}

VM_FUNC_NEW(SetRacialType, 514)
{
	CNWSCreature* creature = GetCreatureById(vm_pop_object());
	uint32_t race = vm_pop_int();
	if (creature && race < 256)
	{
		creature->cre_stats->cs_race = race;
	}
}

int (*GetCriticalHitMultiplier)(CNWSCreatureStats*, int) = (int (*)(CNWSCreatureStats*, int))0x0814c4a0;
VM_FUNC_NEW(GetCriticalHitMultiplier, 517)
{
	CNWSCreature* creature = GetCreatureById(vm_pop_object());
	int off_hand  = vm_pop_int();
	vm_push_int(creature ? GetCriticalHitMultiplier(creature->cre_stats, off_hand) : 0);
}
int (*GetCriticalHitRoll)(CNWSCreatureStats*, int) = (int (*)(CNWSCreatureStats*, int))0x0814c31c;
VM_FUNC_NEW(GetCriticalHitRange, 518)
{
	CNWSCreature* creature = GetCreatureById(vm_pop_object());
	int off_hand  = vm_pop_int();
	vm_push_int(creature ? GetCriticalHitRoll(creature->cre_stats, off_hand) : 0);
}

VM_FUNC_NEW(GetForcedWalk, 521)
{
	CNWSCreature* creature = GetCreatureById(vm_pop_object());
	vm_push_int(creature ? creature->cre_forced_walk : false);
}
VM_FUNC_NEW(SetForcedWalk, 522)
{
	CNWSCreature* creature = GetCreatureById(vm_pop_object());
	int forced_walk = vm_pop_int();
	if (creature)
	{
		creature->cre_forced_walk = forced_walk;
	}
}

VM_FUNC_NEW(GetAnimation, 376)
{
	CNWSObject* object = GetObjectById(vm_pop_object());
	vm_push_int(object?object->obj_anim:-1);
}
VM_FUNC_NEW(GetAnimationSpeed, 377)
{
	CNWSObject* object = GetObjectById(vm_pop_object());
	vm_push_float(object?object->obj_anim_speed:0);
}

int (*GetFlatFlooted)(CNWSCreature*) = (int (*)(CNWSCreature*))0x080f1830;
VM_FUNC_NEW(GetFlatFlooted, 378)
{
	CNWSCreature* creature = GetCreatureById(vm_pop_object());
	vm_push_int(creature?GetFlatFlooted(creature):0);
}

VM_FUNC_NEW(GetIsPolymorphed, 379)
{
	CNWSCreature* creature = GetCreatureById(vm_pop_object());
	vm_push_int(creature ? creature->cre_is_poly : false);
}

int (*PossessFamiliar)(CNWSCreature*) = (int (*)(CNWSCreature*))0x0810de48;
VM_FUNC_NEW(PossessFamiliar, 233)
{
	CNWSCreature* creature = GetCreatureById(vm_pop_object());
	if (creature)
	{
		PossessFamiliar(creature);
	}
}

VM_FUNC_NEW(SetTauntAnimationPlayed, 238)
{
	CNWSCreature* creature = GetCreatureById(vm_pop_object());
	int value = vm_pop_int();
	if (creature)
	{
		creature->cre_taunt_anim_played = value;
	}
}

uint8_t (*GetUseMonkAbilities)(CNWSCreature*) = (uint8_t (*)(CNWSCreature*))0x081241f8;
VM_FUNC_NEW(GetUseMonkAbilities, 226)
{
	CNWSCreature* creature = GetCreatureById(vm_pop_object());
	vm_push_int(creature ? GetUseMonkAbilities(creature) : 0);
}

void* (*GetVisibleListElement)(CNWSCreature*, uint32_t) = (void* (*)(CNWSCreature*, uint32_t))0x081131fc;
VM_FUNC_NEW(GetIsInVisibleList, 182)
{
	CNWSCreature* creature = GetCreatureById(vm_pop_object());
	uint32_t target_id = vm_pop_object();
	int result = false;
	if (creature)
	{
		result = (GetVisibleListElement(creature, target_id)!=NULL);
	}
	vm_push_int(result);
}

VM_FUNC_NEW(SetGender, 177)
{
	uint32_t creature_id = vm_pop_object();
	int gender = vm_pop_int();
	CNWSCreature* creature = GetCreatureById(creature_id);
	if (creature)
	{
		creature->cre_stats->cs_gender = gender;
		creature->cre_last_gender = gender;
	}
}
VM_FUNC_NEW(SetBABOverride, 178)
{
	uint32_t creature_id = vm_pop_object();
	int ab_override = vm_pop_int();
	CNWSCreature* creature = GetCreatureById(creature_id);
	if (creature && creature->obj.obj_id < OBJECT_INVALID)
	{
		creature->cre_stats->cs_override_bab = ab_override;
	}
}

VM_FUNC_NEW(SetDesiredLocation, 173)
{
	uint32_t creature_id = vm_pop_object();
	CScriptLocation location = vm_pop_location();
	CNWSCreature* creature = GetCreatureById(creature_id);
	if (creature)
	{
		creature->cre_desired_area = location.loc_area;
		creature->cre_desired_pos = location.loc_position;
		creature->obj.obj_orientation = location.loc_orientation;
		creature->cre_desired_complete = 0;
	}
}

VM_FUNC_NEW(GetCreatureEncounter, 283)
{
	CNWSCreature* creature = GetCreatureById(vm_pop_object());
	vm_push_object(creature ? creature->cre_encounter_obj : OBJECT_INVALID);
}

VM_FUNC_NEW(GetBodyBagObject, 284)
{
	CNWSCreature* creature = GetCreatureById(vm_pop_object());
	vm_push_object(creature ? creature->cre_bodybag_id : OBJECT_INVALID);
}

#define FEAT_EPIC_ENERGY_RESISTANCE_ACID_10 552
#define FEAT_EPIC_ENERGY_RESISTANCE_ACID_1 543
#define FEAT_EPIC_ENERGY_RESISTANCE_COLD_10 542
#define FEAT_EPIC_ENERGY_RESISTANCE_COLD_1 533
#define FEAT_EPIC_ENERGY_RESISTANCE_ELECTRICAL_10 572
#define FEAT_EPIC_ENERGY_RESISTANCE_ELECTRICAL_1 563
#define FEAT_EPIC_ENERGY_RESISTANCE_FIRE_10 562
#define FEAT_EPIC_ENERGY_RESISTANCE_FIRE_1 553
#define FEAT_EPIC_ENERGY_RESISTANCE_SONIC_10 582
#define FEAT_EPIC_ENERGY_RESISTANCE_SONIC_1 573
VM_FUNC_NEW(GetDamageResistance, 263)
{
	CNWSCreature* creature = GetCreatureById(vm_pop_object());
	int nDamageFlag = vm_pop_int();
	int bNoLimit = vm_pop_int();
	int nMaxDamageResist = 0;
	if (creature)
	{
		CNWSObject* object = (CNWSObject*)creature;
		for (uint32_t nEffect=0; nEffect<object->obj_effects_len; nEffect++)
		{
			CGameEffect* effect = *(object->obj_effects+nEffect);
			if (effect->eff_type == 0x2 &&
				*(effect->eff_integers)&nDamageFlag &&
				*(effect->eff_integers+1)>nMaxDamageResist &&
				(bNoLimit || *(effect->eff_integers+2)==0))
			{
				nMaxDamageResist = *(effect->eff_integers+1);
			}
		}
		if (nDamageFlag & DAMAGE_TYPE_ACID)
		{
			int nEpicResistFeat = FEAT_EPIC_ENERGY_RESISTANCE_ACID_10;
			while (nEpicResistFeat >= FEAT_EPIC_ENERGY_RESISTANCE_ACID_1)
			{
				if (has_feat(creature->cre_stats, nEpicResistFeat))
				{
					nMaxDamageResist += 10*(nEpicResistFeat-FEAT_EPIC_ENERGY_RESISTANCE_ACID_1+1);
					break;
				}
				nEpicResistFeat--;
			}
			if (has_feat(creature->cre_stats, 428)) nMaxDamageResist+=5;
		}
		if (nDamageFlag & DAMAGE_TYPE_COLD)
		{
			int nEpicResistFeat = FEAT_EPIC_ENERGY_RESISTANCE_COLD_10;
			while (nEpicResistFeat >= FEAT_EPIC_ENERGY_RESISTANCE_COLD_1)
			{
				if (has_feat(creature->cre_stats, nEpicResistFeat))
				{
					nMaxDamageResist += 10*(nEpicResistFeat-FEAT_EPIC_ENERGY_RESISTANCE_COLD_1+1);
					break;
				}
				nEpicResistFeat--;
			}
			if (has_feat(creature->cre_stats, 427)) nMaxDamageResist+=5;
		}
		if (nDamageFlag & DAMAGE_TYPE_ELECTRICAL)
		{
			int nEpicResistFeat = FEAT_EPIC_ENERGY_RESISTANCE_ELECTRICAL_10;
			while (nEpicResistFeat >= FEAT_EPIC_ENERGY_RESISTANCE_ELECTRICAL_1)
			{
				if (has_feat(creature->cre_stats, nEpicResistFeat))
				{
					nMaxDamageResist += 10*(nEpicResistFeat-FEAT_EPIC_ENERGY_RESISTANCE_ELECTRICAL_1+1);
					break;
				}
				nEpicResistFeat--;
			}
			if (has_feat(creature->cre_stats, 430)) nMaxDamageResist+=5;
		}
		if (nDamageFlag & DAMAGE_TYPE_FIRE)
		{
			int nEpicResistFeat = FEAT_EPIC_ENERGY_RESISTANCE_COLD_10;
			while (nEpicResistFeat >= FEAT_EPIC_ENERGY_RESISTANCE_COLD_1)
			{
				if (has_feat(creature->cre_stats, nEpicResistFeat))
				{
					nMaxDamageResist += 10*(nEpicResistFeat-FEAT_EPIC_ENERGY_RESISTANCE_COLD_1+1);
					break;
				}
				nEpicResistFeat--;
			}
			if (has_feat(creature->cre_stats, 429)) nMaxDamageResist+=5;
		}
		if (nDamageFlag & DAMAGE_TYPE_SONIC)
		{
			int nEpicResistFeat = FEAT_EPIC_ENERGY_RESISTANCE_SONIC_10;
			while (nEpicResistFeat >= FEAT_EPIC_ENERGY_RESISTANCE_SONIC_1)
			{
				if (has_feat(creature->cre_stats, nEpicResistFeat))
				{
					nMaxDamageResist += 10*(nEpicResistFeat-FEAT_EPIC_ENERGY_RESISTANCE_SONIC_1+1);
					break;
				}
				nEpicResistFeat--;
			}
			if (has_feat(creature->cre_stats, 431)) nMaxDamageResist+=5;
		}
	}
	vm_push_int(nMaxDamageResist);
}
int (*DoDamageReduction)(CNWSObject*, CNWSCreature*, int, uint8_t, int, int) = (int (*)(CNWSObject*, CNWSCreature*, int, uint8_t, int, int))0x081cbd74;
VM_FUNC_NEW(GetDamageReductionEffectForPower, 264)
{
	int damage_reduction = 0;
	CNWSCreature* creature = GetCreatureById(vm_pop_object());
	int dr_power = vm_pop_int();
	if (creature)
	{
		CNWSObject* object = (CNWSObject*)creature;
		damage_reduction = (10000 - DoDamageReduction(object, creature, 10000, dr_power, 1, 1));
	}
	vm_push_int(damage_reduction);
}
#define EFFECT_TRUETYPE_DAMAGE_IMMUNITY_INCREASE          16
#define EFFECT_TRUETYPE_DAMAGE_IMMUNITY_DECREASE          17
inline int immunity_id_to_percent(int immunity_id)
{
    if (immunity_id == 1) return 5;
    if (immunity_id == 2) return 10;
    if (immunity_id == 3) return 26;
    if (immunity_id == 4) return 50;
    if (immunity_id == 5) return 75;
    if (immunity_id == 6) return 90;
    if (immunity_id == 7) return 100;
	return 0;
}
VM_FUNC_NEW(GetTotalDamageImmunity, 265)
{
	int total_immunity = 0;
	CNWSCreature* creature = GetCreatureById(vm_pop_object());
	int damage_type = vm_pop_int();
	if (creature)
	{
		if (damage_type == 256/*fire*/ && has_feat(creature->cre_stats, 964/*red dragon fire immunity*/))
		{
			total_immunity = 100;
		}
		else
		{
			CNWSObject* object = (CNWSObject*)creature;
			for (uint32_t nEffect=0; nEffect<object->obj_effects_len; nEffect++)
			{
				CGameEffect* effect = *(object->obj_effects+nEffect);
				if (effect->eff_type == EFFECT_TRUETYPE_DAMAGE_IMMUNITY_INCREASE &&
					effect->eff_integers[0]==damage_type)
				{
					if (effect->eff_dursubtype == 3)
					{
						total_immunity += effect->eff_integers[1];
					}
					else if (effect->eff_dursubtype == 10)
					{
						total_immunity += immunity_id_to_percent(effect->eff_integers[1]);
					}
					else
					{
						//fprintf(stderr, "unknown effect->eff_dursubtype:%d\n", effect->eff_dursubtype);
					}
				}
				else if (effect->eff_type == EFFECT_TRUETYPE_DAMAGE_IMMUNITY_DECREASE &&
					effect->eff_integers[0]==damage_type)
				{
					if (effect->eff_dursubtype == 3)
					{
						total_immunity -= effect->eff_integers[1];
					}
					else if (effect->eff_dursubtype == 10)
					{
						total_immunity -= immunity_id_to_percent(effect->eff_integers[1]);
					}
					else
					{
						fprintf(stderr, "unknown effect->eff_dursubtype:%d\n", effect->eff_dursubtype);
					}
				}
			}
		}
	}
	if (total_immunity > 100) total_immunity = 100;
	else if (total_immunity < -100) total_immunity = -100;
	vm_push_int(total_immunity);
}
VM_FUNC_NEW(GetConcealment, 266)
{
	int best_conceal = 0;
	CNWSCreature* creature = GetCreatureById(vm_pop_object());
	int versus_id = vm_pop_int();
	if (creature)
	{
		if (has_feat(creature->cre_stats, 752)) best_conceal = 50;
		else if (has_feat(creature->cre_stats, 751)) best_conceal = 40;
		else if (has_feat(creature->cre_stats, 750)) best_conceal = 30;
		else if (has_feat(creature->cre_stats, 749)) best_conceal = 20;
		else if (has_feat(creature->cre_stats, 748)) best_conceal = 10;

		CNWSObject* object = (CNWSObject*)creature;
		for (uint32_t nEffect=0; nEffect<object->obj_effects_len; nEffect++)
		{
			CGameEffect* effect = *(object->obj_effects+nEffect);
			if (effect->eff_type == 76 && (effect->eff_integers[4]==0 || effect->eff_integers[4]==versus_id) && effect->eff_integers[0] > best_conceal)
			{
				best_conceal = effect->eff_integers[0];
			}
		}
	}
	vm_push_int(best_conceal);
}

VM_FUNC_NEW(GetLastDialogReplyTime, 45)
{
	uint32_t creature_id = vm_pop_object();
	int result = 0;
	CNWSCreature* creature = GetCreatureById(creature_id);
	if (creature)
	{
		result = GetCreatureExtra(creature)->last_dialog_reply_time;
	}
	vm_push_int(result);
}

uint32_t (*GetSlotFromItem)(CNWSInventory*, CNWSItem*) = (uint32_t (*)(CNWSInventory*, CNWSItem*))0x0819ec6c;
VM_FUNC_NEW(GetSlotFromItem, 130)
{
	CNWSCreature* creature = GetCreatureById(vm_pop_object());
	CNWSItem* item = GetItemById(vm_pop_object());
	int result = -1;
	if (creature && item)
	{
		int slot_flag = GetSlotFromItem(creature->cre_equipment, item);
		if (slot_flag)
		{
			result = __builtin_ctz(slot_flag);
		}
	}
	vm_push_int(result);
}
void (*RunEquipItem)(CNWSCreature*, uint32_t, uint32_t, uint32_t) = (void (*)(CNWSCreature*, uint32_t, uint32_t, uint32_t))0x08116f20;
VM_FUNC_NEW(ForceEquipItem, 131)
{
	CNWSCreature* creature = GetCreatureById(vm_pop_object());
	uint32_t item_id = vm_pop_object();
	int slot = vm_pop_int();
	if (creature)
	{
		slot = pow(2, slot);
		RunEquipItem(creature, item_id, slot, 0);
	}
}
void (*UnequipItem)(CNWSCreature*, uint32_t, uint32_t, uint8_t, uint8_t, int, uint32_t) = (void (*)(CNWSCreature*, uint32_t, uint32_t, uint8_t, uint8_t, int, uint32_t))0x08117b54;
VM_FUNC_NEW(ForceUnequipItem, 132)
{
	CNWSCreature* creature = GetCreatureById(vm_pop_object());
	uint32_t item_id = vm_pop_object();
	if (creature)
	{
		UnequipItem(creature, item_id, OBJECT_INVALID, 255, 255, 0, OBJECT_INVALID);
	}
}

VM_FUNC_NEW(GetDesiredLocation, 135)
{
	CNWSCreature* creature = GetCreatureById(vm_pop_object());
	CScriptLocation desired_location = empty_location;
	if (creature)
	{
		desired_location.loc_position = creature->cre_desired_pos;
		desired_location.loc_area = creature->cre_desired_area;
	}
	vm_push_location(&desired_location);
}

void (*EndCombatRound)(CNWSCombatRound*) = (void (*)(CNWSCombatRound*))0x080e1908;
VM_FUNC_NEW(EndCombatRound, 153)
{
	CNWSCreature* creature = GetCreatureById(vm_pop_object());
	if (creature)
	{
		EndCombatRound(creature->cre_combat_round);
	}
}

VM_FUNC_NEW(GetSpellCastFeat, 185)
{
	CNWSObject* object = GetObjectById(vm_pop_object());
	vm_push_int(object ? object->obj_last_spell_feat : -1);
}
VM_FUNC_NEW(SetSpellCastFeat, 186)
{
	CNWSObject* object = GetObjectById(vm_pop_object());
	int cast_feat = vm_pop_int();
	if (object)
	{
		object->obj_last_spell_feat = cast_feat;
	}
}
VM_FUNC_NEW(SetLastSpellCastClass, 187)
{
	CNWSCreature* creature = GetCreatureById(vm_pop_object());
	int cast_class = vm_pop_int();
	if (creature)
	{
		if (cast_class == 255)
		{
			creature->obj.obj_last_spell_multiclass = 255;
		}
		else
		{
			for (uint8_t nClassIndex=0; nClassIndex<creature->cre_stats->cs_classes_len; nClassIndex++)
			{
				if (creature->cre_stats->cs_classes[nClassIndex].cl_class == cast_class)
				{
					creature->obj.obj_last_spell_multiclass = nClassIndex;
					break;
				}
			}
		}
	}
}
VM_FUNC_NEW(SetLastSpellCastId, 188)
{
	CNWSCreature* creature = GetCreatureById(vm_pop_object());
	int spell_id = vm_pop_int();
	if (creature)
	{
		creature->obj.obj_last_spell_id = spell_id;
	}
}
VM_FUNC_NEW(SetIsSpellCastFromItem, 189)
{
	CNWSCreature* creature = GetCreatureById(vm_pop_object());
	int is_from_item = vm_pop_int();
	if (creature)
	{
		creature->cre_item_spell = is_from_item;
	}
}
VM_FUNC_NEW(SetMetaMagicFeat, 190)
{
	CNWSObject* object = GetObjectById(vm_pop_object());
	int meta_magic = vm_pop_int();
	if (object)
	{
		object->obj_last_spell_metamagic = meta_magic;
	}
}
uint8_t (*GetFeatSourceClass)(CNWSCreatureStats*, uint16_t) = (uint8_t (*)(CNWSCreatureStats*, uint16_t))0x081623f4;
VM_FUNC_NEW(GetFeatSourceClass, 191)
{
	CNWSCreature* creature = GetCreatureById(vm_pop_object());
	uint32_t feat = vm_pop_int();
	vm_push_int(creature ? GetFeatSourceClass(creature->cre_stats, feat) : -1);
}
CNWSCombatAttackData* (*GetAttack)(CNWSCombatRound*, int) = (CNWSCombatAttackData* (*)(CNWSCombatRound*, int))0x080e522c;
VM_FUNC_NEW(GetLastAttackRollResult, 192)
{
	CNWSCreature* creature = GetCreatureById(vm_pop_object());
	int result = -1;
	if (creature)
	{
		CNWSCombatAttackData* attack_data = GetAttack(creature->cre_combat_round, creature->cre_combat_round->combat_current_attack);
		if (attack_data)
		{
			result = attack_data->attack_roll_result;
		}
	}
	vm_push_int(result);
}
VM_FUNC_NEW(GetLastAttackFeat, 193)
{
	CNWSCreature* creature = GetCreatureById(vm_pop_object());
	int result = -1;
	if (creature)
	{
		CNWSCombatAttackData* attack_data = GetAttack(creature->cre_combat_round, creature->cre_combat_round->combat_current_attack);
		if (attack_data)
		{
			result = attack_data->attack_feat;
		}
	}
	vm_push_int(result);
}
VM_FUNC_NEW(GetHasSpellMemorized, 209)
{
	CNWSCreature* creature = GetCreatureById(vm_pop_object());
	int spell_id = vm_pop_int();
	uint32_t class_id = vm_pop_int();
	int result = false;
	if (creature != NULL)
	{
		for (uint32_t i=0; i<creature->cre_stats->cs_classes_len; i++)
		{
			CNWSCreatureClass* creature_class = &(creature->cre_stats->cs_classes[i]);
			if (creature_class->cl_class != class_id) continue;
			for (uint32_t nLevel=0; nLevel<10; nLevel++)
			{
				CExoArrayList<void*>* level_spells = &(creature_class->cl_spells_mem[nLevel]);
				for (uint32_t nSpell=0; nSpell<level_spells->len; nSpell++)
				{
					if (((CNWSStats_Spell*)level_spells->data)[nSpell].sp_id == spell_id)
					{
						result = true;
						break;
					}
				}
			}
			break;
		}
	}
	vm_push_int(result);
}

}
