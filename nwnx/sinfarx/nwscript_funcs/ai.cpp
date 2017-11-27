#include "nwscript_funcs.h"

namespace
{

inline CServerAIEventNode* get_ai_event_node(int event_index)
{
	CExoLinkedListNode* node = server_internal->srv_ai->ai_pending_events.header->first;
	int cur_index = 0;
	while (node && cur_index<event_index)
	{
		node = node->next;
		cur_index++;
	}
	return (CServerAIEventNode*)(node ? node->data : NULL);
}
VM_FUNC_NEW(GetAIEventParam1, 278)
{
	CServerAIEventNode* event_node = get_ai_event_node(vm_pop_int());
	vm_push_int(event_node ? event_node->obj_id : -1);
}
VM_FUNC_NEW(GetAIEventParam2, 279)
{
	CServerAIEventNode* event_node = get_ai_event_node(vm_pop_int());
	vm_push_int(event_node ? event_node->obj_self_id : -1);
}
VM_FUNC_NEW(GetAIEventType, 280)
{
	CServerAIEventNode* event_node = get_ai_event_node(vm_pop_int());
	vm_push_int(event_node ? event_node->event_type : -1);
}
VM_FUNC_NEW(GetAIEventScriptName, 281)
{
	CServerAIEventNode* event_node = get_ai_event_node(vm_pop_int());
	CExoString result(event_node && ((CVirtualMachineScript*)event_node->event_data)->script_name.text  ? ((CVirtualMachineScript*)event_node->event_data)->script_name.text : "");
	vm_push_string(&result);
}
VM_FUNC_NEW(GetAIEventCount, 282)
{
	int event_count = 0;
	int event_type = vm_pop_int();
	if (event_type == 0)
	{
		CExoLinkedListNode* node = server_internal->srv_ai->ai_pending_events.header->first;
		while (node)
		{
			event_count++;
			node = node->next;
		}
	}
	else
	{
		CExoLinkedListNode* node = server_internal->srv_ai->ai_pending_events.header->first;
		while (node)
		{
			if (((CServerAIEventNode*)node)->event_type == event_type)
			{
				event_count++;
			}
			node = node->next;
		}
	}
	vm_push_int(event_count);
}
VM_FUNC_NEW(GetAIObjectByIndex, 291)
{
	uint32_t ai_level = vm_pop_int();
	uint32_t obj_index = vm_pop_int();
	CServerAIMaster* server_ai = server_internal->srv_ai;
	uint32_t result = OBJECT_INVALID;
	if (ai_level <= 4 && obj_index < server_ai->ai_objects[ai_level].objects.len)
	{
		result = server_ai->ai_objects[ai_level].objects.data[obj_index];
	}
	vm_push_object(result);
}

VM_FUNC_NEW(GetActionTarget, 3)
{
	CNWSObject* object = GetObjectById(vm_pop_object());
	uint32_t result = OBJECT_INVALID;
	if (object && object->obj_actions.header)
	{
		CExoLinkedList* action_list = &object->obj_actions;
		CExoLinkedListNode* action_node = action_list->header->first;
		while (action_node && result==OBJECT_INVALID)
		{
			CNWSAction* action = (CNWSAction*)action_node->data;
			int action_type = action->act_type;
			if (action_type == 12) //attack (target = 52)
			{
				result = action->field_34;
			}
			else if (action_type == 15) //cast spell (target = 72)
			{
				result = action->field_48;
			}
			else if (action_type == 46) //use item
			{
				result = action->field_40;
				/*To find the target
				for (int i=0; i<100; i++)
				{
					fprintf(stderr, "use item target %d:%x\n", i, *(uint32_t*)((char*)action_node->data+i));
				}*/
			}
			action_node = action_node->next;
		}
	}
	vm_push_object(result);
}
VM_FUNC_NEW(GetActionCount, 137)
{
	CNWSObject* object = GetObjectById(vm_pop_object());
	vm_push_int(object && object->obj_actions.header ? object->obj_actions.header->len : 0);
}
VM_FUNC_NEW(GetActionType, 138)
{
	CNWSObject* object = GetObjectById(vm_pop_object());
	uint32_t action_pos = vm_pop_int();
	int result = 0;
	if (object && object->obj_actions.header)
	{
		CExoLinkedList* actions_list = &object->obj_actions;
		if (actions_list->header->len > 0 && actions_list->header->len > action_pos)
		{
			CExoLinkedListNode* action_node = actions_list->header->first;
			for (uint32_t pos=1; pos<action_pos; pos++)
			{
				action_node = action_node->next;
			}
			CNWSAction* action = (CNWSAction*)action_node->data;
			result = action->act_type;
		}
	}
	vm_push_int(result);
}

int (*AddObjectToAI)(CServerAIMaster*, CNWSObject*, int) = (int (*)(CServerAIMaster*, CNWSObject*, int))0x080980e0;
int (*RemoveObjectFromAI)(CServerAIMaster*, CNWSObject*) = (int (*)(CServerAIMaster*, CNWSObject*))0x080962a8;
VM_FUNC_NEW(RemoveObjectFromAI, 126)
{
	uint32_t object_id = vm_pop_object();
	CGameObject* o = GetGameObject(object_id);
	if (o)
	{
		CNWSObject* object = o->vtable->AsNWSObject(o);
		if (object != NULL)
		{
			RemoveObjectFromAI(server_internal->srv_ai, object);
		}
	}
}
VM_FUNC_NEW(AddObjectToAI, 127)
{
	uint32_t object_id = vm_pop_object();
	int ai_level = vm_pop_int();
	CGameObject* o = GetGameObject(object_id);
	if (o)
	{
		CNWSObject* object = o->vtable->AsNWSObject(o);
		if (object != NULL)
		{
			if (ai_level != 0)
			{
				object->obj_ai_level = ai_level;
			}
			AddObjectToAI(server_internal->srv_ai, object, ai_level);
		}
	}
}
VM_FUNC_NEW(GetAIObjectCount, 128)
{
	int ai_level = vm_pop_int();
	int result = 0;
	if (ai_level <= 4)
	{
		result = server_internal->srv_ai->ai_objects[ai_level].objects.len;
	}
	vm_push_int(result);
}
	
}