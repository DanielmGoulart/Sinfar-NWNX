#include "core.h"
#include "creature.h"
#include "script_event.h"

using namespace nwnx::core;
using namespace nwnx::creature;
using namespace nwnx::script_event;

namespace nwnx { namespace polymorph {

unsigned char d_ret_code_equipmelee[0x20];
int (*EquipMostDamagedMeleeWeapon_Org)(CNWSCreature*, uint32_t, int, int) = (int (*)(CNWSCreature*, uint32_t, int, int))&d_ret_code_equipmelee;
int EquipMostDamagedMeleeWeapon_HookProc(CNWSCreature* creature, uint32_t slot, int p3, int p4)
{
	if (creature->cre_is_poly) return 0;
	return EquipMostDamagedMeleeWeapon_Org(creature, slot, p3, p4);
}
unsigned char d_ret_code_equipranged[0x20];
int (*EquipMostDamagedRangedWeapon_Org)(CNWSCreature*, uint32_t) = (int (*)(CNWSCreature*, uint32_t))&d_ret_code_equipranged;
int EquipMostDamagedRangedWeapon_HookProc(CNWSCreature* creature, uint32_t slot)
{
	if (creature->cre_is_poly) return 0;
	return EquipMostDamagedRangedWeapon_Org(creature, slot);
}
unsigned char d_ret_code_equiparmor[0x20];
int (*EquipMostEffectiveArmor_Org)(CNWSCreature*) = (int (*)(CNWSCreature*))&d_ret_code_equiparmor;
int EquipMostEffectiveArmor_HookProc(CNWSCreature* creature)
{
	if (creature->cre_is_poly) return 0;
	return EquipMostEffectiveArmor_Org(creature);
}

uint32_t (*GetSlotFromItem)(CNWSInventory*, CNWSItem*) = (uint32_t (*)(CNWSInventory*, CNWSItem*))0x0819ec6c;
int (*RealUnequipItem)(CNWSCreature*, CNWSItem*, int) = (int (*)(CNWSCreature*, CNWSItem*, int))0x0811b7b0;
int OnPolymorphUnequipItem(CNWSCreature* creature, CNWSItem* item, int p3)
{
	uint32_t nSlot = GetSlotFromItem(creature->cre_equipment, item);
	if (nSlot == 0x10000 || nSlot == 0x8000 || nSlot == 0x4000)
	{
		return RealUnequipItem(creature, item, p3);
	}
	return 0;
}
int (*RealEquipItem)(CNWSCreature*, uint32_t, CNWSItem*, int, int) = (int (*)(CNWSCreature*, uint32_t, CNWSItem*, int, int))0x0811b64c;
int OnUnPolymorphEquipItem(CNWSCreature* creature, uint32_t nSlot, CNWSItem* item, int p4, int p5)
{
	if (nSlot == 0x10000 || nSlot == 0x8000 || nSlot == 0x4000)
	{
		return RealEquipItem(creature, nSlot, item, p4, p5);
	}
	return 0;
}

unsigned char d_ret_code_usemonkabil[0x20];
int (*GetUseMonkAbilities_Org)(CNWSCreature*) = (int (*)(CNWSCreature*))&d_ret_code_usemonkabil;
int GetUseMonkAbilities_HookProc(CNWSCreature* creature)
{
	if (creature->cre_is_poly) return false;

	CNWSItem* left_hand_item = get_item_in_slot(creature->cre_equipment, 0x20);
	if (left_hand_item)
	{
		if (get_base_item(left_hand_item->it_baseitem)->bi_base_ac  > 0) return false;
	}

	return GetUseMonkAbilities_Org(creature);
}

unsigned char d_ret_code_polymorph[0x20];
int (*Polymorph_Org)(CNWSCreature*, int, CGameEffect*, int) = (int (*)(CNWSCreature*, int, CGameEffect*, int))&d_ret_code_polymorph;
int Polymorph_HookProc(CNWSCreature* creature, int p2, CGameEffect* effect, int p4)
{
	script_event::run("s_ev_prepolymrph", creature->obj.obj_id, {*(effect->eff_integers)});
	int nRet = Polymorph_Org(creature, p2, effect, p4);
	script_event::run("s_ev_pstpolymrph", creature->obj.obj_id, {*(effect->eff_integers)});
	return nRet;
}

unsigned char d_ret_code_unpolymorph[0x20];
int (*UnPolymorph_Org)(CNWSCreature*, CGameEffect*) = (int (*)(CNWSCreature*, CGameEffect*))&d_ret_code_unpolymorph;
int UnPolymorph_HookProc(CNWSCreature* creature, CGameEffect* effect)
{
	int nRet = UnPolymorph_Org(creature, effect);
	script_event::run("s_ev_unpolymorph", creature->obj.obj_id);
	return nRet;
}

void init()
{
	//redirect unequip when polymorphing (and bypass for weapon and hide)
	enable_write(0x08135e14);
	*(uint32_t*)(0x08135e14) = ((uint32_t)OnPolymorphUnequipItem-(uint32_t)0x08135e18);
	*(uint8_t*)(0x08136aca) = 0xEB;
	*(uint8_t*)(0x08136ba6) = 0xEB;
	//"" equip item when unpolymorphing
	enable_write(0x08136c07);
	*(uint32_t*)(0x08136c07) = ((uint32_t)OnUnPolymorphEquipItem-(uint32_t)0x08136c0b);
	//can unequip weapon when polymorphed
	enable_write(0x08107e8f);
	*(uint32_t*)0x08107e8f = 0x90909090;
	*(uint16_t*)(0x08107e8f+4) = 0x9090;
	enable_write(0x0811b7c3);
	*(uint8_t*)0x0811b7c3 = 0xEB;
	
	hook_function(0x081241f8, (unsigned long)GetUseMonkAbilities_HookProc, d_ret_code_usemonkabil, 12);

	hook_function(0x081356a0, (unsigned long)Polymorph_HookProc, d_ret_code_polymorph, 12);
	hook_function(0x081369f8, (unsigned long)UnPolymorph_HookProc, d_ret_code_unpolymorph, 12);
	
	//prevent double applying properties when polymorphed
	hook_function(0x08130418, (unsigned long)EquipMostDamagedMeleeWeapon_HookProc, d_ret_code_equipmelee, 12);
	hook_function(0x08130a1c, (unsigned long)EquipMostDamagedRangedWeapon_HookProc, d_ret_code_equipranged, 12);
	hook_function(0x081312dc, (unsigned long)EquipMostEffectiveArmor_HookProc, d_ret_code_equiparmor, 12);

	//do not "re-enter" area after polymorphing
	enable_write(0x08136443);
	*(uint16_t*)0x08136443 = 0xe990;	
}
REGISTER_INIT(init);
	
}
}