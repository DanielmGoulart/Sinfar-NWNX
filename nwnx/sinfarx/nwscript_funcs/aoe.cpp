#include "nwscript_funcs.h"

namespace
{
	
int (*IsPointInAreaOfEffect)(CNWSAreaOfEffectObject*, Vector) = (int (*)(CNWSAreaOfEffectObject*, Vector))0x081eabf0;
VM_FUNC_NEW(IsPointInAreaOfEffect, 5)
{
	CNWSAreaOfEffectObject* aoe = GetAreaOfEffectObjectById(vm_pop_object());
	Vector point = vm_pop_vector();
	vm_push_int(aoe ? IsPointInAreaOfEffect(aoe, point) : false);
}

VM_FUNC_NEW(GetAreaOfEffectCasterLevel, 210)
{
	CNWSAreaOfEffectObject* aoe = GetAreaOfEffectObjectById(vm_pop_object());
	vm_push_int(aoe ? aoe->aoe_spell_level : 0);
}

VM_FUNC_NEW(GetAOESpellId, 235)
{
	CNWSAreaOfEffectObject* aoe = GetAreaOfEffectObjectById(vm_pop_object());
	vm_push_int(aoe ? aoe->aoe_spell_id : -1);
}	

}