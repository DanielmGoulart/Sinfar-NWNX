#include "core.h"

using namespace nwnx::core;

namespace
{
	
CExoString* (*GetModuleScript)(CNWSModule*, int) = (CExoString* (*)(CNWSModule*, int))0x081c1050;
void (*CScriptEvent_Destructor)(CScriptEvent*) = (void (*)(CScriptEvent*))0x0806189c;
uint32_t (*CScriptEvent_GetObjectID)(CScriptEvent*, int) = (uint32_t (*)(CScriptEvent*, int))0x08061a10;
void AddUnAcquireItemEvent_HookProc(CServerAIMaster* server_ai, uint32_t, uint32_t, uint32_t looser_id, uint32_t, uint32_t event_id, CScriptEvent* script_event)
{
	uint32_t item_id = CScriptEvent_GetObjectID(script_event, 0);
	char* module = (char*)get_module();
	CExoString* script_name = GetModuleScript((CNWSModule*)module, 8);
	if(script_name->text && *(script_name->text))
	{
		*(uint32_t*)(0x180+module) = item_id;
		*(uint32_t*)(0x184+module) = looser_id;
		run_script(script_name->text, 0);
	}
	CScriptEvent_Destructor(script_event);
	free(script_event);
}
void (*RemoveItem)(CNWSCreature*, CNWSItem*, int, int, int, int) =
	(void (*)(CNWSCreature*, CNWSItem*, int, int, int, int))0x081000e8;
void RemoveItem_OnDestroy_HookProc(CNWSCreature* creature, CNWSItem* item, int i1, int i2, int i3, int i4)
{
	*(uint32_t*)(0x0819f4ae) = ((uint32_t)AddUnAcquireItemEvent_HookProc-(uint32_t)0x0819f4b2);
	RemoveItem(creature, item, i1, i2, i3, i4);
	*(uint32_t*)(0x0819f4ae) = 0xffef6ed6;
}

void (*DestroyCNWCCMessageData)(CNWCCMessageData*, int) = (void (*)(CNWCCMessageData*, int))0x80c2f88;
unsigned char d_ret_code_sendfeedback[0x20];
void (*SendFeedbackMessage_Org)(CNWSCreature*, uint16_t, CNWCCMessageData*, CNWSPlayer*) = (void (*)(CNWSCreature*, uint16_t, CNWCCMessageData*, CNWSPlayer*))&d_ret_code_sendfeedback;
void SendFeedbackMessage_HookProc(CNWSCreature* creature, uint16_t msg_id, CNWCCMessageData* data, CNWSPlayer* player)
{
	if (player == NULL)
	{
		if (creature == NULL || get_player_by_game_object_id(creature->obj.obj_id)==NULL)
		{
			if (data)
			{
				DestroyCNWCCMessageData(data, 3);
			}
			return;
		}
	}
	SendFeedbackMessage_Org(creature, msg_id, data, player);
}
unsigned char d_ret_code_delvar[0x20];
int (*DeleteLocalVariable_Org)(CNWSScriptVarTable*, int) = (int (*)(CNWSScriptVarTable*, int))&d_ret_code_delvar;
int DeleteLocalVariable_HookProc(CNWSScriptVarTable* var_table, int nIndex)
{
	CScriptVariable* var = &(var_table->vt_list[nIndex]);
	if (var->var_type == 3)//string
	{
		if (((CExoString*)var->var_value)->text) free(((CExoString*)var->var_value)->text);
		free((void*)var->var_value);
	}
	else if (var->var_type == 5)//location
	{
		free((void*)var->var_value);
	}
	return DeleteLocalVariable_Org(var_table, nIndex);
}
void (*DestroyCGameEffect)(CGameEffect*, int) = (void (*)(CGameEffect*, int))0x0817dfdc;
void DeleteOldSubTypeEffect_HookProc(CGameEffect* e)
{
	DestroyCGameEffect(e, 3);
}
CNWSCreature* last_ai_drive_creature = NULL;
unsigned char d_ret_code_aidrive[0x20];
uint32_t (*AIActionDrive_Org)(CNWSCreature*, void*) = (uint32_t (*)(CNWSCreature*, void*))&d_ret_code_aidrive;
uint32_t AIActionDrive_HookProc(CNWSCreature* creature, void* node)
{
	last_ai_drive_creature = creature;
	return AIActionDrive_Org(creature, node);
}
void* OnAIDrive_NewVec(uint32_t size)
{
	void* prev_data = last_ai_drive_creature->cre_pathfinding->unk_0x80;
	if (prev_data)
	{
		free(prev_data);
	}
	return malloc(size);
}
unsigned char d_ret_code_shutdownpath[0x20];
void (*ShutdownPathfinding_Org)(CPathfindInformation*) = (void (*)(CPathfindInformation*))&d_ret_code_shutdownpath;
void ShutdownPathfinding_HookProc(CPathfindInformation* path_finding)
{
	ShutdownPathfinding_Org(path_finding);
	path_finding->unk_0x80 = NULL;
	if (path_finding->unk_0x17c)
	{
		free(path_finding->unk_0x17c);
		path_finding->unk_0x17c = NULL;
	}
	if (path_finding->unk_0x15c)
	{
		free(path_finding->unk_0x15c);
		path_finding->unk_0x15c = NULL;
	}
}
CNWCCMessageData** GetCNWCCMessageDataArrayIndexAndRemove(CExoArrayList<void*>* array_list, uint32_t index)
{
	if (index < 0 || index >= array_list->len) return NULL;
	CNWCCMessageData** ret = reinterpret_cast<CNWCCMessageData**>(&(array_list->data[index]));
	array_list->delindex(index);
	return ret;
}
void DestroyCNWCCMessageDataArray(CExoArrayList<void*>* array_list, int free_self)
{
	for (uint32_t i=0; i<array_list->len; i++)
	{
		DestroyCNWCCMessageData(static_cast<CNWCCMessageData*>(array_list->data[i]), 3);
	}
	free(array_list->data);
	if (free_self & 1)
	{
		free(array_list);
	}
}
CGameEffect** GetGameEffectDataFromArrayIndexAndRemove(CExoArrayList<void*>* array_list, uint32_t index)
{
	if (index < 0 || index >= array_list->len) return NULL;
	CGameEffect** ret = reinterpret_cast<CGameEffect**>(&(array_list->data[index]));
	if (index == array_list->len -1)
	{
		array_list->len = 0;
	}
	return ret;
}
void DestroyCombatAttackEffectsArray(CExoArrayList<void*>* array_list, int free_self)
{
	for (uint32_t i=0; i<array_list->len; i++)
	{
		DestroyCGameEffect(static_cast<CGameEffect*>(array_list->data[i]), 3);
	}
	free(array_list->data);
	if (free_self & 1)
	{
		free(array_list);
	}
}
void* GetCNWSSpellScriptDataFromArrayIndexAndRemove(CExoArrayList<void*>* array_list, uint32_t index)
{
	if (index < 0 || index >= array_list->len) return NULL;
	void* ret = &(array_list->data[index]);
	if (index == array_list->len -1)
	{
		array_list->len = 0;
	}
	return ret;
}
void DestroyCExoArrayList_ptr_AndFreePointers(CExoArrayList<void*>* array_list, int free_self)
{
	for (uint32_t i=0; i<array_list->len; i++)
	{
		free(array_list->data[i]);
	}
	free(array_list->data);
	if (free_self & 1)
	{
		free(array_list);
	}
}
unsigned char d_ret_code_clearattackdata[0x20];
void (*ClearAttackData_Org)(CNWSCombatAttackData*) = (void (*)(CNWSCombatAttackData*))&d_ret_code_clearattackdata;
void ClearAttackData_Hook(CNWSCombatAttackData* attack_data)
{
	for (uint32_t i=0; i<attack_data->attack_messages.len; i++)
	{
		DestroyCNWCCMessageData(static_cast<CNWCCMessageData*>(attack_data->attack_messages.data[i]), 3);
	}
	for (uint32_t i=0; i<attack_data->attack_effects.len; i++)
	{
		DestroyCGameEffect(static_cast<CGameEffect*>(attack_data->attack_effects.data[i]), 3);
	}
	for (uint32_t i=0; i<attack_data->attack_spellscriptdata1.len; i++)
	{
		free(attack_data->attack_spellscriptdata1.data[i]);
	}
	for (uint32_t i=0; i<attack_data->attack_spellscriptdata2.len; i++)
	{
		free(attack_data->attack_spellscriptdata2.data[i]);
	}
	return ClearAttackData_Org(attack_data);
}
void DeleteLoopingVFXArray(CExoArrayList<void*>* array_list, int free_self)
{
	for (uint32_t i=0; i<array_list->len; i++)
	{
		free(array_list->data[i]);
	}
	free(array_list->data);
	if (free_self & 1)
	{
		free(array_list);
	}
}
void EmptyLoopingVFXArray(CExoArrayList<void*>* array_list, uint32_t size)
{
	for (uint32_t i=0; i<array_list->len; i++)
	{
		free(array_list->data[i]);
	}
	free(array_list->data);
	array_list->len = 0;
	array_list->alloc = 0;
	array_list->data = NULL;
}
unsigned char d_ret_code_destroypartyobjectlist[0x20];
void (*DestroyLastPartyUpdateObjectList_Org)(CExoLinkedList*, int) = (void (*)(CExoLinkedList*, int))&d_ret_code_destroypartyobjectlist;
void DestroyLastPartyUpdateObjectList_Hook(CExoLinkedList* party_objects, int p2)
{
	CExoLinkedListNode* node = party_objects->header->first;
	while (node)
	{
		free(node->data);
		node = node->next;
	}
	return DestroyLastPartyUpdateObjectList_Org(party_objects, p2);
}
unsigned char d_ret_code_destroyarea[0x20];
void (*DestroyArea_Org)(CNWSArea*, int) = (void (*)(CNWSArea*, int))&d_ret_code_destroyarea;
void DestroyArea_HookProc(CNWSArea* area, int p2)
{
	char* p_area = (char*)area;
	if (*(p_area+0x1a0))
	{
		free(p_area+0x1a0);
		*(p_area+0x1a0) = 0;
	}
	return DestroyArea_Org(area, p2);
}

void init()
{
	//call the unacquire item event immediately, when the item is beeing destroyed
	enable_write(0x0819f4ae);
	enable_write(0x0819fcaa);
	*(uint32_t*)(0x0819fcaa) = ((uint32_t)RemoveItem_OnDestroy_HookProc-(uint32_t)0x0819fcae);
	
	hook_function(0x0813533c, (unsigned long)SendFeedbackMessage_HookProc, d_ret_code_sendfeedback, 12);
	hook_function(0x81f4584, (unsigned long)DeleteLocalVariable_HookProc, d_ret_code_delvar, 12);
	
	//fix SetEffectSubType mem leak
	hook_call(0x082073a6, (long)DeleteOldSubTypeEffect_HookProc);

	hook_function(0x081052c0, (unsigned long)AIActionDrive_HookProc, d_ret_code_aidrive, 12);
	hook_call(0x08106211, (uint32_t)OnAIDrive_NewVec);
	hook_function(0x08241fc0, (unsigned long)ShutdownPathfinding_HookProc, d_ret_code_shutdownpath, 14);
	//fix CExoArrayList<CNWCCMessageData *>::~CExoArrayList(void) leak
	hook_function(0x080e56b0, (unsigned long)DestroyCNWCCMessageDataArray, d_ret_code_nouse, 12);
	hook_function(0x080e0314, (unsigned long)ClearAttackData_Hook, d_ret_code_clearattackdata, 12);
	hook_call(0x080E9697, (uint32_t)GetCNWCCMessageDataArrayIndexAndRemove);
	hook_call(0x080EAF5F, (uint32_t)GetCNWCCMessageDataArrayIndexAndRemove);
	hook_call(0x080EB0FE, (uint32_t)GetCNWCCMessageDataArrayIndexAndRemove);
	hook_call(0x080E4BEC, (uint32_t)DestroyCombatAttackEffectsArray);
	hook_call(0x080EAFCF, (uint32_t)GetGameEffectDataFromArrayIndexAndRemove);
	hook_call(0x080E97EB, (uint32_t)GetGameEffectDataFromArrayIndexAndRemove);
	hook_call(0x080E9833, (uint32_t)GetCNWSSpellScriptDataFromArrayIndexAndRemove);
	hook_call(0x080E9898, (uint32_t)GetCNWSSpellScriptDataFromArrayIndexAndRemove);
	hook_call(0x080EB017, (uint32_t)GetCNWSSpellScriptDataFromArrayIndexAndRemove);
	hook_call(0x080EB080, (uint32_t)GetCNWSSpellScriptDataFromArrayIndexAndRemove);
	hook_function(0x080E5644, (unsigned long)DestroyCExoArrayList_ptr_AndFreePointers, d_ret_code_nouse, 12);
	enable_write(0x081CED8B);
	*(uint16_t*)0x081CED8B = 0x9090; //always destroy the game effect when OnEffectApplyed return EFFECT_DELETE
	hook_function(0x081d6418, (unsigned long)DeleteLoopingVFXArray, d_ret_code_nouse, 12);
	hook_call(0x080625DD, (uint32_t)EmptyLoopingVFXArray);
	hook_function(0x805f16c, (unsigned long)DestroyLastPartyUpdateObjectList_Hook, d_ret_code_destroypartyobjectlist, 12);
	
	hook_function(0x080cc244, (unsigned long)DestroyArea_HookProc, d_ret_code_destroyarea, 12);
}
REGISTER_INIT(init);
	
}