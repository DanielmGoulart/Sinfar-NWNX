#include "object.h"
#include "nwscript.h"

using namespace nwnx::core;
using namespace nwnx::nwscript;

namespace nwnx { namespace object {
	
uint32_t clear_spells_effects_ignore_object_id = OBJECT_INVALID;
CGameObject* OnClearSpellsEffects_GetGameObject(CServerExoApp* server, uint32_t object_id)
{
	if (object_id == clear_spells_effects_ignore_object_id) return NULL;
	return GetGameObject(object_id);
}

unsigned char d_ret_code_clearallactions[0x20];
int (*ClearAllActions_Org)(CNWSObject*) = (int (*)(CNWSObject*))&d_ret_code_clearallactions;
int ClearAllActions_HookProc(CNWSObject* object)
{
	if (get_local_string(object, "EVENT_CLEARALLACTIONS"))
	{
		run_script("inc_ev_caactions", object->obj_id);
	}
	return ClearAllActions_Org(object);
}
	
void init()
{
	hook_function(0x081cb77c, (unsigned long)ClearAllActions_HookProc, d_ret_code_clearallactions, 12);
	
	hook_call(0x081CFF87, (uint32_t)OnClearSpellsEffects_GetGameObject);
}
REGISTER_INIT(init);

void (*ClearSpellsEffectsOnOthers)(CNWSObject*) = (void (*)(CNWSObject*))0x081cff48;
VM_FUNC_NEW(ClearMySpellsEffectsOnOthers, 225)
{
	CNWSObject* object = GetObjectById(vm_pop_object());
	if (object)
	{
		clear_spells_effects_ignore_object_id = object->obj_id;
		ClearSpellsEffectsOnOthers(object);
		clear_spells_effects_ignore_object_id = OBJECT_INVALID;
	}
}
	
}
}