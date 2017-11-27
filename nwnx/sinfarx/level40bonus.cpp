#include "core.h"
#include "setting.h"

using namespace nwnx;
using namespace nwnx::core;

namespace
{
	
int (*GetWeaponSpecialization)(CNWSCreatureStats*, CNWSItem*) = (int (*)(CNWSCreatureStats*, CNWSItem*))0x081562a4;
inline int OnGetDamage_GetWeaponSpec_WithSingleDualBonus(CNWSCreatureStats* stats, CNWSItem* weapon, uint8_t* p_damage_bonus)
{
	int result = GetWeaponSpecialization(stats, weapon);
	if (result)
	{
		if (stats->cs_is_pc && GetLevel(stats, 0) == 40 && stats->cs_classes_len < 3)
		{
			if (stats->cs_classes_len == 1)
			{
				*p_damage_bonus = 8;
				return result;
			}
			else if (stats->cs_classes_len == 2)
			{
				*p_damage_bonus = 4;
				return result;
			}
		}
	}
	*p_damage_bonus = 2;
	return result;
}
int OnGetDamageBonus_GetWeaponSpec(CNWSCreatureStats* stats, CNWSItem* weapon)
{
	return OnGetDamage_GetWeaponSpec_WithSingleDualBonus(stats, weapon, (uint8_t*)0x08148769);
}
int OnGetMeleeDamageBonus_GetWeaponSpec(CNWSCreatureStats* stats, CNWSItem* weapon)
{
	return OnGetDamage_GetWeaponSpec_WithSingleDualBonus(stats, weapon, (uint8_t*)0x0814371e);
}
	
int (*GetIsWeaponOfChoice)(CNWSCreatureStats*, uint32_t) = (int (*)(CNWSCreatureStats*, uint32_t))0x8160f50;
inline int on_get_weapon_of_choice_add_dual_wm_ab(CNWSCreatureStats* stats, uint32_t weapon, uint32_t write_addr, uint32_t add_ab_val, uint32_t default_val)
{
	int result = GetIsWeaponOfChoice(stats, weapon);
	if (result)
	{
		if (stats->cs_is_pc && GetLevel(stats, 0) == 40 && stats->cs_classes_len == 2)
		{
			*(uint32_t*)write_addr = add_ab_val;
			return true;
		}
	}
	*(uint32_t*)write_addr = default_val;
	return result;
}
int OnGetAB_HasSuperiorWeapon(CNWSCreatureStats* stats, uint32_t weapon)
{
	return on_get_weapon_of_choice_add_dual_wm_ab(stats, weapon,
		0x08143324,
		0x90909047 /*inc $edi, nop, nop, nop*/,
		0x0174c085 /*default (test eax, je "no bonus")*/);
}
int OnGetABVersus_HasSuperiorWeapon(CNWSCreatureStats* stats, uint32_t weapon)
{
	return on_get_weapon_of_choice_add_dual_wm_ab(stats, weapon,
		0x08145118,
		2,
		1);
}
uint16_t sneak_attack_extra_roll = 0;
int GetHasSneakAttackFeat(CNWSCreatureStats* stats, uint16_t feat)
{
	int result = has_feat(stats, feat);
	uint8_t class_index;
	for (class_index=0; class_index<stats->cs_classes_len; class_index++)
	{
		if (stats->cs_classes[class_index].cl_class == 8) break; //rogue
	}
	if (class_index<stats->cs_classes_len)
	{
		if (stats->cs_is_pc && GetLevel(stats, 0) == 40)
		{
			if (stats->cs_classes_len == 1)
			{
				sneak_attack_extra_roll = 3;
				return result;
			}
			else if (stats->cs_classes_len == 2)
			{
				sneak_attack_extra_roll = 1;
				return result;
			}
		}
	}
	sneak_attack_extra_roll = 0;
	return result;
}

uint16_t (*RollDice)(CNWRules*, uint8_t, uint8_t) = (uint16_t (*)(CNWRules*, uint8_t, uint8_t))0x80cb6b0;
int RollSneakAttackDamage(CNWRules* rules, uint8_t num, uint8_t dice)
{
	num += sneak_attack_extra_roll;
	return RollDice(rules, num, dice);
}

int GetEpicDamageReductionFeatsBonus(CNWSCreatureStats* stats, short feat)
{
	uint8_t bonus = 0;

	//barbarian
	uint8_t class_index;
	for (class_index=0; class_index<stats->cs_classes_len; class_index++)
	{
		if (stats->cs_classes[class_index].cl_class == 0) break;
	}
	if (class_index<stats->cs_classes_len)
	{
		if (stats->cs_is_pc && GetLevel(stats, 0) == 40)
		{
			if (stats->cs_classes_len <= 2)
			{
				bonus += 1;
			}
		}
	}

	//Dwarven Defender
	for (class_index=0; class_index<stats->cs_classes_len; class_index++)
	{
		if (stats->cs_classes[class_index].cl_class == 36) break;
	}
	if (class_index<stats->cs_classes_len)
	{
		if (stats->cs_is_pc && GetLevel(stats, 0) == 40)
		{
			if (stats->cs_classes_len <= 2)
			{
				bonus += 2;
			}
		}
	}

	if (has_feat(stats, 494))
	{
		bonus += 9;
	}
	else if (has_feat(stats, 493))
	{
		bonus += 6;
	}
	else if (has_feat(stats, 492))
	{
		bonus += 3;
	}
	*(uint8_t*)0x081cc13a = bonus;
	return (bonus > 0);
}

unsigned char d_ret_code_unarmeddice[0x20];
uint8_t (*GetUnarmedDamageDice_Org)(CNWSCreatureStats*) = (uint8_t (*)(CNWSCreatureStats*))&d_ret_code_unarmeddice;
int GetUnarmedDamageDice_HookProc(CNWSCreatureStats* stats)
{
	if (stats->cs_is_pc && stats->cs_classes_len == 1 && stats->cs_classes[0].cl_class == 5 && GetLevel(stats, 0) == 40)
	{
		if (stats->cs_original->cre_size > 2)
		{
			return 2;
		}
		else
		{
			return 3;
		}
	}

	return GetUnarmedDamageDice_Org(stats);
}
unsigned char d_ret_code_unarmeddie[0x20];
uint8_t (*GetUnarmedDamageDie_Org)(CNWSCreatureStats*) = (uint8_t (*)(CNWSCreatureStats*))&d_ret_code_unarmeddie;
int GetUnarmedDamageDie_HookProc(CNWSCreatureStats* stats)
{
	if (stats->cs_is_pc && stats->cs_classes_len == 1 && stats->cs_classes[0].cl_class == 5 && GetLevel(stats, 0) == 40)
	{
		if (stats->cs_original->cre_size > 2)
		{
			return 12;
		}
		else
		{
			return 6;
		}
	}

	return GetUnarmedDamageDie_Org(stats);
}

unsigned char d_ret_code_onhitdispel[0x20];
int (*ApplyOnHitDispel_Org)(CNWSCreature*, CNWSObject*, CNWItemProperty*) = (int (*)(CNWSCreature*, CNWSObject*, CNWItemProperty*))&d_ret_code_onhitdispel;
int ApplyOnHitDispel_HookProc(CNWSCreature* creature, CNWSObject* target, CNWItemProperty* ip)
{
	//paladin
	CNWSCreatureStats* stats = creature->cre_stats;
	uint8_t dispel_dc = 10;
	uint8_t paladin_level = 0;
	uint8_t class_index;
	for (class_index=0; class_index<stats->cs_classes_len; class_index++)
	{
		if (stats->cs_classes[class_index].cl_class == 6)
		{
			paladin_level = stats->cs_classes[class_index].cl_level;
			break;
		}
	}
	if (paladin_level)
	{
		if (stats->cs_is_pc && GetLevel(stats, 0) == 40)
		{
			if (paladin_level == 40)
			{
				dispel_dc += 15;
			}
			else if (stats->cs_classes_len == 2 && paladin_level >= 30)
			{
				dispel_dc += 5;
			}
		}
	}
	*(uint8_t*)0x080f4e59 = dispel_dc;
	return ApplyOnHitDispel_Org(creature, target, ip);
}

unsigned char d_ret_code_favoredbonus[0x20];
int (*GetFavoredEnemyBonus_Org)(CNWSCreatureStats*, CNWSCreature*) = (int (*)(CNWSCreatureStats*, CNWSCreature*))&d_ret_code_favoredbonus;
int GetFavoredEnemyBonus_HookProc(CNWSCreatureStats* stats, CNWSCreature* target)
{
	int favored_bonus = GetFavoredEnemyBonus_Org(stats, target);
	if (favored_bonus > 0)
	{
		if (stats->cs_is_pc && GetLevel(stats, 0) == 40)
		{
			if (stats->cs_classes_len == 1)
			{
				favored_bonus += 3;
			}
			else if (stats->cs_classes_len == 2)
			{
				favored_bonus += 1;
			}
		}
	}
	return favored_bonus;
}
	
void init()
{
	if (setting::load_server_bool("X_ENABLE_SINGLE_DUAL_CLASSES_BONUS", true, "Enable the bonus for level 40 characters that use only 1 or 2 classes.")==false) return;
	
	//fighters weapon spec
	enable_write(0x08148769); //get damage bonus
	enable_write(0x0814875b);
	*(uint32_t*)(0x0814875b) = ((uint32_t)OnGetDamageBonus_GetWeaponSpec-(uint32_t)0x0814875f);
	enable_write(0x0814371d); //get melee damage bonus
	enable_write(0x08143710);
	*(uint32_t*)(0x08143710) = ((uint32_t)OnGetMeleeDamageBonus_GetWeaponSpec-(uint32_t)0x08143714);
	//weapon masters ab
	hook_call(0x0814331c, (uint32_t)OnGetAB_HasSuperiorWeapon);
	enable_write(0x08143324);
	hook_call(0x0814510B, (uint32_t)OnGetABVersus_HasSuperiorWeapon);
	enable_write(0x08145118);
	//rogues extra sneak damage
	enable_write(0x0814b7a0);
	*(uint32_t*)(0x0814b7a0) = ((uint32_t)RollSneakAttackDamage-(uint32_t)0x0814b7a4);
	enable_write(0x0814b246);
	*(uint32_t*)(0x0814b246) = ((uint32_t)GetHasSneakAttackFeat-(uint32_t)0x0814b24a);
	//barbarians extra damage reduction
	enable_write(0x081cc12c);
	*(uint32_t*)(0x081cc12c) = ((uint32_t)GetEpicDamageReductionFeatsBonus-(uint32_t)0x081cc130);
	enable_write(0x081cc13a);
	*(uint8_t*)0x081cc13a = 0; //default: 0 damage reduction
	//paladin holysword dispel
	enable_write(0x080f4e59);
	hook_function(0x080f4dc4, (unsigned long)ApplyOnHitDispel_HookProc, d_ret_code_onhitdispel, 12);
	
	hook_function(0x08143f88, (unsigned long)GetUnarmedDamageDice_HookProc, d_ret_code_unarmeddice, 12);
	hook_function(0x0814408c, (unsigned long)GetUnarmedDamageDie_HookProc, d_ret_code_unarmeddie, 12);

	hook_function(0x0815e9e0, (unsigned long)GetFavoredEnemyBonus_HookProc, d_ret_code_favoredbonus, 12);
}
REGISTER_INIT(init);
}