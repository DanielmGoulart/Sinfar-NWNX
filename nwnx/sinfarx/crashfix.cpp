#include "core.h"
#include "cstmfac.h"
#include "nwscript.h"

using namespace nwnx;
using namespace nwnx::core;
using namespace nwnx::nwscript;

namespace
{


void (*CNWSItem_Constructor)(CNWSItem*, uint32_t) = (void (*)(CNWSItem*, uint32_t))0x0819eda0;
CNWSItem dummy_invalid_item;

unsigned char d_ret_code_lastused[0x20];
int (*GetLastUsedBy_Org)(CNWVirtualMachineCommands*, int, int) = (int (*)(CNWVirtualMachineCommands*, int, int))&d_ret_code_lastused;
int GetLastUsedBy_HookProc(CNWVirtualMachineCommands* vm_cmd, int p2, int p3)
{
	CGameObject* o = GetGameObject(vm_cmd->cmd_self_id);
	if (o==NULL || o->type != OBJECT_TYPE_PLACEABLE) return 0xfffffd7e;
	return GetLastUsedBy_Org(vm_cmd, p2, p3);
}

unsigned char d_ret_code_randomwalk[0x20];
int (*RandomWalk_Org)(CNWSCreature*, void*) = (int (*)(CNWSCreature*, void*))&d_ret_code_randomwalk;
int RandomWalk_HookProc(CNWSCreature* creature, void* action)
{
	CNWSArea* area = GetAreaById(creature->obj.obj_area_id);
	if (area != NULL)
	{
		area->are_cre_pathfinding = creature->cre_pathfinding;
	}
	return RandomWalk_Org(creature, action);
}

unsigned char d_ret_code_movepltoarea[0x20];
int (*MovePlayerToArea_Org)(CServerExoAppInternal*, CNWSPlayer*) = (int (*)(CServerExoAppInternal*, CNWSPlayer*))&d_ret_code_movepltoarea;
int MovePlayerToArea_HookProc(CServerExoAppInternal* servinternal, CNWSPlayer* player)
{
	CNWSCreature* creature = GetCreatureById(player->pl_oid);
	if (creature)
	{
		if (GetAreaById(creature->obj.obj_area_id) || GetAreaById(creature->cre_desired_area))
		{
			return MovePlayerToArea_Org(servinternal, player);
		}
	}
	fprintf(stderr, "crash fix: MovePlayerToArea\n");
	return 0;
}

unsigned char d_ret_code_calcspellmiss[0x20];
CNWSObject* (*CalculateSpellRangedMissTarget_Org)(CNWSObject*, CNWSObject*, uint32_t) = (CNWSObject* (*)(CNWSObject*, CNWSObject*, uint32_t))&d_ret_code_calcspellmiss;
CNWSObject* CalculateSpellRangedMissTarget_Hook(CNWSObject* object, CNWSObject* p2, uint32_t p3)
{
	if (GetGameObject(p2->obj_area_id)==NULL)
	{
		fprintf(stderr, "crash fix:CalculateSpellRangedMissTarget\n");
		return object;
	}
	return CalculateSpellRangedMissTarget_Org(object, p2, p3);
}

CNWBaseItem* GetBaseItem_HookProc(CNWBaseItemArray* bi_array, uint32_t base_item)
{
	if (base_item < bi_array->size)
	{
		return &(bi_array->base_items[base_item]);
	}
	else
	{
		return &(bi_array->base_items[67]);
	}
}

unsigned char d_ret_code_savecontaineritems[0x20];
int (*SaveContainerItems_Org)(CNWSItem*, CResGFF* , CResStruct*) = (int (*)(CNWSItem*, CResGFF* , CResStruct*))&d_ret_code_savecontaineritems;
int SaveContainerItems_Hook(CNWSItem* item, CResGFF* res_gff, CResStruct* res_struct)
{
	if (item->it_inventory == NULL)
	{
		fprintf(stderr, "crash fix: SaveContainerItems_Org\n");
		return 0;
	}
	return SaveContainerItems_Org(item, res_gff, res_struct);
}
CNWSItem* (*GetItemListItem)(CItemRepository*, CExoLinkedListNode*) = (CNWSItem* (*)(CItemRepository*, CExoLinkedListNode*))0x81a6a58;
CNWSItem* OnSaveContainerItems_GetItem(CItemRepository* item_repos, CExoLinkedListNode* node)
{
	CNWSItem* result = GetItemListItem(item_repos, node);
	if (result == NULL)
	{
		fprintf(stderr, "crash fix: OnSaveContainerItems_GetItem\n");
		result = &dummy_invalid_item;
	}
	return result;
}
void* (*CExoLinkedList_RemoveNode)(CExoLinkedList*, CExoLinkedListNode*) = (void* (*)(CExoLinkedList*, CExoLinkedListNode*))0x8083080;
CNWSItem* OnUpdateBarterItems_GetItemAtPos(CItemRepository* items_repos, CExoLinkedListNode* node)
{
	CNWSItem* item = GetItemListItem(items_repos, node);
	if (item == NULL)
	{
		fprintf(stderr, "OnUpdateBarterItems_GetItemAtPos: Invalid Item Removed\n");
		CExoLinkedList_RemoveNode(&items_repos->ir_list, node);
	}
	return item;
}

unsigned char d_ret_code_geteffinteger[0x20];
int GetEffectInteger(CGameEffect* effect, uint32_t i)
{
	if (effect->eff_integers && i < effect->eff_num_integers)
	{
		return effect->eff_integers[i];
	}
	else
	{
		return -1;
	}
}

int OnHandlePlayerToServerDialog_GetTargetObject(void* array, uint32_t object_id, CGameObject** p_o)
{
	int ret = CGOA_GetObject(array, object_id, p_o);
	if (ret == CGOA_RET_SUCCESS)
	{
		if ((*p_o)->vtable->AsNWSObject(*p_o) == NULL)
		{
			fprintf(stderr, "crash fix: OnHandlePlayerToServerDialog_GetTargetObject\n");
			ret = CGOA_RET_BAD_ID;
			*p_o = NULL;
		}
	}
	return ret;
}

int OnGetTotalEffectBonus_GetItemInSlot_invalid_item_buffer[4] = {0,0,0,0};
CNWSItem* OnGetTotalEffectBonus_GetItemInSlot(CNWSInventory* inventory, uint32_t slot)
{
	CNWSItem* item = get_item_in_slot(inventory, slot);
	if (item == NULL)
	{
		fprintf(stderr, "crash fix: OnGetTotalEffectBonus_GetItemInSlot\n");
		item = (CNWSItem*)&OnGetTotalEffectBonus_GetItemInSlot_invalid_item_buffer;
	}
	return item;
}

int (*VMPopInteger)(CVirtualMachine*, int*) = (int (*)(CVirtualMachine*, int*))0x82629fc;
int last_OnGetNearestObject_PopObjectType_val = 0;
int OnGetNearestObject_PopObjectType(CVirtualMachine* vm, int* val)
{
	int ret = VMPopInteger(vm, val);
	last_OnGetNearestObject_PopObjectType_val = *val;
	return ret;
}
int OnGetNearestObject_GetGameObjectInArea(void* array, uint32_t object_id, CGameObject** p_o)
{
	int ret = CGOA_GetObject(array, object_id, p_o);
	if (ret == CGOA_RET_SUCCESS && (*p_o)->type == OBJECT_TYPE_SOUND && last_OnGetNearestObject_PopObjectType_val & 0x0200)
	{
		*(uint16_t*)0x08201eea = 0x9090;
	}
	else
	{
		*(uint16_t*)0x08201eea = 0x0575;
	}
	return ret;
}

CNWSItem* (*GetCurrentAttackWeapon)(CNWSCombatRound*, int) = (CNWSItem* (*)(CNWSCombatRound*, int))0x80e3778;
CNWSItem* OnResolveRangedAnimation_GetAttackWeapon(CNWSCombatRound* combat_round, int p2)
{
	CNWSItem* result = GetCurrentAttackWeapon(combat_round, p2);
	if (result)
	{
		*(uint16_t*)0x080e8299 = 0x850f;
	}
	else
	{
		*(uint16_t*)0x080e8299 = 0xe990;
	}
	return result;
}

unsigned char d_ret_code_rmdisapp[0x20];
int (*OnRemoveDisappearAppear_Org)(void*, CNWSObject*, CGameEffect*) = (int (*)(void*, CNWSObject*, CGameEffect*))&d_ret_code_rmdisapp;
int OnRemoveDisappearAppear_Hook(void* list, CNWSObject* object, CGameEffect* effect)
{
	CNWSArea* area = GetAreaById(effect->eff_objects[0]);
	if (area)
	{
		return OnRemoveDisappearAppear_Org(list, object, effect);
	}
	else
	{
		return 1;
	}
}

CNWSFaction* (*GetFactionById)(CFactionManager*, int) = (CNWSFaction* (*)(CFactionManager*, int))0x80ba9a0;
CNWSFaction* GetCreatureFaction(CNWSCreature* creature)
{
	CNWSFaction* faction = GetFactionById(server_internal->srv_factions, creature->cre_stats->cs_faction_id);
	if (faction)
	{
		return faction;
	}
	else
	{
		//fprintf(stderr, "creature is in no faction:%x\n", creature->obj.obj_id);
		return GetFactionById(server_internal->srv_factions, 0);
	}
}

CNWSCreature* OnBashPlc_GetCreatureById(CServerExoApp* me, uint32_t creature_id)
{
	CNWSCreature* creature = GetCreatureById(creature_id);
	if (creature)
	{
		*(uint8_t*)0x081df9d5 = 0x89;
		*(uint32_t*)0x081df9d6 = 0x10c483c3;
	}
	else
	{
		fprintf(stderr, "crash fix: OnBashPlc_GetCreatureById\n");
		*(uint8_t*)0x081df9d5 = 0xe9;
		*(uint32_t*)0x081df9d6 = 0x081dfbd8 - 0x081df9da;
	}
	return creature;
}

int OnUseObject_GetTargetGameObject(void* array, uint32_t object_id, CGameObject** p_o)
{
	int ret = CGOA_GetObject(array, object_id, p_o);
	if (ret == CGOA_RET_SUCCESS)
	{
		CGameObject* o = *p_o;
		if (o->vtable->AsNWSObject(o) == NULL)
		{
			fprintf(stderr, "crash fix: OnUseObject_GetTargetGameObject\n");
			*p_o = NULL;
			return CGOA_RET_BAD_ID;
		}
	}
	return ret;
}
int OnGetObjectInShape_GetAreaGameObject(void* array, uint32_t object_id, CGameObject** p_o)
{
	int ret = CGOA_GetObject(array, object_id, p_o);
	if (ret == CGOA_RET_SUCCESS)
	{
		CGameObject* o = *p_o;
		if (o->vtable->AsNWSArea(o) == NULL)
		{
			fprintf(stderr, "crash fix: OnGetObjectInShape_GetAreaGameObject\n");
			*p_o = NULL;
			return CGOA_RET_BAD_ID;
		}
	}
	return ret;
}

CGameObject* on_copy_object_last_caller_go = NULL;
CGameObject** on_copy_object_last_area_go = NULL;
CGameObject* OnCopyObject_GetCallerGameObject(CServerExoApp* server, uint32_t object_id)
{
	on_copy_object_last_caller_go = GetGameObject(object_id);
	return on_copy_object_last_caller_go;
}
int OnCopyObject_GetCallerAreaGameObject(void* array, uint32_t object_id, CGameObject** p_o)
{
	int ret = CGOA_GetObject(array, object_id, p_o);
	on_copy_object_last_area_go = p_o;
	return ret;
}
inline void ValidateCopyObjectArea()
{
	CNWSCreature* caller_creature = on_copy_object_last_caller_go->vtable->AsNWSCreature(on_copy_object_last_caller_go);
	if (caller_creature && caller_creature->cre_desired_area != OBJECT_INVALID)
	{
		CGOA_GetObject(NULL, caller_creature->cre_desired_area, on_copy_object_last_area_go);
	}
	if (*on_copy_object_last_area_go)
	{
		fprintf(stderr, "using the caller desired area\n");
	}
	else
	{
		CGOA_GetObject(NULL, get_module()->mod_areas.data[0], on_copy_object_last_area_go);
		fprintf(stderr, "using the first area in the module\n");
	}
}
void (*SetOrientation)(CNWSObject*, Vector) = (void (*)(CNWSObject*, Vector))0x81d4e10;
void OnCopyObject_SetCreatureOrientation(CNWSObject* object, Vector orientation)
{
	SetOrientation(object, orientation);
	if (*on_copy_object_last_area_go == NULL)
	{
		fprintf(stderr, "crash fix: create creature at invalid location and caller\n");
		ValidateCopyObjectArea();
		CNWSArea* area = (*on_copy_object_last_area_go)->vtable->AsNWSArea(*on_copy_object_last_area_go);
		fprintf(stderr, "creating creture in:%s\n", area->area_tag.text);
	}
}
int (*SetPosition)(CNWSObject*, Vector, int) = (int (*)(CNWSObject*, Vector, int))0x081d4e30;
void OnCopyObject_SetDropPosition(CNWSObject* object, Vector position, int p3)
{
	SetPosition(object, position, p3);
	if (*on_copy_object_last_area_go == NULL)
	{
		fprintf(stderr, "crash fix: copy object in inventory full of creature in invalid area\n");
		ValidateCopyObjectArea();
		CNWSArea* area = (*on_copy_object_last_area_go)->vtable->AsNWSArea(*on_copy_object_last_area_go);
		fprintf(stderr, "dropping in:%s at x:%f,y:%f,z:%f\n", area->area_tag.text, position.x, position.y, position.z);
	}
}

int (*GetItemInRepository)(CItemRepository*, CNWSItem*, int) = (int (*)(CItemRepository*, CNWSItem*, int))0x081a4744;
int OnLearnScroll_GetItemInRepository(CItemRepository* repos, CNWSItem* item, int p3)
{
    if (repos == NULL)
    {
        fprintf(stderr, "crash fix: OnLearnScroll_GetItemInRepository\n");
        return false;
    }
    return GetItemInRepository(repos, item, p3);
}

int OnGetObjectInShape_GetGameObjectInArea(void* array, uint32_t object_id, CGameObject** p_o)
{
	uint8_t ret = CGOA_GetObject(array, object_id, p_o);
	if (ret == 0)
	{
		*(uint16_t*)0x08208a9a = 0x2475;
	}
	else
	{
		*(uint16_t*)0x08208a9a = 0x9090;
	}
	return ret;
}

int (*AddItemToRepository)(CItemRepository*, CNWSItem**, uint8_t, uint8_t, int, int) = (int (*)(CItemRepository*, CNWSItem**, uint8_t, uint8_t, int, int))0x81a32d0;
int OnAcquireItem_AddItem(CItemRepository* repos, CNWSItem** p_item, uint8_t p3, uint8_t p4, int p5, int p6)
{
	if (repos == NULL)
	{
		fprintf(stderr, "crash fix: OnAcquireItem_AddItem\n");
		return 0;
	}
	else
	{
		return AddItemToRepository(repos, p_item, p3, p4, p5, p6);
	}
}

void OnWriteNumMemorizedSpells(CNWMessage* message, uint16_t num_memorized_spells, int p3)
{
	if (num_memorized_spells > 255)
	{
		*(uint32_t*)0x0806e3bf = 0x90fff883;
		*(uint16_t*)(0x0806e3bf+4) = 0x9090;
		num_memorized_spells = 255;
	}
	else
	{
		*(uint32_t*)0x0806e3bf = 0x04b8873b;
		*(uint16_t*)(0x0806e3bf+4) = 0x0000;
	}
	WriteWord(message, num_memorized_spells, p3);
}

unsigned char d_ret_code_addcretoarea[0x20];
int (*AddObjectToAI)(CServerAIMaster*, CNWSObject*, int) = (int (*)(CServerAIMaster*, CNWSObject*, int))0x080980e0;
int (*RemoveObjectFromAI)(CServerAIMaster*, CNWSObject*) = (int (*)(CServerAIMaster*, CNWSObject*))0x080962a8;
int (*AddCreatureToArea_Org)(CNWSCreature*, CNWSArea*, float, float, float, int) = (int (*)(CNWSCreature*, CNWSArea*, float, float, float, int))&d_ret_code_addcretoarea;
int AddCreatureToArea_HookProc(CNWSCreature* creature, CNWSArea* area, float x, float y, float z, int p6)
{
	if (area == NULL) return 0;

	if (x >= area->area_width*10) x = area->area_width*10-0.01;
	if (y >= area->area_height*10) y = area->area_height*10-0.01;
	if (x < 0) x = 0;
	if (y < 0) y = 0;

	cstmfac::load_creature_cstmfac(creature);

	if (get_local_int(&(area->area_vartable), "MSDS_SUSPEND_TIME"))
	{
		if (get_player_by_game_object_id(creature->obj.obj_id)==NULL)
		{
			for (int inv_slot=0; inv_slot<18; inv_slot++)
			{
				CNWSItem* item = GetItemById(creature->cre_equipment->inv_items[inv_slot]);
				if (item)
				{
					RemoveObjectFromAI(server_internal->srv_ai, &(item->obj));
				}
			}
			RemoveObjectFromAI(server_internal->srv_ai, (CNWSObject*)creature);
		}
	}
	else if (creature->obj.obj_ai_level == -1)
	{
		AddObjectToAI(server_internal->srv_ai, (CNWSObject*)creature, 2);
	}

	return AddCreatureToArea_Org(creature, area, x, y, z, p6);
}
unsigned char d_ret_code_addplctoarea[0x20];
int (*AddPlaceableToArea_Org)(CNWSPlaceable*, CNWSArea*, float, float, float, int) = (int (*)(CNWSPlaceable*, CNWSArea*, float, float, float, int))&d_ret_code_addplctoarea;
int AddPlaceableToArea_HookProc(CNWSPlaceable* placeable, CNWSArea* area, float x, float y, float z, int p6)
{
	if (area == NULL || area->area_tiles == NULL) return 0;

	if (get_local_int(&(area->area_vartable), "MSDS_SUSPEND_TIME"))
	{
		RemoveObjectFromAI(server_internal->srv_ai, &(placeable->obj));
	}
	else if (placeable->obj.obj_ai_level == -1)
	{
		AddObjectToAI(server_internal->srv_ai, &(placeable->obj), (placeable->plc_static != 0));
	}

	return AddPlaceableToArea_Org(placeable, area, x, y, z, p6);
}
unsigned char d_ret_code_addaoetoarea[0x20];
int (*AddAOEToArea_Org)(CNWSAreaOfEffectObject*, CNWSArea*, float, float, float, int) = (int (*)(CNWSAreaOfEffectObject*, CNWSArea*, float, float, float, int))&d_ret_code_addaoetoarea;
int AddAOEToArea_HookProc(CNWSAreaOfEffectObject* aoe, CNWSArea* area, float x, float y, float z, int p6)
{
	if (area == NULL || area->area_tiles == NULL) return 0;

	if (get_local_int(&(area->area_vartable), "MSDS_SUSPEND_TIME"))
	{
		RemoveObjectFromAI(server_internal->srv_ai, &(aoe->obj));
	}
	else if (aoe->obj.obj_ai_level == -1)
	{
		AddObjectToAI(server_internal->srv_ai, &(aoe->obj), 1);
	}

	return AddAOEToArea_Org(aoe, area, x, y, z, p6);
}
unsigned char d_ret_code_addtriggertoarea[0x20];
int (*AddTriggerToArea_Org)(CNWSTrigger*, CNWSArea*, float, float, float, int) = (int (*)(CNWSTrigger*, CNWSArea*, float, float, float, int))&d_ret_code_addtriggertoarea;
int AddTriggerToArea_HookProc(CNWSTrigger* trigger, CNWSArea* area, float x, float y, float z, int p6)
{
	if (area == NULL || area->area_tiles == NULL) return 0;

	if (get_local_int(&(area->area_vartable), "MSDS_SUSPEND_TIME"))
	{
		RemoveObjectFromAI(server_internal->srv_ai, &(trigger->obj));
	}
	else if (trigger->obj.obj_ai_level == -1)
	{
		AddObjectToAI(server_internal->srv_ai, &(trigger->obj), 0);
	}

	return AddTriggerToArea_Org(trigger, area, x, y, z, p6);
}

unsigned char d_ret_code_adddoortoarea[0x20];
int (*AddDoorToArea_Org)(CNWSDoor*, CNWSArea*, float, float, float, int) = (int (*)(CNWSDoor*, CNWSArea*, float, float, float, int))&d_ret_code_adddoortoarea;
int AddDoorToArea_HookProc(CNWSDoor* door, CNWSArea* area, float x, float y, float z, int p6)
{
	if (area == NULL || area->area_tiles == NULL) return 0;

	return AddDoorToArea_Org(door, area, x, y, z, p6);
}

unsigned char d_ret_code_packareaintomsg[0x20];
int (*PackAreaInMsg_Org)(CNWSArea*, int, int, int, CNWSPlayer*) = (int (*)(CNWSArea*, int, int, int, CNWSPlayer*))&d_ret_code_packareaintomsg;
int PackAreaInMsg_HookProc(CNWSArea* area, int i1, int i2, int i3, CNWSPlayer* player)
{
    if (area->area_tiles)
    {
        return PackAreaInMsg_Org(area, i1, i2, i3, player);
    }
    else
    {
        fprintf(stderr, "Invalid area:%s\n", area->area_tag.text);
        return 0;
    }
}

void (*InterTileDFSExploreArea)(CNWSArea*, uint8_t*, int, int, int, float, float) = (void (*)(CNWSArea*, uint8_t*, int, int, int, float, float))0x80d7fa4;
void OnExploreArea_InterTileDFSExploreArea(CNWSArea* area, uint8_t* data, int p3, int p4, int p5, float p6, float p7)
{
	size_t num_tiles = area->area_width*area->area_height;
	for (size_t i=0; i<num_tiles; i++)
	{
		if (area->area_tiles[i].data == NULL)
		{
			fprintf(stderr, "area: %s has a tile with no data\n", area->area_tag.text);
			return;
		}
	}
	return InterTileDFSExploreArea(area, data, p3, p4, p5, p6, p7);
}

uint32_t OnApplyUltravision_dummy_vfx[3] = {0,0,0};
void* OnApplyUltravision_dummy_vfx_ptr = &OnApplyUltravision_dummy_vfx;
void* OnApplyUltravision_GetEffectAtIndex(CExoArrayList<void*>* array_list, uint32_t index)
{
	if (index < 0 || index >= array_list->len)
	{
		fprintf(stderr, "crash fix: OnApplyUltravision_GetEffectAtIndex: invalid index:%d\n", index);
		return &OnApplyUltravision_dummy_vfx_ptr;
	}
	return &(array_list->data[index]);
}

unsigned char d_ret_code_updateeffectsptr[0x20];
void (*UpdateEffectPtrs_Org)(CNWSCreature*) = (void (*)(CNWSCreature*))&d_ret_code_updateeffectsptr;
void UpdateEffectPtrs_Hook(CNWSCreature* creature)
{
	memset(&creature->cre_stats->effects_ptr, 0, sizeof(creature->cre_stats->effects_ptr));
	return UpdateEffectPtrs_Org(creature);
}

CNWSItem OnSendInventory_CalculatePage_invalid_item_buffer;
bool OnSendInventory_CalculatePage_current_item_invalid = false;
uint8_t (*Repos_CalculatePage)(CItemRepository*, uint8_t, uint8_t) = (uint8_t (*)(CItemRepository*, uint8_t, uint8_t))0x81a6a38;
uint8_t OnSendInventory_CalculatePage(CItemRepository* repos, uint8_t p2, uint8_t p3)
{
	if (OnSendInventory_CalculatePage_current_item_invalid)
	{
		//fprintf(stderr, "crash fix: OnSendInventory_GetItemById\n");
		OnSendInventory_CalculatePage_current_item_invalid = false;
		return 0xFF;
	}
	return Repos_CalculatePage(repos, p2, p3);
}
CNWSItem* OnSendInventory_GetItemById(CServerExoApp* server, uint32_t item_id)
{
	CNWSItem* item = GetItemById(item_id);
	if (item == NULL)
	{
		OnSendInventory_CalculatePage_current_item_invalid = true;
		item = &OnSendInventory_CalculatePage_invalid_item_buffer;
	}
	return item;
}
int (*GetItemWeight)(CNWSItem*) = (int (*)(CNWSItem*))0x081a6b90;
int OnCalculateInventoryWeight_GetWeight(CNWSItem* item)
{
    if (item == NULL) return 0;
    return GetItemWeight(item);
}

CNWSClient* (*GetFirstClient)(void*) = (CNWSClient* (*)(void*))0x805f374;
uint32_t shutdown_try_count = 0;
CNWSClient* OnShutdown_GetFirstClient(void* clients_list)
{
	if (shutdown_try_count > 2000)
	{
		fprintf(stderr, "Couldn't disconnect all players.\n");
		return NULL;
	}
	CNWSClient* first_client = GetFirstClient(clients_list);
	if (first_client) shutdown_try_count++;
	return first_client;
}

int OnEnterAOE_RunScript(CVirtualMachine* vm, CExoString* script_name, uint32_t object_id, int p3)
{
	CGameObject* o = GetGameObject(object_id);
	if (o)
	{
		CNWSObject* object = o->vtable->AsNWSObject(o);
		if (object)
		{
			CNWSArea* area = GetAreaById(object->obj_area_id);
			if (area)
			{
				return run_script(script_name, object_id, p3);
			}
		}
	}
	fprintf(stderr, "OnEnterAOE_RunScript: area of: %x is invalid\n", object_id);
	return 0;
}

unsigned char d_ret_code_gettile[0x20];
void* (*GetTile_Org)(CNWSArea*, Vector) = (void* (*)(CNWSArea*, Vector))&d_ret_code_gettile;
void* GetTile_Hook(CNWSArea* area, Vector v)
{
	if (area && area->area_tiles)
	{
		return GetTile_Org(area, v);
	}
	return NULL;
}
unsigned char d_ret_code_computeheight[0x20];
float (*ComputeHeight_Org)(CNWSArea*, Vector) = (float (*)(CNWSArea*, Vector))&d_ret_code_computeheight;
float ComputeHeight_Hook(CNWSArea* area, Vector v)
{
	if (area && area->area_tiles)
	{
		return ComputeHeight_Org(area, v);
	}
	return 0.0;
}

int GetGameObject_MustBeNWSObject(void* array, uint32_t object_id, CGameObject** p_o)
{
	int ret = CGOA_GetObject(array, object_id, p_o);
	if (ret == CGOA_RET_SUCCESS)
	{
		if ((*p_o)->vtable->AsNWSObject(*p_o) == NULL)
		{
			fprintf(stderr, "GetGameObject_MustBeNWSObject invalid object type:%d (%s)\n", (*p_o)->type, get_last_script_ran().c_str());
			*p_o = NULL;
			ret = CGOA_RET_BAD_ID;
		}
	}
	return ret;
}

unsigned char d_ret_code_itemininv[0x20];
int (*GetItemInInventory_Org)(CNWSInventory*, CNWSItem*) = (int (*)(CNWSInventory*, CNWSItem*))&d_ret_code_itemininv;
int GetItemInInventory_HookProc(CNWSInventory* inventory, CNWSItem* item)
{
	if (item == NULL)
	{
		fprintf(stderr, "Crash Fix: GetItemInInventory\n");
		return false;
	}
	return GetItemInInventory_Org(inventory, item);
}

unsigned char d_ret_code_primod[0x20];
int (*GetPrimaryMod_Org)(CNWSCreatureStats*, uint8_t) = (int (*)(CNWSCreatureStats*, uint8_t))&d_ret_code_primod;
int GetPrimaryMod_HookProc(CNWSCreatureStats* stats, uint8_t nClass)
{
	if (nClass == 0xFF) return 0;
	return GetPrimaryMod_Org(stats, nClass);
}

int (*GetSpecialAttackDamageBonus)(CNWSCreatureStats*, CNWSCreature*) = (int (*)(CNWSCreatureStats*, CNWSCreature*))0x08149140;
int OnResolve_GetSpecialAttackDamageBonus_HookProc(CNWSCreatureStats* stats, CNWSCreature* target)
{
	if (target == NULL)
	{
		/*fprintf(stderr,"Crash Fix in GetSpecialAttackDamageBonus caused by:%x\n", stats->cs_original->obj.obj_id);
		fflush(stderr);*/
		return 0;
	}
	return GetSpecialAttackDamageBonus(stats, target);
}

uint8_t (*GetClass)(CNWSCreatureStats*, uint8_t) = (uint8_t (*)(CNWSCreatureStats*, uint8_t))0x08163da4;
int OnSetClassGetClass_HookProc(CNWSCreatureStats* stats, uint8_t index)
{
	uint8_t class_id = GetClass(stats, index);
	if (class_id == 255)
	{
		*(uint16_t*)(0x08140771) = 0xE990;
	}
	else
	{
		*(uint16_t*)(0x08140771) = 0x840F;
	}
	return class_id;
}

void* (*GetTile)(CNWSArea*, Vector) = (void* (*)(CNWSArea*, Vector))0x080cdcc8;
CNWSArea* BeforeGetTile_GetArea_HookProc(CNWSObject* object)
{
	CNWSArea* area = GetAreaById(object->obj_area_id);
	if (area != NULL)
	{
		if (GetTile(area, object->obj_position) != NULL)
		{
			return area;
		}
		/*else
		{
			fprintf(stderr,"Crash Fix: GetTile == NULL after GetArea\n");
			fflush(stderr);
		}*/
	}
	return NULL;
}

unsigned char d_ret_code_lvlupmsg[0x20];
void (*HandlePlayerToServerLevelUpMessage_Org)(CNWSMessage*, CNWSPlayer*, uint8_t) = (void (*)(CNWSMessage*, CNWSPlayer*, uint8_t))&d_ret_code_lvlupmsg;
void HandlePlayerToServerLevelUpMessage_HookProc(CNWSMessage* message, CNWSPlayer* player, uint8_t p3)
{
	CGameObject* oPlayer = GetGameObject(player->pl_oid);
	if (oPlayer == NULL) return;
	CNWSCreature* creature = oPlayer->vtable->AsNWSCreature(oPlayer);
	if (creature == NULL) return;
	if (creature->cre_is_pc == false)
	{
		fprintf(stderr,"Crash Fix in HandlePlayerToServerLevelUpMessage caused by:%x\n", creature->obj.obj_id);
		fflush(stderr);
		return;
	}

	return HandlePlayerToServerLevelUpMessage_Org(message, player, p3);
}

uint32_t ClearHostileActionsVersus_loop_count = 0;
CNWSCreature* ClearHostileActionsVersus_source = NULL;
CNWSCreature* ClearHostileActionsVersus_target = NULL;
unsigned char d_ret_code_clearhact[0x20];
void (*ClearHostileActionsVersus_Org)(CNWSCreature*, CNWSCreature*) = (void (*)(CNWSCreature*, CNWSCreature*))&d_ret_code_clearhact;
void ClearHostileActionsVersus_HookProc(CNWSCreature* source, CNWSCreature* target)
{
	ClearHostileActionsVersus_source = source;
	ClearHostileActionsVersus_target = target;
	ClearHostileActionsVersus_loop_count = 0;
	ClearHostileActionsVersus_Org(source, target);
}
CNWSAction* OnClearHostileActionsVersus_GetActionAt(CExoLinkedList* list, CExoLinkedListNode* node)
{
	CNWSAction* action = (CNWSAction*)node->data;
	if (ClearHostileActionsVersus_loop_count >= 100)
	{
		fprintf(stderr, "OnClearHostileActionsVersus...\n");
		fflush(stderr);
		CNWSArea* ClearHostileActionsVersus_source_area = GetAreaById(ClearHostileActionsVersus_source->obj.obj_area_id);
		CNWSArea* ClearHostileActionsVersus_target_area = GetAreaById(ClearHostileActionsVersus_target->obj.obj_area_id);
		fprintf(stderr, "source:%s/%s(%x) in %s(%x) --- target:%s/%s(%x) in %s(%x), action_type:%x action_sub_type:%x\n\n",
			ClearHostileActionsVersus_source->obj.obj_tag.text, ClearHostileActionsVersus_source->obj.obj_template, ClearHostileActionsVersus_source->obj.obj_id,
			(ClearHostileActionsVersus_source_area?ClearHostileActionsVersus_source_area->area_tag.text:NULL),
			(ClearHostileActionsVersus_source_area?ClearHostileActionsVersus_source_area->area_id:0),
			ClearHostileActionsVersus_target->obj.obj_tag.text, ClearHostileActionsVersus_target->obj.obj_template, ClearHostileActionsVersus_target->obj.obj_id,
			(ClearHostileActionsVersus_target_area?ClearHostileActionsVersus_target_area->area_tag.text:NULL),
			(ClearHostileActionsVersus_target_area?ClearHostileActionsVersus_target_area->area_id:0),
			action->act_type,
			action->act_subtype);
		return NULL;
	}
	ClearHostileActionsVersus_loop_count++;
	return action;
}

void (*AddCloseDoorAction)(CNWSObject*, uint32_t, int) = (void (*)(CNWSObject*, uint32_t, int))0x81d5590;
void ScriptAddCloseDoorAction(CNWSObject* object, uint32_t door_id, int p3)
{
	if (object != NULL)
	{
		return AddCloseDoorAction(object, door_id, p3);
	}
}

void init()
{
	hook_function(0x0821415c, (unsigned long)GetLastUsedBy_HookProc, d_ret_code_lastused, 12);

	hook_function(0x08101a10, (unsigned long)RandomWalk_HookProc, d_ret_code_randomwalk, 12);

	hook_function(0x0817f5f8, (unsigned long)GetEffectInteger, d_ret_code_geteffinteger, 12);
	
	hook_function(0x080a1b08, (unsigned long)MovePlayerToArea_HookProc, d_ret_code_movepltoarea, 12);

	hook_function(0x081d2f9c, (unsigned long)CalculateSpellRangedMissTarget_Hook, d_ret_code_calcspellmiss, 12);

	hook_function(0x080c1a80, (unsigned long)GetBaseItem_HookProc, d_ret_code_nouse, 11);
	
	//crash fix save container of items with no inventory
	hook_function(0x081a267c, (unsigned long)SaveContainerItems_Hook, d_ret_code_savecontaineritems, 9);
	//or invalid items
	hook_call(0x081a26ee, (uint32_t)OnSaveContainerItems_GetItem);
	
	//fix an infinite loop when cleaning or accepting a barter window while an item in it is invalid
	hook_call(0x080DEE66, (uint32_t)OnUpdateBarterItems_GetItemAtPos);
	hook_call(0x080DF951, (uint32_t)OnUpdateBarterItems_GetItemAtPos);
	hook_call(0x080DFB27, (uint32_t)OnUpdateBarterItems_GetItemAtPos);
	
	//jump to invalid location fix
	hook_function(0x08118d20, (unsigned long)AddCreatureToArea_HookProc, d_ret_code_addcretoarea, 12);
	hook_function(0x081df174, (unsigned long)AddPlaceableToArea_HookProc, d_ret_code_addplctoarea, 12);
	hook_function(0x081669bc, (unsigned long)AddDoorToArea_HookProc, d_ret_code_adddoortoarea, 12);
	hook_function(0x081ee668, (unsigned long)AddTriggerToArea_HookProc, d_ret_code_addtriggertoarea, 12);
	hook_function(0x081ea548, (unsigned long)AddAOEToArea_HookProc, d_ret_code_addaoetoarea, 12);
	
    //area with no tiles
    hook_function(0x080d0e00, (unsigned long)PackAreaInMsg_HookProc, d_ret_code_packareaintomsg, 12);

	hook_function(0x0817b630, (unsigned long)OnRemoveDisappearAppear_Hook, d_ret_code_rmdisapp, 12);
	
	//crash fix: creature in no faction
	hook_function(0x08113178, (unsigned long)GetCreatureFaction, d_ret_code_nouse, 11);

	//fix the deflect arrow relative weapon size bug, where it base itself on the attacker size
	enable_write(0x080ee39c+2);
	*(uint8_t*)(0x080ee39c+2) = 0xc;

	//crash fix when applying ultravision and trying to remove the last blindness effect
	hook_call(0x0817a638, (uint32_t)OnApplyUltravision_GetEffectAtIndex);

	//reset the effects ptr as it should...
	hook_function(0x0811dd04, (uint32_t)UpdateEffectPtrs_Hook, d_ret_code_updateeffectsptr, 12);

	//crash fix when exploring an invalid area
	hook_call(0x080d930f, (uint32_t)OnExploreArea_InterTileDFSExploreArea);

	//crash fix when talking to the module
	hook_call(0x081882af, (uint32_t)OnHandlePlayerToServerDialog_GetTargetObject);

	//crash fix when the weapon get destroyed, somehow, during the attack roll
	hook_call(0x08133091, (uint32_t)OnGetTotalEffectBonus_GetItemInSlot);

	//fix the 33% chance of crash when a ranged weapon get unequipped while missing an attack against a shield
	hook_call(0x080e8037, (uint32_t)OnResolveRangedAnimation_GetAttackWeapon);
	enable_write(0x080e8299);

	//Consider OBJECT_TYPE_SOUND for GetNearestObject nwscript function
	hook_call(0x082013f9, (uint32_t)OnGetNearestObject_PopObjectType);
	hook_call(0x082018f2, (uint32_t)OnGetNearestObject_GetGameObjectInArea);
	enable_write(0x08201eea);
	
	//crash fix: bash placeable + log out
	hook_call(0x081df9d0, (long)OnBashPlc_GetCreatureById);
	
	//crash fix: use non placeable object
	hook_call(0x081c72e0, (long)OnUseObject_GetTargetGameObject);
	
	//crash fix: GetFirstObject in non-area object
	hook_call(0x082084d9, (long)OnGetObjectInShape_GetAreaGameObject);
	
	//crash fix: Copy Object in a full inventory of a creature in an invalid area
	hook_call(0x0822a15c, (uint32_t)OnCopyObject_GetCallerGameObject);
	hook_call(0x0822a22d, (uint32_t)OnCopyObject_GetCallerAreaGameObject);
    hook_call(0x0822a10a, (uint32_t)OnCopyObject_GetCallerAreaGameObject);
	hook_call(0x0822a58d, (uint32_t)OnCopyObject_SetCreatureOrientation);
	hook_call(0x0822aacb, (uint32_t)OnCopyObject_SetDropPosition);

    //crash fix: GetItemInRepository when learning a spell from a a dropped scroll
    hook_call(0x0813ac3b, (uint32_t)OnLearnScroll_GetItemInRepository);
    
	//crash fix in GetObjectInShape, when there's an invalid object in the area
	enable_write(0x08208a9a);
	hook_call(0x08208a43, (uint32_t)OnGetObjectInShape_GetGameObjectInArea);

	//crash fix: acquire item
	hook_call(0x0810004e, (uint32_t)OnAcquireItem_AddItem);

	//infite loop fix: too many memorized spells (> 255)
	enable_write(0x0806e3bf);
	hook_call(0x0806e2d0, (uint32_t)OnWriteNumMemorizedSpells);
    
	//crash fix: invalid item in inventory
	hook_call(0x08070344, (long)OnSendInventory_GetItemById);
	enable_write(0x08070350); // no condition if the item is invalid, check page will return an invalid page instead
	*(uint16_t*)0x08070350 = 0x9090;
	hook_call(0x08070366, (long)OnSendInventory_CalculatePage);
    hook_call(0x081a4ba4, (long)OnCalculateInventoryWeight_GetWeight);
    
    //preven infinite loop when the server shutdown
	enable_write(0x080a5121);
	*(uint32_t*)(0x080a5121) = ((uint32_t)OnShutdown_GetFirstClient-(uint32_t)0x080a5125);
    
    //check that the aoe area is valid before triggered the on entered event
	hook_call(0x081EA4BC, (uint32_t)OnEnterAOE_RunScript);
    
	//crash fix: GetTile == NULL
	enable_write(0x08119c56);
	*(uint32_t*)(0x08119c56) = ((uint32_t)BeforeGetTile_GetArea_HookProc-(uint32_t)0x08119c5a);
	//crash fix: get tile while the "tile list" is NULL
	hook_function(0x80cdcc8, (unsigned long)GetTile_Hook, d_ret_code_gettile, 11);
	hook_function(0x80d65fc, (unsigned long)ComputeHeight_Hook, d_ret_code_computeheight, 11);

	//crash fix when starting a dialog with the module
	hook_call(0x081d03af, (long)GetGameObject_MustBeNWSObject);
	hook_call(0x081c3312, (long)GetGameObject_MustBeNWSObject);

	//no barter when possessing fix
	enable_write(0x0812fca9);
    
    hook_function(0x0819ec40, (unsigned long)GetItemInInventory_HookProc, d_ret_code_itemininv, 12);
	hook_function(0x0815d778, (unsigned long)GetPrimaryMod_HookProc, d_ret_code_primod, 11);
    
	hook_call(0x081489AF, (long)OnResolve_GetSpecialAttackDamageBonus_HookProc);
    
    //crash fix: GetClass in SetClass
	hook_call(0x0814074c, (long)OnSetClassGetClass_HookProc);
	//jmp change
	enable_write(0x08140771);
    
    hook_function(0x08198654, (unsigned long)HandlePlayerToServerLevelUpMessage_HookProc, d_ret_code_lvlupmsg, 12);
    
    //prevent infinite loop in ClearHostileActionsVersus
    hook_function(0x08137f50, (unsigned long)ClearHostileActionsVersus_HookProc, d_ret_code_clearhact, 12);
	hook_call(0x08137fae, (long)OnClearHostileActionsVersus_GetActionAt);
	
	//crash fix when script call action close door on an invalid object
	hook_call(0x08203396, (long)ScriptAddCloseDoorAction);
	
	register_hook(hook::module_loaded, []{
		CNWSItem_Constructor(&dummy_invalid_item, OBJECT_INVALID);
		dummy_invalid_item.it_baseitem = 67;
	});
}
REGISTER_INIT(init);

}