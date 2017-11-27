#include "creature.h"
#include "player.h"
#include "nwscript.h"
#include <cmath>

using namespace nwnx::core;
using namespace nwnx::player;
using namespace nwnx::nwscript;

namespace nwnx { namespace creature {

std::map<PERCEPTION_POINT, unsigned char, PERCEPTION_POINT_CMP>::iterator perception_points_iterator;

void CREATURE_EXTRA::DetachAttachedBy()
{
	if (attached_by)
	{
		CNWSCreature* creature_attached_by = GetCreatureById(attached_by->target_id);
		if (creature_attached_by)
		{
			CREATURE_EXTRA* creature_extra_attached_by = GetCreatureExtra(creature_attached_by);
			creature_extra_attached_by->attached_to = OBJECT_INVALID;
		}
		force_players_to_update_object(attached_by->target_id);
		delete attached_by;
		attached_by = NULL;
	}
}
void CREATURE_EXTRA::DetachAttachedTo()
{
	if (attached_to != OBJECT_INVALID)
	{
		CNWSCreature* creature_attached_to = GetCreatureById(attached_to);
		if (creature_attached_to)
		{
			CREATURE_EXTRA* creature_attached_to_extra = GetCreatureExtra(creature_attached_to);
			if (creature_attached_to_extra->attached_by)
			{
				force_players_to_update_object(creature_attached_to_extra->attached_by->target_id);
				delete creature_attached_to_extra->attached_by;
				creature_attached_to_extra->attached_by = NULL;
			}
		}
		attached_to = OBJECT_INVALID;
	}
}
CREATURE_EXTRA::~CREATURE_EXTRA()
{
	DetachAttachedBy();
	DetachAttachedTo();
	if (head_as_item)
	{
		CNWSItem_Destructor(head_as_item, false);
		delete head_as_item;
	}
}

unsigned char d_ret_code_createstats[0x20];
void (*CreateCreatureStats_Org)(CNWSCreatureStats*, CNWSCreature*) = (void (*)(CNWSCreatureStats*, CNWSCreature*))&d_ret_code_createstats;
void CreateCreatureStats_HookProc(CNWSCreatureStats* stats, CNWSCreature* creature)
{
	CREATURE_EXTRA* creature_extra = new CREATURE_EXTRA;
	stats->cs_age = (int)creature_extra;

	return CreateCreatureStats_Org(stats, creature);
}
unsigned char d_ret_code_destroystats[0x20];
void (*DestroyCreatureStats_Org)(CNWSCreatureStats*) = (void (*)(CNWSCreatureStats*))&d_ret_code_destroystats;
void DestroyCreatureStats_HookProc(CNWSCreatureStats* stats)
{
	delete GetCreatureExtra(stats);

	return DestroyCreatureStats_Org(stats);
}

void* GetVisibleListElement_Imp(CNWSCreature* creature, uint32_t element_id)
{
	uint32_t* current_element = (uint32_t*)(creature->cre_percepts.data);
	uint32_t* last_element = current_element+creature->cre_percepts.len;
	while (current_element < last_element)
	{
		if (*(uint32_t*)*current_element == element_id)
		{
			return (void*)*current_element;
		}
		current_element++;
	}
	return 0;
}
unsigned char d_ret_code_clos[0x20];
int (*ClearLineOfSight)(CNWSArea*, Vector, Vector, Vector*, uint32_t*, uint32_t, uint32_t, int) = (int (*)(CNWSArea*, Vector, Vector, Vector*, uint32_t*, uint32_t, uint32_t, int))&d_ret_code_clos;
int ClearLineOfSight_Hook(CNWSArea* area, Vector v1, Vector v2, Vector* v3, uint32_t* d1, uint32_t d2, uint32_t d3, int i1)
{
	if (area->area_tiles)
	{
		return ClearLineOfSight(area, v1, v2, v3, d1, d2, d3, i1);
	}
	else
	{
		fprintf(stderr, "ClearLineOfSight_Hook in an area having an invalid tile:%s!\n", area->area_tag.text);
		return false;
	}
}
unsigned char d_ret_code_upvislist[0x20];
void (*UpdateVisibleList_Org)(CNWSCreature*) = (void (*)(CNWSCreature*))&d_ret_code_upvislist;
void UpdateVisibleList_HookProc(CNWSCreature* creature)
{
	UpdateVisibleList_Org(creature);
	std::map<PERCEPTION_POINT, unsigned char, PERCEPTION_POINT_CMP>& perception_points = GetCreatureExtra(creature)->perception_points;
	for (perception_points_iterator=perception_points.begin() ; perception_points_iterator!=perception_points.end(); perception_points_iterator++)
	{
		if (perception_points_iterator->second & PERCEPTION_POINT_STATE_FLAG_USED)
		{
			perception_points_iterator->second &= ~PERCEPTION_POINT_STATE_FLAG_USED;
		}
		else
		{
			perception_points_iterator = perception_points.erase(perception_points_iterator);
			if (perception_points_iterator == perception_points.end()) break;
		}
	}
}
float hips_min_distance = 4.25;
CNWSCreature* current_perception_source = NULL;
unsigned char d_ret_code_doperception[0x20];
int (*DoPerceptionUpdate_Org)(CNWSCreature*, CNWSCreature*, int) = (int (*)(CNWSCreature*, CNWSCreature*, int))&d_ret_code_doperception;
int DoPerceptionUpdate_HookProc(CNWSCreature* self, CNWSCreature* target, int p3)
{
	current_perception_source = self;

	if (p3 &&
		target->cre_mode_stealth &&
		get_distance_between_points(self->obj.obj_position, target->obj.obj_position) < hips_min_distance)
	{
		*(uint8_t*)0x08125228 = 0xC2;
		int nRet = DoPerceptionUpdate_Org(self, target, p3);
		*(uint8_t*)0x08125228 = 0xC0;
		return nRet;
	}

	/*if (target->cre_mode_stealth)
	{
		uint32_t target_to_ignore = get_local_object((CNWSObject*)self, "IGNORE_NEXT_PERCEPTION_CHECK");
		if (target_to_ignore == target->obj.obj_id)
		{
			DeleteLocalObject(&(self->obj), "IGNORE_NEXT_PERCEPTION_CHECK");
			return 0;
		}
	}*/

	return DoPerceptionUpdate_Org(self, target, p3);
}
int ClearLineOfSight_Imp(CNWSArea* area, Vector source_pos, Vector target_pos, Vector* p4, uint32_t* p5, uint32_t p6, uint32_t p7, int p8)
{
	PERCEPTION_POINT pp;
	pp.x = target_pos.x;
	pp.y = target_pos.y;
	std::map<PERCEPTION_POINT, unsigned char, PERCEPTION_POINT_CMP>& perception_points = GetCreatureExtra(current_perception_source)->perception_points;
	perception_points_iterator = perception_points.find(pp);
	if (perception_points_iterator == perception_points.end() || current_perception_source->cre_combat_state)
	{
		unsigned char los = (ClearLineOfSight(area, source_pos, target_pos, p4, p5, p6, p7, p8) != 0);
		los |= PERCEPTION_POINT_STATE_FLAG_USED;
		perception_points[pp] = los;
		return (los & PERCEPTION_POINT_STATE_FLAG_LOS);
	}
	else
	{
		perception_points_iterator->second |= PERCEPTION_POINT_STATE_FLAG_USED;
		return (perception_points_iterator->second & PERCEPTION_POINT_STATE_FLAG_LOS);
	}
}

int (*ComputeArmorClass)(CNWSItem*) = (int (*)(CNWSItem*))0x81a2d58;
int GetHasAmbidexOrTwoWeaponFeat(CNWSCreatureStats* stats, uint16_t feat_id)
{
	if (has_feat(stats, feat_id)) return true;

	if (has_feat(stats, 374/*dual wield*/))
	{
		CNWSItem* armor = get_item_in_slot(stats->cs_original->cre_equipment, 2/*chest*/);
		if (armor == NULL || ComputeArmorClass(armor) <= 3) return true;
	}

	return false;
}

CNWSItem* GetItemInSlot_HookProc(CNWSInventory* inventory, uint32_t slot_flag)
{
	return get_item_in_slot(inventory, slot_flag);
}

int OnGetIsInUseRange_GetLoS_called;
Vector OnGetIsInUseRange_GetLoS_cv1;
Vector OnGetIsInUseRange_GetLoS_cv2;
int OnGetIsInUseRange_GetLoS(CNWSArea* area, Vector source_pos, Vector target_pos, Vector* p4, uint32_t* p5, uint32_t p6, uint32_t p7, int p8)
{
	OnGetIsInUseRange_GetLoS_called = true;
	OnGetIsInUseRange_GetLoS_cv1.x = source_pos.x;
	OnGetIsInUseRange_GetLoS_cv1.y = source_pos.y;
	OnGetIsInUseRange_GetLoS_cv1.z = source_pos.z;
	OnGetIsInUseRange_GetLoS_cv2.x = target_pos.x;
	OnGetIsInUseRange_GetLoS_cv2.y = target_pos.y;
	OnGetIsInUseRange_GetLoS_cv2.z = target_pos.z;
	return true;
}
int (*GetUseRange)(CNWSCreature*, uint32_t, Vector*, float*) = (int (*)(CNWSCreature*, uint32_t, Vector*, float*))0x0813799c;
unsigned char d_ret_code_isinuserange[0x20];
int (*GetIsInUseRange_Org)(CNWSCreature*, uint32_t, float, int) = (int (*)(CNWSCreature*, uint32_t, float, int))&d_ret_code_isinuserange;
int GetIsInUseRange_HookProc(CNWSCreature* creature, uint32_t object_id, float distance, int p4)
{
	OnGetIsInUseRange_GetLoS_called = false;
	bool is_in_use_range = GetIsInUseRange_Org(creature, object_id, distance, p4);
	if (is_in_use_range && OnGetIsInUseRange_GetLoS_called)
	{
		CNWSArea* area = GetAreaById(creature->obj.obj_area_id);
		if (!area) return false;
		uint32_t los_p5;
		Vector los_p4;
		is_in_use_range = ClearLineOfSight(area, OnGetIsInUseRange_GetLoS_cv1, OnGetIsInUseRange_GetLoS_cv2, &los_p4, &los_p5, creature->obj.obj_id, object_id, p4);
	}
	return is_in_use_range;
}
int GetIsInUseRange_NoLoS(CNWSCreature* creature, uint32_t object_id, float distance, int p4)
{
	return GetIsInUseRange_Org(creature, object_id, distance, p4);
}

int ignore_AdjustSpellUsesPerDay = 0;
struct ScopeIgnoreAdjustSpellUsesPerDay
{
	ScopeIgnoreAdjustSpellUsesPerDay() {ignore_AdjustSpellUsesPerDay++;}
	~ScopeIgnoreAdjustSpellUsesPerDay() {ignore_AdjustSpellUsesPerDay--;}
};

unsigned char d_ret_code_adjustspelluses[0x20];
void (*AdjustSpellUsesPerDay_Org)(CNWSCreatureStats*) = (void (*)(CNWSCreatureStats*))&d_ret_code_adjustspelluses;
void AdjustSpellUsesPerDay_Hook(CNWSCreatureStats* stats)
{
	if (ignore_AdjustSpellUsesPerDay) return;
	return AdjustSpellUsesPerDay_Org(stats);
}

uint8_t nDispelSaveOverride = 0;
uint8_t (*GetClassLevelByIndex)(CNWSCreatureStats*, uint8_t, int) = (uint8_t (*)(CNWSCreatureStats*, uint8_t, int))0x8163e50;
uint8_t GetSpellCastClassLevel_HookProc(CNWSCreatureStats* stats, uint8_t nClassIndex, int p3)
{
	if (nDispelSaveOverride > 0)
	{
		return nDispelSaveOverride;
	}
	else
	{
		return GetClassLevelByIndex(stats, nClassIndex, p3);
	}
}

int nCasterLevelOverride = 0;
int (*VMPushInteger)(CVirtualMachine*, int) = (int (*)(CVirtualMachine*, int))0x826434c;
int OnGetCasterLevel_PushInteger_HookProc(CVirtualMachine* vm, int i)
{
	if (nCasterLevelOverride > 0) i = nCasterLevelOverride;
	return VMPushInteger(vm, i);
}

CNWBaseItem* (*GetBaseItem)(CNWRules*, int) = (CNWBaseItem* (*)(CNWRules*, int))0x080cba5c;
unsigned char d_ret_code_canequip[0x20];
int (*CanEquipItem_Org)(CNWSCreature*, CNWSItem*, uint32_t*, int, int, int, CNWSPlayer*) = (int (*)(CNWSCreature*, CNWSItem*, uint32_t*, int, int, int, CNWSPlayer*))&d_ret_code_canequip;
int CanEquipItem_HookProc(CNWSCreature* creature, CNWSItem* item, uint32_t* p3, int p4, int p5, int p6, CNWSPlayer* player)
{
	int original_baseitem_type = item->it_baseitem;
	CNWBaseItem* original_baseitem = GetBaseItem(nwn_rules, original_baseitem_type);
	if (original_baseitem->bi_base_ac > 0)
	{
		switch (original_baseitem->bi_base_ac)
		{
			case 1:
				item->it_baseitem = 0xe;
				break;
			case 2:
				item->it_baseitem = 0x38;
				break;
			case 3:
				item->it_baseitem = 0x39;
				break;
		}
	}

	uint32_t item_type = item->it_baseitem;
	if (item_type == 20 ||
		item_type == 25 ||
		item_type == 27)
	{
		*(uint8_t*)(0x080ffba6) = 0xEB;
	}
	else
	{
		*(uint8_t*)(0x080ffba6) = 0x75;
	}

	int ret = CanEquipItem_Org(creature, item, p3, p4, p5, p6, player);
	item->it_baseitem = original_baseitem_type;
	return ret;
}

unsigned char d_ret_code_runequip[0x20];
int (*RunEquip_Org)(CNWSCreature*, uint32_t, uint32_t, uint32_t) = (int (*)(CNWSCreature*, uint32_t, uint32_t, uint32_t))&d_ret_code_runequip;
int RunEquip_HookProc(CNWSCreature* creature, uint32_t d1, uint32_t d2, uint32_t d3)
{
	CREATURE_EXTRA* creature_extra = GetCreatureExtra(creature);
	if (creature_extra->bIsEquipingItem) return 0;
	creature_extra->bIsEquipingItem = true;
	int nRet = RunEquip_Org(creature, d1, d2, d3);
	creature_extra->bIsEquipingItem = false;
	return nRet;
}

CNWSItem* GetNotHiddenHelmetInSlot_Root(CNWSCreature* creature, uint32_t slot)
{
	CNWSInventory* inventory = creature->cre_equipment;
	CNWSItem* helmet = get_item_in_slot(inventory, slot);
	if (helmet != NULL)
	{
		CNWSCreature* owner = GetCreatureById(helmet->it_possessor);
		if (owner)
		{
			if (get_local_int((CNWSObject*)owner, "HIDE_HELMET") ) helmet = NULL;
		}
	}
	if (helmet == NULL)
	{
		helmet = GetCreatureExtra(creature)->head_as_item;
	}
	return helmet;
}
CNWSItem* GetNotHiddenHelmetInSlot_CheckSlot(CNWSCreature* creature, uint32_t slot)
{
	switch (slot)
	{
		case 0x1: return GetNotHiddenHelmetInSlot_Root(creature, slot);
		default: return get_item_in_slot(creature->cre_equipment, slot);
	}
}

unsigned char d_ret_code_exacredata[0x20];
int (*SendExamineCreatureData_Org)(CNWSMessage*, CNWSPlayer*, uint32_t) = (int (*)(CNWSMessage*, CNWSPlayer*, uint32_t))&d_ret_code_exacredata;
int SendExamineCreatureData_Hook(CNWSMessage* message, CNWSPlayer* player, uint32_t creature_id)
{
	CNWSCreature* creature = GetCreatureById(creature_id);
	if (creature && creature->cre_stats->cs_is_pc)
	{
		*(uint8_t*)0x08073d1c = 0xEB;
	}
	int ret = SendExamineCreatureData_Org(message, player, creature_id);
	*(uint8_t*)0x08073d1c = 0x75;
	return ret;
}

void init()
{
	hook_function(0x0813fbb4, (unsigned long)CreateCreatureStats_HookProc, d_ret_code_createstats, 12);
	hook_function(0x0814035c, (unsigned long)DestroyCreatureStats_HookProc, d_ret_code_destroystats, 12);
	//do not set the creature age... the field is reserved for a pointer to extra data
	enable_write(0x0813fd2d);
	for (int i=0; i<7; i++) *(uint8_t*)(0x0813fd2d+i) = 0x90;
	enable_write(0x0814c83b);
	for (int i=0; i<3; i++) *(uint8_t*)(0x0814c83b+i) = 0x90;
	
	//fix a crash when doing a LoS in an area with an invalid tile
	hook_function(0x080d31f0, (long)ClearLineOfSight_Hook, d_ret_code_clos, 12);

    //On do perception, ignore hips
	enable_write(0x08125228);
	hook_function(0x8125164, (unsigned long)DoPerceptionUpdate_HookProc, d_ret_code_doperception, 12);
	hook_call(0x08125411, (uint32_t)ClearLineOfSight_Imp);

	hook_function(0x81131fc, (unsigned long)GetVisibleListElement_Imp, d_ret_code_nouse, 12);

	hook_function(0x08108500, (unsigned long)UpdateVisibleList_HookProc, d_ret_code_upvislist, 12);	

	hook_function(0x0819ec9c, (unsigned long)GetItemInSlot_HookProc, d_ret_code_nouse, 10);

	hook_function(0x08137730, (unsigned long)GetIsInUseRange_HookProc, d_ret_code_isinuserange, 12);	

	//improve performance
	//check distance before LoS
	hook_call(0x08137921, (long)OnGetIsInUseRange_GetLoS);
	//no need to call LoS for actionSit
	hook_call(0x0810bea8, (long)GetIsInUseRange_NoLoS);	
	
	//consider dual wield on get has ambidex or 2 weapon fighting feat
	hook_call(0x08143505, (long)GetHasAmbidexOrTwoWeaponFeat);
	hook_call(0x081434ed, (long)GetHasAmbidexOrTwoWeaponFeat);
	hook_call(0x08144a5d, (long)GetHasAmbidexOrTwoWeaponFeat);
	hook_call(0x08144a46, (long)GetHasAmbidexOrTwoWeaponFeat);
	
	//improve performance
	hook_call(0x08125543, (long)GetVisibleListElement_Imp);
	hook_call(0x081257db, (long)GetVisibleListElement_Imp);
	hook_call(0x0812593d, (long)GetVisibleListElement_Imp);
	hook_call(0x0806352d, (long)GetVisibleListElement_Imp);
	hook_call(0x08108743, (long)GetVisibleListElement_Imp);
	
	//optionally do not update the spells count
	hook_function(0x08160088, (unsigned long)AdjustSpellUsesPerDay_Hook, d_ret_code_adjustspelluses, 12);

	//custom caster level
	//dispel dc
	hook_call(0x0817ef9b, (long)GetSpellCastClassLevel_HookProc);
	hook_call(0x0820afeb, (long)GetSpellCastClassLevel_HookProc);
	//nwscript function
    hook_call(0x08205e30, (long)OnGetCasterLevel_PushInteger_HookProc);

	//GetHelmetInSlot... to hide helm
	enable_write(0x0812f056);
	*(uint8_t*)0x0812f056 = 0x50; //push creature instead of equipment
	for (uint32_t addr=0x0812f057; addr<0x0812f05c; addr++) *(uint8_t*)addr = 0x90;
	hook_call(0x0812f05c, (long)GetNotHiddenHelmetInSlot_Root);
	enable_write(0x0806a666);
	*(uint8_t*)0x0806a666 = 0x50; //push creature instead of equipment
	for (uint32_t addr=0x0806a667; addr<0x0806a66c; addr++) *(uint8_t*)addr = 0x90;
	hook_call(0x0806a66c, (long)GetNotHiddenHelmetInSlot_Root);
	enable_write(0x0806a82a);
	*(uint8_t*)0x0806a82a = 0x53; //push creature instead of equipment
	for (uint32_t addr=0x0806a82b; addr<0x0806a830; addr++) *(uint8_t*)addr = 0x90;
	hook_call(0x0806a830, (long)GetNotHiddenHelmetInSlot_Root);
	enable_write(0x0806aa67);
	*(uint8_t*)0x0806aa67 = 0x50; //push creature instead of equipment
	for (uint32_t addr=0x0806aa68; addr<0x0806aa6d; addr++) *(uint8_t*)addr = 0x90;
	hook_call(0x0806aa6d, (long)GetNotHiddenHelmetInSlot_Root);
	enable_write(0x0806ad91);
	*(uint8_t*)0x0806ad91 = 0x57; //push creature instead of equipment
	for (uint32_t addr=0x0806ad92; addr<0x0806ad97; addr++) *(uint8_t*)addr = 0x90;
	hook_call(0x0806ad97, (long)GetNotHiddenHelmetInSlot_Root);
	enable_write(0x0806b02a);
	*(uint32_t*)0x0806b02a = 0xd389; //push creature instead of equipment
	for (uint32_t addr=0x0806b02c; addr<0x0806b030; addr++) *(uint8_t*)addr = 0x90;
	hook_call(0x0806b03e, (long)GetNotHiddenHelmetInSlot_CheckSlot);

	hook_function(0x08116f20, (unsigned long)RunEquip_HookProc, d_ret_code_runequip, 12);
	enable_write(0x080ffba6); //allow equip ammunition while polymorphed
	hook_function(0x080ff978, (unsigned long)CanEquipItem_HookProc, d_ret_code_canequip, 12);

	//NPCs examine challenge rating = level
	enable_write(0x08073d3f);
	*(uint16_t*)0x08073d3f = 0x9090;
	enable_write(0x08073d1c);
	hook_function(0x08073958, (long)SendExamineCreatureData_Hook, d_ret_code_exacredata, 12);	
}
REGISTER_INIT(init);

void (*RemoveItemProperties)(CNWSItem*, CNWSCreature*, uint32_t) = (void (*)(CNWSItem*, CNWSCreature*, uint32_t))0x081a2cc0;
void (*ApplyItemProperties)(CNWSItem*, CNWSCreature*, uint32_t, int) = (void (*)(CNWSItem*, CNWSCreature*, uint32_t, int))0x081a68ac;
VM_FUNC_NEW(ReapplyItemPropertiesOfItemInSlot, 230)
{
	CNWSCreature* creature = GetCreatureById(vm_pop_object());
	int slot = vm_pop_int();
	if (creature)
	{
		slot = (1<<slot);
		CNWSItem* item = get_item_in_slot(creature->cre_equipment, slot);
		if (item)
		{
			RemoveItemProperties(item, creature, slot);
			ApplyItemProperties(item, creature, slot, 0);
		}
	}
}
inline void update_item_id_in_repository(CItemRepository* repos, uint32_t org_item_id, uint32_t new_item_id)
{
	if (repos)
	{
		CExoLinkedListNode* item_node = repos->ir_list.header->first;
		while (item_node)
		{
			if (*(uint32_t*)item_node->data == org_item_id)
			{
				*(uint32_t*)item_node->data = new_item_id;
			}
			item_node = item_node->next;
		}
	}
	else
	{
		fprintf(stderr, "updating item appearance in object with invalid item repository!\n");
	}
}
int (*AddObjectToAI)(CServerAIMaster*, CNWSObject*, int) = (int (*)(CServerAIMaster*, CNWSObject*, int))0x080980e0;
int (*RemoveObjectFromAI)(CServerAIMaster*, CNWSObject*) = (int (*)(CServerAIMaster*, CNWSObject*))0x080962a8;
int (*UpdateQuickbarButton)(CNWSMessage* message, CNWSPlayer*, uint8_t, int) = (int (*)(CNWSMessage* message, CNWSPlayer*, uint8_t, int))0x807c794;
VM_FUNC_NEW(UpdateItemAppearance, 295)
{
	uint32_t org_item_id = vm_pop_object();
	CGameObject* o = GetGameObject(org_item_id);
	if (o)
	{
		CNWSItem* item = o->vtable->AsNWSItem(o);
		if (item)
		{
			ScopeIgnoreAdjustSpellUsesPerDay scope_ignore_adjust_spell_use_per_day;
			int org_ai_level = item->obj.obj_ai_level;
			uint32_t org_possessor_id = item->it_possessor;
			uint32_t org_area = item->obj.obj_area_id;
			//remove item properties while we can!
			CGameObject* possessor = GetGameObject(org_possessor_id);
			CNWSCreature* creature = NULL;
			if (possessor && possessor->type == OBJECT_TYPE_CREATURE)
			{
				creature = possessor->vtable->AsNWSCreature(possessor);
				if (creature && creature->cre_equipment)
				{
					for (int i=0; i<18; i++)
					{
						if (creature->cre_equipment->inv_items[i] == org_item_id)
						{
							RemoveItemProperties(item, creature, 1<<i);
						}
					}
				}
			}
			if (org_ai_level >= 0)
			{
				RemoveObjectFromAI(server_internal->srv_ai, &(item->obj));
			}
			CGOA_Delete(NULL, org_item_id);
			uint32_t new_item_id;
			CGOA_AddInternalObject(NULL, &new_item_id, o, false);
			o->id = new_item_id;
			item->obj.obj_id = new_item_id;
			if (org_ai_level >= 0)
			{
				AddObjectToAI(server_internal->srv_ai, &(item->obj), 0);
			}
			if (possessor)
			{
				if (creature)
				{
					if (creature->cre_equipment)
					{
						for (int i=0; i<18; i++)
						{
							if (creature->cre_equipment->inv_items[i] == org_item_id)
							{
								creature->cre_equipment->inv_items[i] = new_item_id;
								ApplyItemProperties(item, creature, 1<<i, 0);
							}
						}
					}
					else
					{
						fprintf(stderr, "updating an item owned by a creature that has an invalid equipment!\n");
					}
					if (creature->cre_quickbar)
					{
						for (int i=0; i<36; i++)
						{
							bool quickbar_updated = false;
							if (creature->cre_quickbar[i].qb_objid1 == org_item_id)
							{
								creature->cre_quickbar[i].qb_objid1 = new_item_id;
								quickbar_updated = true;
							}
							if (creature->cre_quickbar[i].qb_objid2 == org_item_id)
							{
								creature->cre_quickbar[i].qb_objid2 = new_item_id;
								quickbar_updated = true;
							}
							if (quickbar_updated)
							{
								CNWSPlayer* player = get_player_by_game_object_id(org_possessor_id);
								if (player)
								{
									UpdateQuickbarButton(server_internal->srv_client_messages, player, i, 0);
								}
							}
						}
					}
					update_item_id_in_repository(creature->cre_inventory, org_item_id, new_item_id);
				}
				else if (possessor->type == OBJECT_TYPE_PLACEABLE)
				{
					CNWSPlaceable* placeable = possessor->vtable->AsNWSPlaceable(possessor);
					if (placeable)
					{
						update_item_id_in_repository(placeable->plc_inventory, org_item_id, new_item_id);
					}
				}
				else if (possessor->type == OBJECT_TYPE_ITEM)
				{
					CNWSItem* possessor_item = possessor->vtable->AsNWSItem(possessor);
					if (possessor_item)
					{
						update_item_id_in_repository(possessor_item->it_inventory, org_item_id, new_item_id);
					}
				}
				else if (possessor->type == OBJECT_TYPE_STORE)
				{
					CNWSStore* store = possessor->vtable->AsNWSStore(possessor);
					if (store)
					{
						update_item_id_in_repository(store->st_page_1, org_item_id, new_item_id);
						update_item_id_in_repository(store->st_page_2, org_item_id, new_item_id);
						update_item_id_in_repository(store->st_page_3, org_item_id, new_item_id);
						update_item_id_in_repository(store->st_page_4, org_item_id, new_item_id);
						update_item_id_in_repository(store->st_page_5, org_item_id, new_item_id);
					}
				}
				else
				{
					fprintf(stderr, "unsupported item possessor type:%d\n", possessor->type);
				}
			}
			CNWSCreature* barter_owner = GetCreatureById(item->it_barterowner);
			if (barter_owner && barter_owner->cre_barter)
			{
				CExoLinkedListNode* barter_items_node = barter_owner->cre_barter->items_repos->ir_list.header->first;
				while (barter_items_node)
				{
					if (*(uint32_t*)barter_items_node->data == org_item_id)
					{
						*(uint32_t*)barter_items_node->data = new_item_id;
					}
					barter_items_node = barter_items_node->next;
				}
			}
			CNWSArea* area = GetAreaById(org_area);
			if (area)
			{
				uint32_t* area_objects_id = (uint32_t*)area->are_objects.data;
				uint32_t* area_objects_id_list_end = area_objects_id+area->are_objects.len;
				while (area_objects_id < area_objects_id_list_end)
				{
					if (*area_objects_id == org_item_id)
					{
						*area_objects_id = new_item_id;
					}
					area_objects_id++;
				}
			}
			org_item_id = new_item_id;
		}
	}
	vm_push_object(org_item_id);
}

VM_FUNC_NEW(SetCasterLevelOverride, 183)
{
	nCasterLevelOverride = vm_pop_int();
}
VM_FUNC_NEW(SetDispelSaveOverride, 184)
{
	nDispelSaveOverride = vm_pop_int();
}

void (*CNWSItem_Constructor)(CNWSItem*, uint32_t) = (void (*)(CNWSItem*, uint32_t))0x0819eda0;
VM_FUNC_NEW(SetHeadItemOverrideString, 231)
{
	CNWSCreature* creature = GetCreatureById(vm_pop_object());
	std::string head_item_appr = vm_pop_string();
	if (creature)
	{
		CREATURE_EXTRA* creature_extra = GetCreatureExtra(creature);
		if (creature_extra->head_as_item)
		{
			CNWSItem_Destructor(creature_extra->head_as_item, false);
			delete creature_extra->head_as_item;
			creature_extra->head_as_item = NULL;
		}
		uint32_t base_item, appearance_index, cloth1_color, cloth2_color, leather1_color, leather2_color, metal1_color, metal2_color;
		if (sscanf(head_item_appr.c_str(), "%d#%d#%d#%d#%d#%d#%d#%d",
			&base_item, &appearance_index, &cloth1_color, &cloth2_color, &leather1_color, &leather2_color, &metal1_color, &metal2_color)==8 && base_item<nwn_rules->ru_baseitems->size)
		{
			CNWSItem* head_item = new CNWSItem;
			CNWSItem_Constructor(head_item, OBJECT_INVALID);
			head_item->it_baseitem = base_item;
			head_item->it_model[0] = appearance_index;
			head_item->it_color[0] = cloth1_color;
			head_item->it_color[1] = cloth2_color;
			head_item->it_color[2] = leather1_color;
			head_item->it_color[3] = leather2_color;
			head_item->it_color[4] = metal1_color;
			head_item->it_color[5] = metal2_color;
			creature_extra->head_as_item = head_item;

			set_local_string(&creature->obj.obj_vartable, "X_HEAD_ITEM", head_item_appr);
		}
		else
		{
			delete_local_string(&creature->obj.obj_vartable, "X_HEAD_ITEM");
		}
	}
}
VM_FUNC_NEW(GetHeadItemOverrideString, 239)
{
	CNWSCreature* creature = GetCreatureById(vm_pop_object());
	CExoString result(creature ? get_local_string(&creature->obj.obj_vartable, "X_HEAD_ITEM") : "");
	vm_push_string(&result);
}
	
}
}
