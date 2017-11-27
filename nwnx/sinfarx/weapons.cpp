#include "core.h"
#include "cached2da.h"

using namespace nwnx::core;
using namespace nwnx::cached2da;

namespace nwnx { namespace weapons {

inline C2da* weapons_feats() {return get_cached_2da("weapons_feats");}

void (*RunEquipItem)(CNWSCreature*, uint32_t, uint32_t, uint32_t) = (void (*)(CNWSCreature*, uint32_t, uint32_t, uint32_t))0x08116f20;
int (*SendFeedbackMessageRef)(CNWSCreature*, uint16_t, CNWCCMessageData*, CNWSPlayer*) = (int (*)(CNWSCreature*, uint16_t, CNWCCMessageData*, CNWSPlayer*))0x813533c;
int (*CanEquipItem)(CNWSCreature*, CNWSItem*, uint32_t*, int, int, int, CNWSPlayer*) = (int (*)(CNWSCreature*, CNWSItem*, uint32_t*, int, int, int, CNWSPlayer*))0x80ff978;
uint32_t (*FindItemWithBaseItemId)(CItemRepository*, uint32_t, int) = (uint32_t (*)(CItemRepository*, uint32_t, int))0x081a3ee4;
int (*GetPropertyByTypeExists)(CNWSItem*, uint16_t, uint16_t) = (int (*)(CNWSItem*, uint16_t, uint16_t))0x81a2a6c;
int GetAmmunitionAvailable_HookProc(CNWSCreature* creature, int num_ammunition_needed)
{
	CNWSItem* weapon = get_item_in_slot(creature->cre_equipment, 0x10);
	if (weapon)
	{
		if (GetPropertyByTypeExists(weapon, 0x3d, 0)) return num_ammunition_needed;
		char ammunition_type_str[12];
		if (weapons_feats()->GetString("AMMUNITION_TYPE", weapon->it_baseitem, ammunition_type_str, 12))
		{
			uint32_t ammunition_type = atoi(ammunition_type_str);
			CNWBaseItem* ammunition_baseitem = get_base_item(ammunition_type);
			if (ammunition_baseitem)
			{
				uint32_t ammunition_slot = ammunition_baseitem->bi_equip_slots;
				CNWSItem* equipped_ammunition = get_item_in_slot(creature->cre_equipment, ammunition_slot);
				if (equipped_ammunition && equipped_ammunition->it_baseitem==ammunition_type) return std::min(num_ammunition_needed, (int)equipped_ammunition->it_stack_size);
				//find an ammunition and equip it
				int find_ammunition_index = 0;
				uint32_t to_equip_ammunition_id = FindItemWithBaseItemId(creature->cre_inventory, ammunition_type, find_ammunition_index);
				while (to_equip_ammunition_id != OBJECT_INVALID)
				{
					CNWSItem* to_equip_ammunition = GetItemById(to_equip_ammunition_id);
					if (to_equip_ammunition != NULL)
					{
						if (CanEquipItem(creature, to_equip_ammunition, &ammunition_slot, 1, 0, 0, NULL))
						{
							RunEquipItem(creature, to_equip_ammunition_id, ammunition_slot, 0);
							//SetItemPossessor(to_equip_ammunition, creature->obj.obj_id, 0, 0, 0);
							//EquipItem(creature, ammunition_slot, to_equip_ammunition, 1, 0);
							return std::min(num_ammunition_needed, (int)to_equip_ammunition->it_stack_size);
						}
					}
					find_ammunition_index++;
					to_equip_ammunition_id = FindItemWithBaseItemId(creature->cre_inventory, ammunition_type, find_ammunition_index);
				}
			}
		}
	}
	SendFeedbackMessageRef(creature, 0x19, NULL, NULL);
	return 0;
}
void (*UpdateEncumbranceState)(CNWSCreature*, int) = (void (*)(CNWSCreature*, int))0x8111E94;
int (*RemoveCreatureItem)(CNWSCreature*, CNWSItem*, int, int, int, int) = (int (*)(CNWSCreature*, CNWSItem*, int, int, int, int))0x081000e8;
int (*AddEventDeltaTime)(CServerAIMaster*, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, void*) = (int (*)(CServerAIMaster*, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, void*))0x08096388;
void ResolveAmmunition_HookProc(CNWSCreature* creature, uint32_t p2)
{
	CNWSItem* weapon = get_item_in_slot(creature->cre_equipment, 0x10);
	if (weapon == NULL) return;
	if (GetPropertyByTypeExists(weapon, 0x3d, 0)) return;
	char ammunition_type_str[12];
	if (weapons_feats()->GetString("AMMUNITION_TYPE", weapon->it_baseitem, ammunition_type_str, 12)==NULL) return;
	uint32_t ammunition_type = atoi(ammunition_type_str);
	CNWBaseItem* ammunition_baseitem = get_base_item(ammunition_type);
	if (ammunition_baseitem == NULL) return;
	uint32_t ammunition_slot = ammunition_baseitem->bi_equip_slots;
	CNWSItem* equipped_ammunition = get_item_in_slot(creature->cre_equipment, ammunition_slot);
	if (equipped_ammunition == NULL || equipped_ammunition->it_baseitem != ammunition_type) return;
	if (equipped_ammunition->it_stack_size <= 1)
	{
		RemoveCreatureItem(creature, equipped_ammunition, false, true, false, true);
		//find an ammunition and equip it
		int find_ammunition_index = 0;
		uint32_t to_equip_ammunition_id = FindItemWithBaseItemId(creature->cre_inventory, ammunition_type, find_ammunition_index);
		while (to_equip_ammunition_id != OBJECT_INVALID)
		{
			CNWSItem* to_equip_ammunition = GetItemById(to_equip_ammunition_id);
			if (to_equip_ammunition)
			{
				if (CanEquipItem(creature, to_equip_ammunition, &ammunition_slot, 1, 0, 0, NULL))
				{
					RunEquipItem(creature, to_equip_ammunition_id, ammunition_slot, 0);
					break;
				}
			}
			find_ammunition_index++;
			to_equip_ammunition_id = FindItemWithBaseItemId(creature->cre_inventory, ammunition_type, find_ammunition_index);
		}
	}
	else
	{
		equipped_ammunition->it_stack_size--;
		CNWBaseItem* base_item = get_base_item(equipped_ammunition->it_baseitem);
		creature->cre_equipped_weight -= base_item->bi_weight;
		UpdateEncumbranceState(creature, 1);
	}
}
void (*CNWSCombatAttackData_Constructor)(CNWSCombatAttackData*) = (void (*)(CNWSCombatAttackData*))0x80e01d4;
void (*CNWSCombatAttackData_Copy)(CNWSCombatAttackData*, CNWSCombatAttackData*, int) = (void (*)(CNWSCombatAttackData*, CNWSCombatAttackData*, int))0x80e0430;
unsigned char d_ret_code_resolvesafeprojectile[0x20];
void ResolveSafeProjectile_HookProc(CNWSCreature* creature, uint32_t p2, int attack_id)
{
	CNWSItem* weapon = get_item_in_slot(creature->cre_equipment, 0x10);
	if (weapon == NULL) return;
	char ammunition_type_str[12];
	if (weapons_feats()->GetString("AMMUNITION_TYPE", weapon->it_baseitem, ammunition_type_str, 12)==NULL) return;
	uint32_t ammunition_type = atoi(ammunition_type_str);
	CNWBaseItem* ammunition_baseitem = get_base_item(ammunition_type);
	if (ammunition_baseitem == NULL) return;
	uint32_t ammunition_slot = ammunition_baseitem->bi_equip_slots;
	CNWSItem* equipped_ammunition = get_item_in_slot(creature->cre_equipment, ammunition_slot);
	if (equipped_ammunition == NULL || equipped_ammunition->it_baseitem != ammunition_type) return;

	CNWSCombatAttackData* attack_data = GetAttack(creature->cre_combat_round, attack_id);
	attack_data->attack_projectile = equipped_ammunition->obj.obj_id;

	CNWSCombatAttackData* new_attack_data = (CNWSCombatAttackData*)malloc(0xa8);
	CNWSCombatAttackData_Constructor(new_attack_data);
	CNWSCombatAttackData_Copy(new_attack_data, attack_data, 0);

	AddEventDeltaTime(server_internal->srv_ai, 0, p2, creature->obj.obj_id, creature->obj.obj_id, 0x15 /*EVENT_BROADCAST_SAFE_PROJECTILE*/, new_attack_data);
}

unsigned char d_ret_code_weaponfocus[0x20];
unsigned char d_ret_code_epicweaponfocus[0x20];
unsigned char d_ret_code_weaponspecialization[0x20];
unsigned char d_ret_code_epicweaponspecialization[0x20];
unsigned char d_ret_code_weaponimprovedcritical[0x20];
unsigned char d_ret_code_weaponoverhelmingcritical[0x20];
unsigned char d_ret_code_weapondevastatingcritical[0x20];
unsigned char d_ret_code_weaponofchoice[0x20];
unsigned char d_ret_code_weaponfiness[0x20];
int (*GetWeaponFocus_Org)(CNWSCreatureStats*, CNWSItem*) = (int (*)(CNWSCreatureStats*, CNWSItem*))&d_ret_code_weaponfocus;
int (*GetEpicWeaponFocus_Org)(CNWSCreatureStats*, CNWSItem*) = (int (*)(CNWSCreatureStats*, CNWSItem*))&d_ret_code_epicweaponfocus;
int (*GetWeaponSpecialization_Org)(CNWSCreatureStats*, CNWSItem*) = (int (*)(CNWSCreatureStats*, CNWSItem*))&d_ret_code_weaponspecialization;
int (*GetEpicWeaponSpecialization_Org)(CNWSCreatureStats*, CNWSItem*) = (int (*)(CNWSCreatureStats*, CNWSItem*))&d_ret_code_epicweaponspecialization;
int (*GetWeaponImprovedCritical_Org)(CNWSCreatureStats*, CNWSItem*) = (int (*)(CNWSCreatureStats*, CNWSItem*))&d_ret_code_weaponimprovedcritical;
int (*GetWeaponOverhelmingCritical_Org)(CNWSCreatureStats*, CNWSItem*) = (int (*)(CNWSCreatureStats*, CNWSItem*))&d_ret_code_weaponoverhelmingcritical;
int (*GetWeaponDevastatingCritical_Org)(CNWSCreatureStats*, CNWSItem*) = (int (*)(CNWSCreatureStats*, CNWSItem*))&d_ret_code_weapondevastatingcritical;
int (*GetWeaponOfChoice_Org)(CNWSCreatureStats*, uint32_t) = (int (*)(CNWSCreatureStats*, uint32_t))&d_ret_code_weaponofchoice;
int (*GetWeaponFiness_Org)(CNWSCreatureStats*, CNWSItem*) = (int (*)(CNWSCreatureStats*, CNWSItem*))&d_ret_code_weaponfiness;
int GetWeaponFocus_HookProc(CNWSCreatureStats* stats, CNWSItem* weapon)
{
	char weapon_feat[12];
	if (weapon != NULL && weapons_feats()->GetString("WEAPON_FOCUS", weapon->it_baseitem, weapon_feat, 12))
	{
		return has_feat(stats, atoi(weapon_feat));
	}
	return GetWeaponFocus_Org(stats, weapon);
}
int GetEpicWeaponFocus_HookProc(CNWSCreatureStats* stats, CNWSItem* weapon)
{
	char weapon_feat[12];
	if (weapon != NULL && weapons_feats()->GetString("EPIC_WEAPON_FOCUS", weapon->it_baseitem, weapon_feat, 12))
	{
		return has_feat(stats, atoi(weapon_feat));
	}
	return GetEpicWeaponFocus_Org(stats, weapon);
}
int GetWeaponSpecialization_HookProc(CNWSCreatureStats* stats, CNWSItem* weapon)
{
	char weapon_feat[12];
	if (weapon != NULL && weapons_feats()->GetString("WEAPON_SPECIALIZATION", weapon->it_baseitem, weapon_feat, 12))
	{
		return has_feat(stats, atoi(weapon_feat));
	}
	else
	{
		return GetWeaponSpecialization_Org(stats, weapon);
	}
}
int GetEpicWeaponSpecialization_HookProc(CNWSCreatureStats* stats, CNWSItem* weapon)
{
	char weapon_feat[12];
	if (weapon != NULL && weapons_feats()->GetString("EPIC_WEAPON_SPECIALIZATION", weapon->it_baseitem, weapon_feat, 12))
	{
		return has_feat(stats, atoi(weapon_feat));
	}
	return GetEpicWeaponSpecialization_Org(stats, weapon);
}
int GetWeaponImprovedCritical_HookProc(CNWSCreatureStats* stats, CNWSItem* weapon)
{
	char weapon_feat[12];
	if (weapon != NULL && weapons_feats()->GetString("WEAPON_IMPROVED_CRITICAL", weapon->it_baseitem, weapon_feat, 12))
	{
		return has_feat(stats, atoi(weapon_feat));
	}
	return GetWeaponImprovedCritical_Org(stats, weapon);
}
int GetWeaponOverhelmingCritical_HookProc(CNWSCreatureStats* stats, CNWSItem* weapon)
{
	char weapon_feat[12];
	if (weapon != NULL && weapons_feats()->GetString("WEAPON_OVERHELMING_CRITICAL", weapon->it_baseitem, weapon_feat, 12))
	{
		return has_feat(stats, atoi(weapon_feat));
	}
	return GetWeaponOverhelmingCritical_Org(stats, weapon);
}
int GetWeaponDevastatingCritical_HookProc(CNWSCreatureStats* stats, CNWSItem* weapon)
{
	char weapon_feat[12];
	if (weapon != NULL && weapons_feats()->GetString("WEAPON_DEVASTATING_CRITICAL", weapon->it_baseitem, weapon_feat, 12))
	{
		return has_feat(stats, atoi(weapon_feat));
	}
	return GetWeaponDevastatingCritical_Org(stats, weapon);
}
int GetWeaponOfChoice_HookProc(CNWSCreatureStats* stats, uint32_t weapon_type)
{
	char weapon_feat[12];
	if (weapons_feats()->GetString("WEAPON_OF_CHOICE", weapon_type, weapon_feat, 12))
	{
		return has_feat(stats, atoi(weapon_feat));
	}
	return GetWeaponOfChoice_Org(stats, weapon_type);
}
int GetWeaponFiness_HookProc(CNWSCreatureStats* stats, CNWSItem* weapon)
{
	char weapon_finess[12];
	if (weapon != NULL && weapons_feats()->GetString("FINESS_SIZE", weapon->it_baseitem, weapon_finess, 12))
	{
		return (stats->cs_original->cre_size >= (uint32_t)atoi(weapon_finess) && has_feat(stats, 42));
	}
	return GetWeaponFiness_Org(stats, weapon);
}

CNWSItem* OnCheckUseMonkAttack_GetItemInSlot(CNWSInventory* inventory, uint32_t slot)
{
	CNWSItem* weapon = get_item_in_slot(inventory, slot);
	if (weapon == NULL) return NULL;
	char is_monk_weapon[12];
	if (weapons_feats()->GetString("IS_MONK_WEAPON", weapon->it_baseitem, is_monk_weapon, 12) && atoi(is_monk_weapon))
	{
		return NULL;
	}
	else
	{
		return weapon;
	}

}
CNWSItem* OnSetCombatMonk_GetRightHandWeapon_CheckMonkWeapon(CNWSInventory* inventory, uint32_t slot)
{
	CNWSItem* weapon = get_item_in_slot(inventory, slot);
	char is_monk_weapon[12];
	if (weapon && weapons_feats()->GetString("IS_MONK_WEAPON", weapon->it_baseitem, is_monk_weapon, 12) && atoi(is_monk_weapon))
	{
		*(uint16_t*)0x080f9940 = 0x9090;
	}
	else
	{
		*(uint16_t*)0x080f9940 = 0x2375;
	}
	return weapon;
}
CNWSItem* OnSetCombatMonk_GetLeftHandWeapon_CheckMonkWeapon(CNWSInventory* inventory, uint32_t slot)
{
	CNWSItem* weapon = get_item_in_slot(inventory, slot);
	char is_monk_weapon[12];
	if (weapon && weapons_feats()->GetString("IS_MONK_WEAPON", weapon->it_baseitem, is_monk_weapon, 12) && atoi(is_monk_weapon))
	{
		*(uint16_t*)0x080f9956 = 0xE990;
	}
	else
	{
		*(uint16_t*)0x080f9956 = 0x840f;
	}
	return weapon;
}
CNWSItem* OnAttack_GetRightHandWeapon_CheckMonkWeapon(CNWSInventory* inventory, uint32_t slot)
{
	CNWSItem* weapon = get_item_in_slot(inventory, slot);
	char is_monk_weapon[12];
	if (weapon && weapons_feats()->GetString("IS_MONK_WEAPON", weapon->it_baseitem, is_monk_weapon, 12) && atoi(is_monk_weapon))
	{
		*(uint8_t*)0x080e23ec = 0xEB;
	}
	else
	{
		*(uint8_t*)0x080e23ec = 0x74;
	}
	return weapon;
}
CNWSItem* OnEquip_GetRightHandWeapon_CheckMonkWeapon(CNWSInventory* inventory, uint32_t slot)
{
	CNWSItem* weapon = get_item_in_slot(inventory, slot);
	char is_monk_weapon[12];
	if (weapon && weapons_feats()->GetString("IS_MONK_WEAPON", weapon->it_baseitem, is_monk_weapon, 12) && atoi(is_monk_weapon))
	{
		*(uint16_t*)0x081177b4 = 0x9090;
	}
	else
	{
		*(uint16_t*)0x081177b4 = 0x0875;
	}
	return weapon;
}

void init()
{
	hook_function(0x080e78a4, (unsigned long)GetAmmunitionAvailable_HookProc, d_ret_code_nouse, 9);
	hook_function(0x080e7754, (unsigned long)ResolveAmmunition_HookProc, d_ret_code_nouse, 9);
	hook_function(0x080e7630, (unsigned long)ResolveSafeProjectile_HookProc, d_ret_code_resolvesafeprojectile, 9);
    
    //for custom weapons feats
	hook_function(0x08155dec, (unsigned long)GetWeaponFocus_HookProc, d_ret_code_weaponfocus, 11);
	hook_function(0x08155ff0, (unsigned long)GetEpicWeaponFocus_HookProc, d_ret_code_epicweaponfocus, 11);
	hook_function(0x081562a4, (unsigned long)GetWeaponSpecialization_HookProc, d_ret_code_weaponspecialization, 11);
	hook_function(0x08156550, (unsigned long)GetEpicWeaponSpecialization_HookProc, d_ret_code_epicweaponspecialization, 11);
	hook_function(0x08156804, (unsigned long)GetWeaponImprovedCritical_HookProc, d_ret_code_weaponimprovedcritical, 11);
	hook_function(0x08156a18, (unsigned long)GetWeaponOverhelmingCritical_HookProc, d_ret_code_weaponoverhelmingcritical, 11);
	hook_function(0x08156ccc, (unsigned long)GetWeaponDevastatingCritical_HookProc, d_ret_code_weapondevastatingcritical, 11);
	hook_function(0x08160f50, (unsigned long)GetWeaponOfChoice_HookProc, d_ret_code_weaponofchoice, 12);
	hook_function(0x08155cf4, (unsigned long)GetWeaponFiness_HookProc, d_ret_code_weaponfiness, 10);
    
    //add monk weapons
	enable_write(0x08143f3d);
	*(uint32_t*)(0x08143f3d) = ((uint32_t)OnCheckUseMonkAttack_GetItemInSlot-(uint32_t)0x08143f41);
	enable_write(0x08143f29);
	*(uint32_t*)(0x08143f29) = ((uint32_t)OnCheckUseMonkAttack_GetItemInSlot-(uint32_t)0x08143f2d);
	//get weapon for combat mode: check monk weapon for flurry blows
	enable_write(0x080f9880);
	*(uint32_t*)(0x080f9880) = ((uint32_t)OnSetCombatMonk_GetRightHandWeapon_CheckMonkWeapon-(uint32_t)0x080f9884);
	enable_write(0x080f9904);
	*(uint32_t*)(0x080f9904) = ((uint32_t)OnSetCombatMonk_GetLeftHandWeapon_CheckMonkWeapon-(uint32_t)0x080f9908);
	//edit this jump if monk weapon or not:
	enable_write(0x080f9940); //right hand
	enable_write(0x080f9956); //left hand
	//on attack
	enable_write(0x080e227a);
	*(uint32_t*)(0x080e227a) = ((uint32_t)OnAttack_GetRightHandWeapon_CheckMonkWeapon-(uint32_t)0x080e227e);
	enable_write(0x080e23ec); //edit jump
	//on equip
	enable_write(0x0811775c);
	*(uint32_t*)(0x0811775c) = ((uint32_t)OnEquip_GetRightHandWeapon_CheckMonkWeapon-(uint32_t)0x08117760);
	enable_write(0x081177b4); //edit jump
    
}
REGISTER_INIT(init);
 
}
}