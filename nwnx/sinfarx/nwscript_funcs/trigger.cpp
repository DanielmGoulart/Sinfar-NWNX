#include "nwscript_funcs.h"

namespace {

VM_FUNC_NEW(SetTrapCreator, 492)
{
    CNWSTrigger* trigger = GetTriggerById(vm_pop_object());
    uint32_t creator_id = vm_pop_object();
    if (trigger && GetGameObject(creator_id))
    {
        trigger->tr_creator = creator_id;
    }
}

int (*ComputeBoundingBox)(CNWSTrigger*, float*, float*, float*, float*) =
	(int (*)(CNWSTrigger*, float*, float*, float*, float*))0x081f2a80;
VM_FUNC_NEW(GetTriggerBounds, 228)
{
	float f1=0, f2=2, f3=0, f4=0;
	CNWSTrigger* trigger = GetTriggerById(vm_pop_object());
	if (trigger)
	{
		ComputeBoundingBox(trigger, &f1, &f2, &f3, &f4);
	}
	CScriptArray result(new CScriptArrayData);
	result->push_back(CScriptVarValue(f1));
	result->push_back(CScriptVarValue(f2));
	result->push_back(CScriptVarValue(f3));
	result->push_back(CScriptVarValue(f4));
	vm_push_array(&result);
}

int (*GetTriggerHasPoint)(CNWSTrigger*, Vector) = (int (*)(CNWSTrigger*, Vector))0x081ef0a0;
VM_FUNC_NEW(GetTriggerHasPoint, 244)
{
	CNWSTrigger* trigger = GetTriggerById(vm_pop_object());
	Vector point = vm_pop_vector();
	vm_push_int(trigger ? GetTriggerHasPoint(trigger, point) : 0);
}

}
