#include "core.h"

using namespace nwnx::core;

namespace
{

inline bool is_bow_baseitem(int baseitem)
{
	return (baseitem == 214 || baseitem == 215 || baseitem == 216);
}
inline bool is_bow(CNWSItem* item)
{
	return (item && is_bow_baseitem(item->it_baseitem));
}

CNWSItem* (*GetCurrentAttackWeapon)(CNWSCombatRound*, int) = (CNWSItem* (*)(CNWSCombatRound*, int))0x80e3778;
CNWSItem* OnGetAttackModVersus_GetCurrentWeapon(CNWSCombatRound* combat_round, int p2)
{
	CNWSItem* current_weapon = GetCurrentAttackWeapon(combat_round, p2);
	if (is_bow(current_weapon))
	{
		*(uint8_t*)0x08145656 = 0xEB;
	}
	else
	{
		*(uint8_t*)0x08145656 = 0x74;
	}
	return current_weapon;
}
CNWSItem* OnGetDamageBonus_GetCurrentWeapon(CNWSCombatRound* combat_round, int p2)
{
	CNWSItem* current_weapon = GetCurrentAttackWeapon(combat_round, p2);
	if (is_bow(current_weapon))
	{
		*(uint8_t*)0x08147e18 = 0xEB;
	}
	else
	{
		*(uint8_t*)0x08147e18 = 0x74;
	}
	return current_weapon;
}
CNWSItem* OnCaculateDamagePower_GetEquippedWeapon(CNWSInventory* inventory, uint32_t slot_flag)
{
	CNWSItem* item = get_item_in_slot(inventory, slot_flag);
	if (is_bow(item))
	{
		*(uint8_t*)0x0812ec70 = 0xEB;
	}
	else
	{
		*(uint8_t*)0x0812ec70 = 0x74;
	}
	return item;
}
CNWSItem* OnGetRangedDamageBonus_GetEquippedWeapon(CNWSInventory* inventory, uint32_t slot_flag)
{
	CNWSItem* item = get_item_in_slot(inventory, slot_flag);
	if (is_bow(item))
	{
		*(uint8_t*)0x08143c4f = 0xEB;
	}
	else
	{
		*(uint8_t*)0x08143c4f = 0x74;
	}
	return item;
}
CNWSItem* OnUseAAFeat_GetEquippedWeapon(CNWSInventory* inventory, uint32_t slot_flag)
{
	CNWSItem* item = get_item_in_slot(inventory, slot_flag);
	if (is_bow(item))
	{
		*(uint8_t*)0x0812ade9 = 0xEB;
	}
	else
	{
		*(uint8_t*)0x0812ade9 = 0x74;
	}
	return item;
}
CNWSItem* OnCastAAFeat_GetEquippedWeapon(CNWSInventory* inventory, uint32_t slot_flag)
{
	CNWSItem* item = get_item_in_slot(inventory, slot_flag);
	if (is_bow(item))
	{
		*(uint8_t*)0x080faeb0 = 0xEB;
	}
	else
	{
		*(uint8_t*)0x080faeb0 = 0x74;
	}
	return item;
}

void init()
{
	//custom longbow: consider aa feats
	enable_write(0x08145641);
	*(uint32_t*)(0x08145641) = ((uint32_t)OnGetAttackModVersus_GetCurrentWeapon-(uint32_t)0x08145645);
	enable_write(0x08145656); //jump if longbow
	enable_write(0x0814778a);
	*(uint32_t*)(0x0814778a) = ((uint32_t)OnGetDamageBonus_GetCurrentWeapon-(uint32_t)0x0814778e);
	enable_write(0x08147e18); //jump if longbow
	enable_write(0x0812ec5b);
	*(uint32_t*)(0x0812ec5b) = ((uint32_t)OnCaculateDamagePower_GetEquippedWeapon-(uint32_t)0x0812ec5f);
	enable_write(0x0812ec70); //jump if longbow
	enable_write(0x08143b74);
	*(uint32_t*)(0x08143b74) = ((uint32_t)OnGetRangedDamageBonus_GetEquippedWeapon-(uint32_t)0x08143b78);
	enable_write(0x08143c4f); //jump if longbow
	enable_write(0x0812add8);
	*(uint32_t*)(0x0812add8) = ((uint32_t)OnUseAAFeat_GetEquippedWeapon-(uint32_t)0x0812addc);
	enable_write(0x0812ade9); //jump if longbow
	enable_write(0x080fae9f);
	*(uint32_t*)(0x080fae9f) = ((uint32_t)OnCastAAFeat_GetEquippedWeapon-(uint32_t)0x080faea3);
	enable_write(0x080faeb0); //jump if longbow	
	
}
REGISTER_INIT(init);
	
}