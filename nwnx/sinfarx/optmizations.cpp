#include "core.h"
#include "creature.h"

using namespace nwnx::core;
using namespace nwnx::creature;

namespace nwnx { namespace optmizations {

int HasExpansionPack_HookProc(CNWSPlayer* player, uint8_t expansion_pack, int p3)
{
	return true;
}

void RunScriptCallback_HookProc(CNWVirtualMachineCommands* vm_cmd, CExoString* script_name)
{
	//do nothing, this function is kinda useless
}

unsigned char d_ret_code_setdlgdelay[0x20];
float (*Dialog_SetDelay_Org)(CNWSDialog*, CNWSObject*, CExoLocString*, uint32_t, int) = (float (*)(CNWSDialog*, CNWSObject*, CExoLocString*, uint32_t, int))&d_ret_code_setdlgdelay;
float Dialog_SetDelay_HookProc(CNWSDialog* dialog, CNWSObject* object, CExoLocString* p3, uint32_t p4, int p5)
{
	if (object->obj_type == 9 ||  //if placeable
		dialog->oid_self == dialog->oid_with ||
		get_local_int(object, "NO_DIALOG_DELAY"))
	{
		CExoLocString_Destructor(p3, 2);
		return 0;
	}
	return Dialog_SetDelay_Org(dialog, object, p3, p4, p5);
}

unsigned char d_ret_code_getmaxhitpoint[0x20];
int (*GetCreatureMaxHitPoint_Org)(CNWSCreature*, int) = (int (*)(CNWSCreature*, int))&d_ret_code_getmaxhitpoint;
int GetCreatureMaxHitPoint_HookProc(CNWSCreature* creature, int p2)
{
	if (creature->obj.obj_id>OBJECT_INVALID && creature->cre_stats->cs_feats.len==0)
	{
		return GetCreatureMaxHitPoint_Org(creature, p2);
	}
	if (p2)
	{
		CREATURE_EXTRA* creature_extra = GetCreatureExtra(creature);
		if (creature_extra->last_max_hp == creature->obj.obj_hp_max &&
			creature->cre_stats->cs_con_mod == creature_extra->last_con_mod)
		{
			return creature_extra->last_real_max_hp;
		}
		else
		{
			int nMaxNewMaxHP = GetCreatureMaxHitPoint_Org(creature, p2);
			creature_extra->last_max_hp = creature->obj.obj_hp_max;
			creature_extra->last_con_mod = creature->cre_stats->cs_con_mod;
			creature_extra->last_real_max_hp = nMaxNewMaxHP;
			return nMaxNewMaxHP;
		}
	}
	else
	{
		return GetCreatureMaxHitPoint_Org(creature, p2);
	}
}

void init()
{
	hook_function(0x0805ea80, (long)HasExpansionPack_HookProc, d_ret_code_nouse, 10);  

	hook_function(0x081fb558, (long)RunScriptCallback_HookProc, d_ret_code_nouse, 9);

	hook_function(0x0823cd70, (long)Dialog_SetDelay_HookProc, d_ret_code_setdlgdelay, 12);

	hook_function(0x0812e25c, (unsigned long)GetCreatureMaxHitPoint_HookProc, d_ret_code_getmaxhitpoint, 9);
}
REGISTER_INIT(init);
 
}
}