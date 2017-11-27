#include "core.h"
#include "creature.h"
#include "script_event.h"

using namespace nwnx::core;
using namespace nwnx::creature;
using namespace nwnx::script_event;

namespace nwnx { namespace item {

CNWSItem* get_armor_ac_target = NULL;
unsigned char d_ret_code_getitemac[0x20];
int (*GetItemAC_Org)(CNWSItem*) = (int (*)(CNWSItem*))&d_ret_code_getitemac;
int GetItemAC_HookProc(CNWSItem* item)
{
	CNWBaseItem* baseitem = get_base_item(item->it_baseitem);
	if (baseitem->bi_base_ac > 0)
	{
		return baseitem->bi_base_ac;
	}
	get_armor_ac_target = item;
	return GetItemAC_Org(item);
}
unsigned char d_ret_code_computecreatureac[0x20];
void (*ComputeCreatureAC_Org)(CNWSCreature*, CNWSItem*, int, int) = (void (*)(CNWSCreature*, CNWSItem*, int, int))&d_ret_code_computecreatureac;
void ComputeCreatureAC_HookProc(CNWSCreature* creature, CNWSItem* armor, int p3, int p4)
{
	get_armor_ac_target = armor;
	return ComputeCreatureAC_Org(creature, armor, p3, p4);
}
int (*Get2daFloatEntry)(C2DA*, int, CExoString*, float*) = (int (*)(C2DA*, int, CExoString*, float*))0x82bf100;
int OnCompute_GetTorsoAC(C2DA* table, int row, CExoString* column, float* result)
{
	if (get_armor_ac_target)
	{
		CNWSScriptVarTable* var_table = &(get_armor_ac_target->obj.obj_vartable);
		uint32_t nVarCount = var_table->vt_len;
		for (uint32_t nVar=0; nVar<nVarCount; nVar++)
		{
			CScriptVariable* var = &(var_table->vt_list[nVar]);
			if (var->var_type == 1)//int
			{
				if (var->var_name.text && strncmp(var->var_name.text, "XAC", var->var_name.len) == 0)
				{
					*result = (float)var->var_value;
					return 1;
				}
			}
		}
	}
	return Get2daFloatEntry(table, row, column, result);
}
void (*ComputeWeight)(CNWSItem*) = (void (*)(CNWSItem*))0x081a137c;
unsigned char d_ret_code_loaditem[0x20];
int (*LoadItem_Org)(CNWSItem*, void*, void*, int) = (int (*)(CNWSItem*, void*, void*, int))&d_ret_code_loaditem;
int LoadItem_HookProc(CNWSItem* item, void* gff, void* gff_struct, int p4)
{
	int ret = LoadItem_Org(item, gff, gff_struct, p4);

	//recompute ac and weight (after that the variables are loaded)
	item->it_ac = GetItemAC_HookProc(item);
	ComputeWeight(item);

	return ret;
}

CNWSItem* OnComputeCreature_GetShieldItem(CNWSInventory* inventory, uint32_t slot)
{
	CNWSItem* item = get_item_in_slot(inventory, slot);
	if (item)
	{
		CNWBaseItem* baseitem = get_base_item(item->it_baseitem);
		if (baseitem->bi_base_ac > 0)
		{
			*(uint8_t*)0x08129cc5 = 0xEB;
			*(uint8_t*)0x08129e3a = 0xEB;
		}
		else
		{
			*(uint8_t*)0x08129cc5 = 0x74;
			*(uint8_t*)0x08129e3a = 0x74;
		}
	}
	return item;
}

unsigned char d_ret_code_applyip[0x20];
void (*ApplyItemProperties_Org)(CNWSItem*, CNWSCreature*, uint32_t, int) = (void (*)(CNWSItem*, CNWSCreature*, uint32_t, int))&d_ret_code_applyip;
void ApplyItemProperties_HookProc(CNWSItem* item, CNWSCreature* creature, uint32_t slot, int p4)
{
	CREATURE_EXTRA* creature_extra = GetCreatureExtra(creature);
	if (slot & creature_extra->slots_ip)
	{
		fprintf(stderr, "applying IP from item:%s in already used slot:%x for:%x|%s!\n", item->obj.obj_tag.text, slot, creature->obj.obj_id, creature->obj.obj_tag.text);
		print_backtrace();
	}
	else
	{
		creature_extra->slots_ip |= slot;
		return ApplyItemProperties_Org(item, creature, slot, p4);
	}
}
unsigned char d_ret_code_rmip[0x20];
void (*RemoveItemProperties_Org)(CNWSItem*, CNWSCreature*, uint32_t) = (void (*)(CNWSItem*, CNWSCreature*, uint32_t))&d_ret_code_rmip;
void RemoveItemProperties_HookProc(CNWSItem* item, CNWSCreature* creature, uint32_t slot)
{
	CREATURE_EXTRA* creature_extra = GetCreatureExtra(creature);
	if (slot & creature_extra->slots_ip)
	{
		creature_extra->slots_ip &= ~slot;
		return RemoveItemProperties_Org(item, creature, slot);
	}
	else
	{
		fprintf(stderr, "removing IP from item:%s in empty slot:%x for:%x|%s\n", item->obj.obj_tag.text, slot, creature->obj.obj_id, creature->obj.obj_tag.text);
		print_backtrace();
	}
}

unsigned char d_ret_code_equipaction[0x20];
int (*AddEquipItemAction_Org)(CNWSCreature*, CNWSItem*, uint32_t, int, int, uint32_t) = (int (*)(CNWSCreature*, CNWSItem*, uint32_t, int, int, uint32_t))&d_ret_code_equipaction;
int AddEquipItemAction_HookProc(CNWSCreature* creature, CNWSItem* item, uint32_t p3, int as_normal_action, int to_combat_round, uint32_t by_object_id)
{
	return AddEquipItemAction_Org(creature, item, p3, true, true, by_object_id);
}
unsigned char d_ret_code_unequipaction[0x20];
int (*AddUnequipItemAction_Org)(CNWSCreature*, CNWSItem*, uint32_t, uint8_t, uint8_t, int, int, uint32_t) = (int (*)(CNWSCreature*, CNWSItem*, uint32_t, uint8_t, uint8_t, int, int, uint32_t))&d_ret_code_unequipaction;
int AddUnequipItemAction_HookProc(CNWSCreature* creature, CNWSItem* item, uint32_t p3, uint8_t p4, uint8_t p5, int as_normal_action, int to_combat_round, uint32_t by_object_id)
{
	return AddUnequipItemAction_Org(creature, item, p3, p4, p5, true, true, by_object_id);
}

int (*CheckItemAlignmentRestrictions)(CNWSCreature*, CNWSItem*) = (int (*)(CNWSCreature*, CNWSItem*))0x08111374;
int OnCanUseOrEquipItem_CheckAlignment(CNWSCreature* creature, CNWSItem* item)
{
	if (get_local_string(&(item->obj), "EVENT_CANUSE"))
	{
		script_event::RESULT result = script_event::run("inc_ev_canuse", item->obj.obj_id, {creature->obj.obj_id});
		if (result) return result->as_int();
	}
	return CheckItemAlignmentRestrictions(creature, item);
}

void OnSendShieldACFeedBack(CNWSCreature* creature, uint16_t p2, uint32_t* p3, CNWSPlayer* player)
{
}

inline bool will_recv_item_event(CNWSObject* from, CNWSItem* item, uint32_t target_id)
{
	if (item == NULL) return false;
	if (item->it_possessor == target_id) return true;
	CGameObject* target_o = GetGameObject(target_id);
	if (target_o == NULL) return true;
	CNWSObject* target_object = target_o->vtable->AsNWSObject(target_o);
	if (target_object == NULL) return false;
	if (target_o->type != OBJECT_TYPE_CREATURE && target_o->type != OBJECT_TYPE_ITEM)
	{
		if (target_o->type == OBJECT_TYPE_PLACEABLE)
		{
			CNWSPlaceable* plc = target_o->vtable->AsNWSPlaceable(target_o);
			if (plc == NULL || plc->plc_has_inventory == false)
			{
				return false;
			}
		}
		else
		{
			return false;
		}
	}
	if (get_local_string(target_object, "EVENT_RECVITEM"))
	{
		script_event::RESULT result = script_event::run("inc_ev_recvitem", target_id, {item->obj.obj_id, from->obj_id});
		if (result && result->as_int()==0) return false;
	}
	return true;
}
unsigned char d_ret_code_addgiveitem[0x20];
int (*AddGiveItemAction_Org)(CNWSObject*, uint32_t, uint32_t, int) = (int (*)(CNWSObject*, uint32_t, uint32_t, int))&d_ret_code_addgiveitem;
int AddGiveItemAction_HookProc(CNWSObject* object, uint32_t item_id, uint32_t target_id, int p4)
{
	if (!will_recv_item_event(object, GetItemById(item_id), target_id)) return 0;

	return AddGiveItemAction_Org(object, item_id, target_id, p4);
}
unsigned char d_ret_code_addrepmove[0x20];
int (*AddRepositoryMoveAction_Org)(CNWSCreature*, CNWSItem*, uint32_t, uint8_t, uint8_t) = (int (*)(CNWSCreature*, CNWSItem*, uint32_t, uint8_t, uint8_t))&d_ret_code_addrepmove;
int AddRepositoryMoveAction_HookProc(CNWSCreature* creature, CNWSItem* item, uint32_t target_id, uint8_t p4, uint8_t p5)
{
	if (!will_recv_item_event((CNWSObject*)creature, item, target_id)) return 0;

	return AddRepositoryMoveAction_Org(creature, item, target_id, p4, p5);
}

void init()
{
	//custom armor ac
	hook_function(0x081a2d58, (unsigned long)GetItemAC_HookProc, d_ret_code_getitemac, 12);
	hook_function(0x081298b0, (unsigned long)ComputeCreatureAC_HookProc, d_ret_code_computecreatureac, 12);
	enable_write(0x0812993a);
	*(uint32_t*)(0x0812993a) = ((uint32_t)OnCompute_GetTorsoAC-(uint32_t)0x0812993e);
	enable_write(0x081a2de3);
	*(uint32_t*)(0x081a2de3) = ((uint32_t)OnCompute_GetTorsoAC-(uint32_t)0x081a2de7);
	hook_function(0x081a17cc, (unsigned long)LoadItem_HookProc, d_ret_code_loaditem, 9);

	//support for new shields
	hook_call(0x08129cad, (uint32_t)OnComputeCreature_GetShieldItem);
	enable_write(0x08129cc5);
	enable_write(0x08129e3a);
	
	//check that an equipment slot IP dont get applied twice
	hook_function(0x081a68ac, (unsigned long)ApplyItemProperties_HookProc, d_ret_code_applyip, 9);
	hook_function(0x081a2cc0, (unsigned long)RemoveItemProperties_HookProc, d_ret_code_rmip, 12);

	//weapon swap exploit / equip when sitting script
	hook_function(0x08116cec, (unsigned long)AddEquipItemAction_HookProc, d_ret_code_equipaction, 12);
	hook_function(0x081178e4, (unsigned long)AddUnequipItemAction_HookProc, d_ret_code_unequipaction, 12);

	//custom can use/equip item check
	enable_write(0x080ffafa);
	*(uint32_t*)(0x080ffafa) = ((uint32_t)OnCanUseOrEquipItem_CheckAlignment-(uint32_t)0x080ffafe);
	enable_write(0x08110e3e);
	*(uint32_t*)(0x08110e3e) = ((uint32_t)OnCanUseOrEquipItem_CheckAlignment-(uint32_t)0x08110e42);	
	
	//bypass send feedback message about shield ac staking
	enable_write(0x08170232);
	*(uint32_t*)(0x08170232) = ((uint32_t)OnSendShieldACFeedBack-(uint32_t)0x08170236);
	
	hook_function(0x081c7bd8, (long)AddGiveItemAction_HookProc, d_ret_code_addgiveitem, 10);
	hook_function(0x0813bef8, (long)AddRepositoryMoveAction_HookProc, d_ret_code_addrepmove, 10);
}
REGISTER_INIT(init);
	
}
}