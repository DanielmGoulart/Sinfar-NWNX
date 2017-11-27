#include "nwscript_funcs.h"

namespace {

VM_FUNC_NEW(GetDoorAppearance, 216)
{
	CNWSDoor* door = GetDoorById(vm_pop_object());
	vm_push_int(door ? door->door_appearance : -1);
}
VM_FUNC_NEW(SetDoorAppearance, 217)
{
	CNWSDoor* door = GetDoorById(vm_pop_object());
	uint32_t appearance = vm_pop_int();
	if (door)
	{
		door->door_appearance = appearance;
	}
}

uint8_t (*GetDoorOpenState)(CNWSDoor*) = (uint8_t (*)(CNWSDoor*))0x0816A908;
VM_FUNC_NEW(GetDoorOpenState, 195)
{
	CNWSDoor* door = GetDoorById(vm_pop_object());
	int result = -1;
	if (door)
	{
		result = GetDoorOpenState(door);
	}
	vm_push_int(result);
}
void (*SetDoorOpenState)(CNWSDoor*, uint8_t) = (void (*)(CNWSDoor*, uint8_t))0x0816A5B8;
VM_FUNC_NEW(SetDoorOpenState, 196)
{
	CNWSDoor* door = GetDoorById(vm_pop_object());
	int state = vm_pop_int();
	if (door)
	{
		SetDoorOpenState(door, state);
	}
}


void (*SetDoorLinkedFlags)(CNWSDoor*, uint8_t) = (void (*)(CNWSDoor*, uint8_t))0x0816b17c;
VM_FUNC_NEW(RemoveDoorLink, 211)
{
	CNWSDoor* door = GetDoorById(vm_pop_object());
	if (door)
	{
		SetDoorLinkedFlags(door, 0);
	}
}
void (*SetDoorLinkedToTag)(CNWSDoor*, CExoString) = (void (*)(CNWSDoor*, CExoString))0x0816b1b8;
void (*SetTriggerLinkedFlags)(CNWSTrigger*, uint8_t) = (void (*)(CNWSTrigger*, uint8_t))0x081f325c;
void (*SetTriggerLinkedToTag)(CNWSTrigger*, CExoString) = (void (*)(CNWSTrigger*, CExoString))0x081f2f20;
VM_FUNC_NEW(SetLinkedToTag, 245)
{
	CGameObject* o = GetGameObject(vm_pop_object());
	int flags = vm_pop_int();
	CExoString tag = vm_pop_string();
	if (o)
	{
		if (o->type == OBJECT_TYPE_DOOR)
		{
			CNWSDoor* door = o->vtable->AsNWSDoor(o);
			SetDoorLinkedFlags(door, flags);
			SetDoorLinkedToTag(door, tag);
			door->door_target_obj = OBJECT_INVALID;
		}
		else if (o->type == OBJECT_TYPE_TRIGGER)
		{
			CNWSTrigger* trigger = o->vtable->AsNWSTrigger(o);
			SetTriggerLinkedFlags(trigger, flags);
			SetTriggerLinkedToTag(trigger, tag);
		}
	}
}
VM_FUNC_NEW(SetDoorLinkedTo, 246)
{
	CNWSDoor* door = GetDoorById(vm_pop_object());
	uint32_t target_id = vm_pop_object();
	if (door)
	{
		CGameObject* target = GetGameObject(target_id);
		if (target)
		{
			if (target->type == 10)
			{
				SetDoorLinkedFlags(door, 1);
			}
			else
			{
				SetDoorLinkedFlags(door, 2);
			}
			door->door_target_obj = target_id;
		}
		else
		{
			door->door_target_obj = OBJECT_INVALID;
		}
	}
}
	
}