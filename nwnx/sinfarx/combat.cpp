#include "core.h"
#include "cached2da.h"
#include "creature.h"

#include <sys/time.h>

using namespace nwnx::core;
using namespace nwnx::creature;
using namespace nwnx::cached2da;

namespace nwnx { namespace combat {

inline C2da* weapons_feats() {return get_cached_2da("weapons_feats");}

unsigned char d_ret_code_runactions[0x20];
void (*RunActions_Org)(CNWSObject*, uint32_t, uint32_t, uint32_t) = (void (*)(CNWSObject*, uint32_t, uint32_t, uint32_t))&d_ret_code_runactions;
void RunActions_HookProc(CNWSObject* object, uint32_t p2, uint32_t p3, uint32_t p4)
{
	if (object->obj_type == 5)
	{
		CNWSCreature* creature = (CNWSCreature*)object;
		CExoLinkedList* action_list = &creature->obj.obj_actions;
		int nActionCount = action_list->header->len;
		CExoLinkedListNode* action_node = action_list->header->first;
		for (int nAction=0; nAction<nActionCount; nAction++)
		{
			CNWSAction* action = (CNWSAction*)action_node->data;
			int nActionType = action->act_type;
			if (nActionType == 1) //move to point
			{
				break;
			}
			else if (nActionType == 12) //attack (target = 52)
			{/*	//to get the target id...
				for (int i=0; i<100; i++)
				{
					nDebugValue = i;
					run_script("debug", *(uint32_t*)((char*)action_node->data+i));
				}*/

				CNWSItem* weapon = get_item_in_slot(creature->cre_equipment, 16);
				if (weapon != NULL)
				{
					char ammunition_type_str[12];
					if (weapons_feats()->GetString("AMMUNITION_TYPE", weapon->it_baseitem, ammunition_type_str, 12)) //ranged weapon?
					{
						timeval* tv_lastmove = &(GetCreatureExtra(creature)->lastmove);
						timeval tv;
						gettimeofday(&tv, NULL);
						timersub(&tv, tv_lastmove, &tv);
						timeval tv_delay;
						tv_delay.tv_sec = 1; tv_delay.tv_usec = 500000;
						if (timercmp(&tv, &tv_delay, <))
						{
							return;
						}
					}
					/*else //attack with a melee weapon
					{
					}*/
				}
			}
			else if (nActionType == 15) //cast spell (target = 72)
			{
				timeval* tv_lastmove = &(GetCreatureExtra(creature)->lastmove);
				timeval tv;
				gettimeofday(&tv, NULL);
				timersub(&tv, tv_lastmove, &tv);
				timeval tv_delay;
				tv_delay.tv_sec = 1; tv_delay.tv_usec = 500000;
				if (timercmp(&tv, &tv_delay, <))
				{
					return;
				}
			}//*/
			else if (nActionType == 46) //use item
			{
				timeval* tv_lastmove = &(GetCreatureExtra(creature)->lastmove);
				timeval tv;
				gettimeofday(&tv, NULL);
				timersub(&tv, tv_lastmove, &tv);
				timeval tv_delay;
				tv_delay.tv_sec = 1; tv_delay.tv_usec = 750000;
				if (timercmp(&tv, &tv_delay, <))
				{
					return;
				}
			}//*/
			action_node = action_node->next;
		}
	}

	return RunActions_Org(object, p2, p3, p4);
}

bool bNewDamageRollCall = false;
unsigned char d_ret_code_getdmgroll[0x20];
int (*GetDamageRoll_Org)(CNWSCreatureStats*, CNWSObject*, int, int, int, int, int) = (int (*)(CNWSCreatureStats*, CNWSObject*, int, int, int, int, int))&d_ret_code_getdmgroll;
int GetDamageRoll_HookProc(CNWSCreatureStats* stats, CNWSObject* target, int i1, int i2, int i3, int i4, int i5)
{
	bNewDamageRollCall = true;

	return GetDamageRoll_Org(stats, target, i1, i2, i3, i4, i5);
}

int (*DoSavingThrowRoll)(CNWSCreature*, uint8_t, uint16_t, uint8_t, uint32_t, int, uint16_t, int) =
	(int (*)(CNWSCreature*, uint8_t, uint16_t, uint8_t, uint32_t, int, uint16_t, int))0x080f0a90;
int (*GetCriticalHitMultiplier)(CNWSCreatureStats*, int) = (int (*)(CNWSCreatureStats*, int))0x0814c4a0;
uint16_t (*GetHasWeaponDevastatingCriticalFeat)(CNWSCreatureStats*, CNWSItem*) = (uint16_t (*)(CNWSCreatureStats*, CNWSItem*))0x08156ccc;
CNWSItem* (*GetCurrentAttackWeapon)(CNWSCombatRound*, int) = (CNWSItem* (*)(CNWSCombatRound*, int))0x80e3778;
bool bGetCritMultInternalCall = false;
int nLastMultiplier;
unsigned char d_ret_code_getcritmul[0x20];
int (*GetCriticalHitMultiplier_Org)(CNWSCreatureStats*, int) = (int (*)(CNWSCreatureStats*, int))&d_ret_code_getcritmul;
int GetCriticalHitMultiplier_HookProc(CNWSCreatureStats* stats, int p2)
{
	if (bGetCritMultInternalCall)
	{
		return GetCriticalHitMultiplier_Org(stats, p2);
	}

	bGetCritMultInternalCall = true;
	int nMultiplier = GetCriticalHitMultiplier(stats, p2);
	bGetCritMultInternalCall = false;

	CNWSCreature* creature = stats->cs_original;
	CGameObject* go_target = GetGameObject(creature->obj.obj_action_target);
	if (go_target == NULL) return nMultiplier;
	else if (!bNewDamageRollCall) return nLastMultiplier;

	CNWSItem* weapon = GetCurrentAttackWeapon(creature->cre_combat_round, 0)/*get_item_in_slot(creature->cre_equipment, 16)*/;
	if (weapon != NULL)
	{
		if (GetHasWeaponDevastatingCriticalFeat(stats, weapon))
		{
			if (go_target->type == 5)//its a creature?
			{
				CNWSCreature* target = go_target->vtable->AsNWSCreature(go_target);
				int nSaveDC = 10 + stats->cs_str_mod + GetLevel(stats, 0)/2;
				if (!DoSavingThrowRoll(target, 1, nSaveDC, 0, creature->obj.obj_id, 1, 0, 0/*1=delay?*/))
				{
					nMultiplier *= 2;
				}
			}
		}
	}
	bNewDamageRollCall = false;
	nLastMultiplier = nMultiplier;
	return nMultiplier;
}

int ByPass_GetHasWeaponDevastatingCriticalFeat(CNWSCreatureStats* stat, CNWSItem* weapon)
{
	return false;
}

#define SPELL_TIME_STOP 185
unsigned char d_ret_code_rsdmg[0x20];
int (*GetHasSpellEffect)(CNWSObject*, uint32_t) = (int (*)(CNWSObject*, uint32_t))0x081d53dc;
int (*ResolveDamage_Org)(CNWSCreature*, CNWSObject*) = (int (*)(CNWSCreature*, CNWSObject*))&d_ret_code_rsdmg;
int ResolveDamage_HookProc(CNWSCreature* creature, CNWSObject* target)
{
	if (GetHasSpellEffect(target, SPELL_TIME_STOP))
	{
		for (uint32_t nEffect=0; nEffect<target->obj_effects_len; nEffect++)
		{
			CGameEffect* effect = *(target->obj_effects+nEffect);
			if (effect->eff_spellid == SPELL_TIME_STOP &&
				effect->eff_creator != creature->obj.obj_id)
			{
				return 0;
			}
		}
	}

	return ResolveDamage_Org(creature, target);
}

int (*GetDead)(CNWSObject*) = (int (*)(CNWSObject*))0x081d1c10;
void (*UpdateAttackTargetForAllActions)(CNWSCombatRound* combat_round, uint32_t new_target_id) = (void (*)(CNWSCombatRound*, uint32_t))0x080E54C0;
bool resolving_attack = false;
unsigned char d_ret_code_resolveattack[0x20];
int (*ResolveAttack_Org)(CNWSCreature*, uint32_t, int, int) = (int (*)(CNWSCreature*, uint32_t, int, int))&d_ret_code_resolveattack;
int ResolveAttack_HookProc(CNWSCreature* creature, uint32_t target_id, int p3, int p4)
{
	resolving_attack = true;
	int nRet = ResolveAttack_Org(creature, target_id, p3, p4);
	resolving_attack = false;

	CREATURE_EXTRA* creature_extra = GetCreatureExtra(creature);
	uint32_t last_circle_kick_real_target = creature_extra->last_circle_kick_real_target;
	if (last_circle_kick_real_target!=OBJECT_INVALID &&
		last_circle_kick_real_target!=target_id)
	{
		CNWSObject* object = GetObjectById(last_circle_kick_real_target);
		if (object!=NULL &&
			!GetDead(object))
		{
			CNWSAction* action = creature->obj.obj_ai_action;
			if (action)
			{
				action->field_34 = last_circle_kick_real_target;
				creature->cre_attempted_target = last_circle_kick_real_target;
				creature->obj.obj_action_target = last_circle_kick_real_target;
				UpdateAttackTargetForAllActions(creature->cre_combat_round, last_circle_kick_real_target);
			}
		}
		creature_extra->last_circle_kick_real_target = OBJECT_INVALID;
	}

	return nRet;
}

unsigned char d_ret_code_attackroll[0x20];
int (*ResolveAttackRoll_Org)(CNWSCreature*, CNWSObject*) = (int (*)(CNWSCreature*, CNWSObject*))&d_ret_code_attackroll;
int ResolveAttackRoll_HookProc(CNWSCreature* creature, CNWSObject* target)
{
	int nRet = ResolveAttackRoll_Org(creature, target);

	if (resolving_attack)
	{
		if (get_local_string((CNWSObject*)creature, "EVENT_ATTACKROLL"))
		{
			run_script("inc_ev_attckroll", ((CNWSObject*)creature)->obj_id);
		}
		CNWSItem* weapon = GetCurrentAttackWeapon(creature->cre_combat_round, 0);
		if (weapon != NULL && get_local_string(&(weapon->obj), "EVENT_ATTACKROLL"))
		{
			run_script("inc_ev_weapsmash", weapon->obj.obj_id);
		}
	}

	return nRet;
}

unsigned char d_ret_code_addcirclekick[0x20];
int (*AddCircleKickAttack_Org)(CNWSCombatRound*, uint32_t) = (int (*)(CNWSCombatRound*, uint32_t))&d_ret_code_addcirclekick;
int AddCircleKickAttack_HookProc(CNWSCombatRound* combat_round, uint32_t target_id)
{
	CNWSCreature* creature = combat_round->creature;
	GetCreatureExtra(creature)->last_circle_kick_real_target = creature->cre_attack_target;
	return AddCircleKickAttack_Org(combat_round, target_id);
}

unsigned char d_ret_code_startcombatround[0x20];
int (*StartCombatRound_Org)(CNWSCombatRound*, uint32_t) = (int (*)(CNWSCombatRound*, uint32_t))&d_ret_code_startcombatround;
int StartCombatRound_HookProc(CNWSCombatRound* combat_round, uint32_t target_id)
{
	CNWSCreature* creature = combat_round->creature;
	CREATURE_EXTRA* creature_extra = GetCreatureExtra(creature);
	creature_extra->last_circle_kick_real_target = OBJECT_INVALID;
	combat_round->next_target_id = OBJECT_INVALID;

	return StartCombatRound_Org(combat_round, target_id);
}

unsigned char d_ret_code_setstealth[0x20];
int (*SetStealthMode_Org)(CNWSCreature*, uint8_t) = (int (*)(CNWSCreature*, uint8_t))&d_ret_code_setstealth;
int SetStealthMode_HookProc(CNWSCreature* creature, uint8_t is_active)
{
	if (creature->cre_is_pc)
	{
		if (is_active)
		{
			timeval* tv_lastmove = &(GetCreatureExtra(creature)->lastmove);
			gettimeofday(tv_lastmove, NULL);

			run_script("pc_ev_stealth", creature->obj.obj_id);
		}
		else
		{
			run_script("pc_ev_unstealth", creature->obj.obj_id);
		}
	}

	return SetStealthMode_Org(creature, is_active);
}

int (*GetIsInUseRange)(CNWSCreature*, uint32_t, float, int) = (int (*)(CNWSCreature*, uint32_t, float, int))0x08137730;
int GetTauntIsInUseRange(CNWSCreature* creature, uint32_t target_id, float distance, int p4)
{
	if (creature->cre_taunt_anim_played)
	{
		return true;
	}

	return GetIsInUseRange(creature, target_id, distance, p4);
}

int DoDeathAttackSave(CNWSCreature* creature, uint8_t p1, uint16_t p2, uint8_t p3, uint32_t assassin_id, int p5, uint16_t p6, int p7)
{
	run_script("s_ev_deathattack", assassin_id);
	return 1;
}

unsigned char d_ret_code_getspellhealaction[0x20];
int (*GetHasSpellOrHealAction_Org)(CNWSCreature*) = (int (*)(CNWSCreature*))&d_ret_code_getspellhealaction;
int GetHasSpellOrHealAction_HookProc(CNWSCreature* creature)
{
	/*//disable aoo option
	if (get_local_int((CNWSObject*)creature, "NO_AOO")) return true;*/

	//check for taunt too
	CExoLinkedList* actions_list = &creature->obj.obj_actions;
	int nActionCount = actions_list->header->len;
	CExoLinkedListNode* action_node = actions_list->header->first;
	for (int nAction=0; nAction<nActionCount; nAction++)
	{
		if (((CNWSAction*)action_node->data)->act_type == 43/*taunt*/)
		{
			return true;
		}
		action_node = action_node->next;
	}

	return GetHasSpellOrHealAction_Org(creature);
}

int HasCripplingStrike(CNWSCreatureStats* stats, uint16_t feat_id)
{
	return false;
}

CNWSCreature* last_post_damage_creature = NULL;
CNWSCombatAttackData* OnPostDamage_GetAttack(CNWSCombatRound* combat_round)
{
	last_post_damage_creature = combat_round->creature;
	return GetAttack(combat_round, combat_round->combat_current_attack);
}

int (*GetTotalDamage)(CNWSCombatAttackData*, int) = (int (*)(CNWSCombatAttackData*, int))0x80e4cec;
int OnPostDamage_GetTotalDamage(CNWSCombatAttackData* attack_data, int p2)
{
	int total_damage = GetTotalDamage(attack_data, p2);
	if (total_damage > 0 &&
		attack_data->is_sneak == true &&
		last_post_damage_creature != NULL &&
		has_feat(last_post_damage_creature->cre_stats, 222))
	{
		run_script("s_ev_crippstrike", last_post_damage_creature->obj.obj_id);
	}
	return total_damage;
}

void init()
{
	hook_function(0x080f975c, (unsigned long)SetStealthMode_HookProc, d_ret_code_setstealth, 12);
	hook_function(0x0813b980, (unsigned long)GetHasSpellOrHealAction_HookProc, d_ret_code_getspellhealaction, 12);
    
	hook_function(0x081c213c, (unsigned long)RunActions_HookProc, d_ret_code_runactions, 12);
	hook_function(0x0814c4a0, (unsigned long)GetCriticalHitMultiplier_HookProc, d_ret_code_getcritmul, 9);
	hook_function(0x0814a8f0, (unsigned long)GetDamageRoll_HookProc, d_ret_code_getdmgroll, 12);
    
    //disable dev crit
	enable_write(0x080ed082);
	*(uint32_t*)0x080ed082 = ((uint32_t)ByPass_GetHasWeaponDevastatingCriticalFeat-0x080ed086);
	enable_write(0x080ed9b6);
	*(uint32_t*)0x080ed9b6 = ((uint32_t)ByPass_GetHasWeaponDevastatingCriticalFeat-0x080ed9ba);
    
	//redirect Do Death Attack Save call
	enable_write(0x080ec88b);
	*(uint32_t*)(0x080ec88b) = ((uint32_t)DoDeathAttackSave-(uint32_t)0x080ec88f);

	//redirect GetIsInRange taunt call
	enable_write(0x08109d49);
	*(uint32_t*)(0x08109d4A) = ((uint32_t)GetTauntIsInUseRange-(uint32_t)0x08109d4e);

	//redirect has_feat for crippling strike
	hook_call(0x080ed1f9, (uint32_t)HasCripplingStrike);
	hook_call(0x080ecfae, (uint32_t)OnPostDamage_GetAttack);
	hook_call(0x080ecfc4, (uint32_t)OnPostDamage_GetTotalDamage);
	hook_call(0x080ed812, (uint32_t)OnPostDamage_GetAttack);
	hook_call(0x080ed825, (uint32_t)OnPostDamage_GetTotalDamage);
    
	hook_function(0x080ec5b4, (unsigned long)ResolveDamage_HookProc, d_ret_code_rsdmg, 12);
	hook_function(0x080e6c44, (unsigned long)ResolveAttack_HookProc, d_ret_code_resolveattack, 12);
	hook_function(0x080eb190, (unsigned long)ResolveAttackRoll_HookProc, d_ret_code_attackroll, 12);
    
	hook_function(0x080e2f94, (unsigned long)AddCircleKickAttack_HookProc, d_ret_code_addcirclekick, 12);
	hook_function(0x080e11e8, (unsigned long)StartCombatRound_HookProc, d_ret_code_startcombatround, 12);
    
    //prepare to control the damage
	enable_write(0x080ec5b4); //CNWSCreature::ResolveDamage
}
REGISTER_INIT(init);
 
}
}