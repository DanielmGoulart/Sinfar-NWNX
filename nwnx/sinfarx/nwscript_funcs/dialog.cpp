#include "nwscript_funcs.h"
#include "../ai.h"

namespace {

void (*StopDialog)(CNWSObject*) = (void (*)(CNWSObject*))0x081d0e8c;
VM_FUNC_NEW(StopDialog, 215)
{
	uint32_t object_id = vm_pop_object();
	ai::add_pending_x_event([=]{
		CNWSObject* object = GetObjectById(object_id);
		if (object)
		{
			StopDialog(object);
		}
	});
}
VM_FUNC_NEW(InstantlyPauseConversation, 197)
{
	CNWSObject* object = GetObjectById(vm_pop_object());
	int pause = vm_pop_int();
	if (object)
	{
		object->obj_conv_paused = pause;
	}
}
VM_FUNC_NEW(SetConversationInterruptable, 206)
{
	CNWSCreature* creature = GetCreatureById(vm_pop_object());
	int value = vm_pop_int();
	if (creature)
	{
		creature->cre_stats->cs_conv_interruptable = value;
	}
}
	
}