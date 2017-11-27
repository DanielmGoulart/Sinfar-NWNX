#include "core.h"
#include "nwscript.h"

using namespace nwnx::core;
using namespace nwnx::nwscript;

namespace nwnx
{
namespace fixes
{
    
struct CNWObjectVarListElement
{
    CExoString sVarName;
    uint32_t nVarType;
    uint32_t nVarValue;
};

struct CNWObjectVarList
{
    CNWObjectVarListElement *VarList;
    uint32_t VarCount;
};

bool CompareVarLists (CNWObjectVarList *pVarList1, CNWObjectVarList *pVarList2) {
    if (pVarList1->VarCount == 0 && pVarList2->VarCount == 0)
        return true;

    for (size_t i = 0; i < pVarList1->VarCount; i++) {
        bool bFound = false;
        CNWObjectVarListElement *pVar1 = &pVarList1->VarList[i];

        for (size_t j = 0; j < pVarList2->VarCount; j++) {
            CNWObjectVarListElement *pVar2 = &pVarList2->VarList[j];

            if (pVar1->nVarType == pVar2->nVarType && 
                (pVar1->sVarName.text == pVar2->sVarName.text ||
                    (pVar1->sVarName.text && pVar2->sVarName.text && strcmp(pVar1->sVarName.text, pVar2->sVarName.text) == 0))) {

                bFound = true;

                //Compare values
                switch (pVar1->nVarType) {
                case 1:  //int
                        if ((int)(pVar1->nVarValue) != (int)(pVar2->nVarValue)) {
#ifdef NWNX_FIXES_DEBUG
                            fixes.Log(3, "blocking merge: int value '%s' %d != %d\n", pVar1->sVarName.text,
                                      (int)(pVar1->nVarValue), (int)(pVar2->nVarValue));
#endif
                            return false;
                        }
                    break;

                case 2:  //float
                        if ((float)(pVar1->nVarValue) != (float)(pVar2->nVarValue)) {
#ifdef NWNX_FIXES_DEBUG
                            fixes.Log(3, "blocking merge: float value '%s' %.04f != %.04f\n", pVar1->sVarName.text,
                                      (float)(pVar1->nVarValue), (float)(pVar2->nVarValue));
#endif
                            return false;
                        }
                    break;

                case 3:  //string
                        // both pointers are equal or both are null
                        if ((char **)(pVar1->nVarValue) == (char **)(pVar2->nVarValue)) break;
                        
                        if ((char **)(pVar1->nVarValue) == NULL || (char **)(pVar2->nVarValue) == NULL) {  //the variable is not set on one of the objects
#ifdef NWNX_FIXES_DEBUG
                            fixes.Log(3, "blocking merge: string value '%s' is not set on one of the objects\n", pVar1->sVarName.text);
#endif
                            return false;
                        }

                        if (*(char **)(pVar1->nVarValue) == *(char **)(pVar2->nVarValue))  //equal pointers
                            break;
                        if (*(char **)(pVar1->nVarValue) == NULL || *(char **)(pVar2->nVarValue) == NULL) { //one of the variables is empty
#ifdef NWNX_FIXES_DEBUG
                            fixes.Log(3, "blocking merge: string value '%s' is not set on one of the objects\n", pVar1->sVarName.text);
#endif
                            return false;
                        }

                        if (strcmp(*(char **)(pVar1->nVarValue), *(char **)(pVar2->nVarValue)) != 0) {  //string values are not equal
#ifdef NWNX_FIXES_DEBUG
                            fixes.Log(3, "blocking merge: string value '%s' '%s' != '%s'\n", pVar1->sVarName.text,
                                      *(char **)(pVar1->nVarValue), *(char **)(pVar2->nVarValue));
#endif
                            return false;
                        }
                        break;

                case 4:  //object
                        if ((uint32_t)(pVar1->nVarValue) != (uint32_t)(pVar2->nVarValue)) {
#ifdef NWNX_FIXES_DEBUG
                            fixes.Log(3, "blocking merge: object value '%s' %08X != %08X\n", pVar1->sVarName.ext,
                                      (uint32_t)(pVar1->nVarValue), (uint32_t)(pVar2->nVarValue));
#endif
                            return false;
                        }
                    break;

                case 5:  //location
                    break;
                }

                break;
            }
        }

        if (!bFound) {
#ifdef NWNX_FIXES_DEBUG
            fixes.Log(3, "blocking merge: local variable '%s' not found on one of the objects", pVar1->sVarName.text);
#endif
            return false;
    }
}

    return true;
}

unsigned char d_ret_code_merg[0x20];
int (*GetIsMergeable_Org)(CNWSItem*, CNWSItem*) = (int (*)(CNWSItem*, CNWSItem*))&d_ret_code_merg;
int GetIsMergeableHookProc(CNWSItem* item1, CNWSItem* item2)
{
    int ret = GetIsMergeable_Org(item1, item2);
    if(ret)
    {
        //Check local vars
        CNWObjectVarList *pVarList1 = (CNWObjectVarList*)((char*)item1+0x10+0x4+0xD8);
        CNWObjectVarList *pVarList2 = (CNWObjectVarList*)((char*)item2+0x10+0x4+0xD8);
        if(!pVarList1 && !pVarList2) return 1;
        if(!pVarList1 || !pVarList2) {
#ifdef NWNX_FIXES_DEBUG
                    fixes.Log(3, "blocking merge: one object has a variable list and the other does not\n");
#endif
                    return 0;
                }
        return (CompareVarLists(pVarList1, pVarList2) && CompareVarLists(pVarList2, pVarList1));
    }
    return ret;
}

int (*SetActivity)(CNWSCreature*, int, int) = (int (*)(CNWSCreature*, int, int))0x081152e8;
int OnDialog_SetActivity(CNWSCreature* creature, int p2, int p3)
{
    CNWSAction* action = ((CNWSObject*)creature)->obj_ai_action;
    if (*(uint32_t*)(((char*)action)+0x34) == ((CNWSObject*)creature)->obj_id) return 0;
    return SetActivity(creature, p2, p3);
}

int (*VMPopObject)(void*, uint32_t*) = (int (*)(void*, uint32_t*))0x08262dfc;
int OnScriptDestroyObject_PopObject(void* vm, uint32_t* p_object_id)
{
	int nRet = VMPopObject(vm, p_object_id);
	CGameObject* o = NULL;
	CGOA_GetObject(NULL, *p_object_id, &o);
	if (o)
	{
		if (o->type == OBJECT_TYPE_ITEM)
		{
			CNWSItem* item = o->vtable->AsNWSItem(o);
			if (item)
			{
				if (get_local_int(&(item->obj.obj_vartable), "DELETED"))
				{
					//fprintf(stderr, "trying to delete item twice:%s owned by:%x in script:%s\n", item->obj.obj_tag.text, item->it_possessor, get_last_script_ran().c_str());
				}
				else
				{
					set_local_int(&(item->obj.obj_vartable), "DELETED", 1);
				}
			}
		}
	}
	//run_script("mod_ev_destryobj", *p_object_id);
	return nRet;
}
CNWSItem* OnCopyAndModifyItem_GetItemById(CServerExoApp* server, uint32_t item_id)
{
	CNWSItem* item = GetItemById(item_id);
	if (item)
	{
		if (get_local_int(&(item->obj.obj_vartable), "DELETED"))
		{
			fprintf(stderr, "CopyItemAndModify with deleted item:%s owned by:%x in script:%s\n", item->obj.obj_tag.text, item->it_possessor, get_last_script_ran().c_str());
			item = NULL;
		}
	}
	return item;
}
int OnCopyObject_GetGameObject(void* array, uint32_t object_id, CGameObject** p_o)
{
	int ret = CGOA_GetObject(array, object_id, p_o);
	if (ret == CGOA_RET_SUCCESS)
	{
		CNWSItem* item = (*p_o)->vtable->AsNWSItem(*p_o);
		if (item)
		{
			if (get_local_int(&(item->obj.obj_vartable), "DELETED"))
			{
				fprintf(stderr, "CopyObject with deleted item:%s owned by:%x in script:%s\n", item->obj.obj_tag.text, item->it_possessor, get_last_script_ran().c_str());
				*p_o = NULL;
				ret = CGOA_RET_BAD_ID;
			}
		}
	}
	return ret;
}
inline CNWSItem* validate_save_item(CNWSItem* item)
{
	if (item)
	{
		if (get_local_int(&(item->obj.obj_vartable), "DELETED"))
		{
			fprintf(stderr, "not saving deleted item:%s owned by:%x\n", item->obj.obj_tag.text, item->it_possessor);
			item = NULL;
		}
	}
	return item;
}
CNWSItem* OnSaveCreature_GetItemInSlot(CNWSInventory* inventory, uint32_t slot_flag)
{
	CNWSItem* item = get_item_in_slot(inventory, slot_flag);
	item = validate_save_item(item);
	return item;
}
CNWSItem* OnSaveCreature_GetItemById(CServerExoApp* server, uint32_t item_id)
{
	CNWSItem* item = GetItemById(item_id);
	item = validate_save_item(item);
	return item;
}
CExoLinkedListNode* validate_save_repository_node(CExoLinkedListNode* node)
{
	if (node)
	{
		CNWSItem* item = GetItemById(*(uint32_t*)(node->data));
		if (item && !validate_save_item(item))
		{
			node = validate_save_repository_node(node->next);
		}
	}
	return node;
}
uint32_t* OnSaveCreature_GetItemRepositoryNext(CExoLinkedList* list, CExoLinkedListNode** p_node)
{
	*p_node = validate_save_repository_node((*p_node)->next);
	if (*p_node)
	{
		return (uint32_t*)(*p_node)->data;
	}
	else
	{
		return NULL;
	}
}
CExoLinkedListNode* OnSaveCreature_GetItemRepositoryHeadPos(CExoLinkedList* list)
{
	return validate_save_repository_node(list->header->first);
}

uint32_t last_combat_state_change_creature_id = OBJECT_INVALID;
unsigned char d_ret_code_broadcombat[0x20];
int (*BroadcastCombatState_Org)(CNWSCreature*) = (int (*)(CNWSCreature*))&d_ret_code_broadcombat;
int BroadcastCombatState_HookProc(CNWSCreature* creature)
{
	last_combat_state_change_creature_id = creature->obj.obj_id;
	*(uint32_t*)(0x08128e97) = 0xfeff8c0f;
	*(uint16_t*)(0x08128e9b) = 0xffff;

	return BroadcastCombatState_Org(creature);
}
CNWSPlayer* OnBroadcastCombatState_GetPlayer(CServerExoApp* server, uint32_t o_id)
{
	*(uint32_t*)(0x08128e97) = 0x90909090;
	*(uint16_t*)(0x08128e9b) = 0x9090;
	return get_player_by_game_object_id(last_combat_state_change_creature_id);
}

unsigned char d_ret_code_setconbase[0x20];
void (*SetCONBase_Org)(CNWSCreatureStats*, uint8_t, int) = (void (*)(CNWSCreatureStats*, uint8_t, int))&d_ret_code_setconbase;
void SetCONBase_HookProc(CNWSCreatureStats* stats, uint8_t con, int p3)
{
	if (stats->cs_original->obj.obj_hp_cur > -11)
	{
		*(uint32_t*)0x08151aec = 0x00b88389;
		*(uint16_t*)(0x08151aec+4) = 0x0000;
	}
	else
	{
		*(uint32_t*)0x08151aec = 0x90909090;
		*(uint16_t*)(0x08151aec+4) = 0x9090;
	}
	return SetCONBase_Org(stats, con, p3);
}

unsigned char d_ret_code_bumpfriends[0x20];
int (*IsBumpable_Org)(CNWSCreature*,CNWSCreature*) = (int (*)(CNWSCreature*,CNWSCreature*))&d_ret_code_bumpfriends;
int IsBumpable_HookProc(CNWSCreature* creature,CNWSCreature* creature_target)
{
	if (creature_target->cre_is_pc && !creature_target->cre_combat_state && !creature->cre_combat_state) return false;

	return IsBumpable_Org(creature, creature_target);
}

unsigned char d_ret_code_dayfromsec[0x20];
int (*GetDayFromSeconds_Org)(CWorldTimer*, float) = (int (*)(CWorldTimer*, float))&d_ret_code_dayfromsec;
int GetDayFromSeconds_HookProc(CWorldTimer* world_timer, float sec)
{
	if (sec > 1000000)	sec = 1000000;
	return GetDayFromSeconds_Org(world_timer, sec);
}
unsigned char d_ret_code_timeofdayfromsec[0x20];
int (*GetTimeOfDayFromSeconds_Org)(CWorldTimer*, float) = (int (*)(CWorldTimer*, float))&d_ret_code_timeofdayfromsec;
int GetTimeOfDayFromSeconds_HookProc(CWorldTimer* world_timer, float sec)
{
	if (sec > 1000000)	sec = 1000000;
	return GetTimeOfDayFromSeconds_Org(world_timer, sec);
}

void ClearAllPartyInvalidActions_HookProc(CNWSObject* object)
{
	return;
}

int OnEquipMerge(CNWSItem* item1, CNWSItem* item2)
{
	return 0;
}

void DoNotCheckMasterServerTimeouts(CNetLayerInternal* net_layer_internal)
{
	return;
}
bool gs_init = false;
unsigned char d_ret_code_connecttogs[0x20];
int (*ServerConnectToGameSpy_Org)(void*, int) = (int (*)(void*, int))&d_ret_code_connecttogs;
int ServerConnectToGameSpy_Hook(void* connection_lib, int socket)
{
	if (gs_init)
	{
		return 1;
	}
	else
	{
		gs_init =  ServerConnectToGameSpy_Org(connection_lib, socket);
		return gs_init;
	}
}

void init()
{
    //hide chars level
    enable_write(0x0807e586);
    memcpy((char*)0x0807e586, "\xB0\x00\x90\x90\x90\x90", 6);
    
    //copy vars when splitting item
    enable_write(0x081a326f);
    *(uint8_t*)(0x081a326f+1) = 0x1;
    //when merging
    enable_write(0x08192d65);
    *(uint8_t*)(0x08192d65+1) = 0x0;
    hook_function(0x081A2ED0, (long)GetIsMergeableHookProc, d_ret_code_merg, 9);
    //when buying an item
    enable_write(0x080855f4);
    *(uint8_t*)(0x080855f4+1) = 0x1;

    //stay hidden when starting a conversation
    hook_call(0x081c3427, (long)OnDialog_SetActivity);
    
    //log destroy object vm call
    //and prevent dup exploits
    hook_call(0x0820e865, (long)OnScriptDestroyObject_PopObject);
    //do not copy items that should be deleted
    hook_call(0x082200d7, (long)OnCopyAndModifyItem_GetItemById);
    hook_call(0x08229fb5, (long)OnCopyObject_GetGameObject);
    //do not save deleted items
    hook_call(0x08120EFD, (long)OnSaveCreature_GetItemInSlot);
    hook_call(0x08121254, (long)OnSaveCreature_GetItemById);
    hook_call(0x0812105c, (long)OnSaveCreature_GetItemRepositoryNext);
    hook_call(0x08120f75, (long)OnSaveCreature_GetItemRepositoryHeadPos);

	//dont broadcast combat state : dont get disturbed by party members
	hook_call(0x08128db1, (long)OnBroadcastCombatState_GetPlayer);
	enable_write(0x08128e97);
	hook_function(0x08128d34, (unsigned long)BroadcastCombatState_HookProc, d_ret_code_broadcombat, 12);

	//do not change the HP from the constitution when dead
	enable_write(0x08151aec);
	hook_function(0x081519f0, (unsigned long)SetCONBase_HookProc, d_ret_code_setconbase, 12);
	
	//do not bump pcs when not in combat
	hook_function(0x0813811c, (unsigned long)IsBumpable_HookProc, d_ret_code_bumpfriends, 12);
	
	//prevent lag when the number of second is too big
	hook_function(0x08241a8c, (unsigned long)GetDayFromSeconds_HookProc, d_ret_code_dayfromsec, 12);
	hook_function(0x08241af0, (unsigned long)GetTimeOfDayFromSeconds_HookProc, d_ret_code_timeofdayfromsec, 11);
	
	//no cancel when leaving a party
	hook_function(0x081cb908, (unsigned long)ClearAllPartyInvalidActions_HookProc, d_ret_code_nouse, 12);
	
	//VM action examine: add action to front
	hook_call(0x08236056, (long)AddActionToFront);
	//VM action play sound: add action to front
	hook_call(0x08203624, (long)AddActionToFront);
	
	//bypass merge item on equip because of a bug
	hook_call(0x08117271, (long)OnEquipMerge);
	
	hook_call(0x082AA529, (long)DoNotCheckMasterServerTimeouts);
	//do no try to connect to the master server / gamespy
	hook_function(0x08272050, (unsigned long)ServerConnectToGameSpy_Hook, d_ret_code_connecttogs, 12);
}
REGISTER_INIT(init);
    
}
}

