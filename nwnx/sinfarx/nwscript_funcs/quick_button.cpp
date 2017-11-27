#include "nwscript_funcs.h"

namespace
{
CNWSQuickbarButton* vm_pop_quick_button()
{
	CNWSCreature* creature = GetCreatureById(vm_pop_object());
	uint32_t position = vm_pop_int();
	if (creature && creature->cre_quickbar && position < 36)
	{
		return creature->cre_quickbar+position;
	}
	return NULL;
}
VM_FUNC_NEW(GetQuickButtonType, 354)
{
	CNWSQuickbarButton* btn = vm_pop_quick_button();
	vm_push_int(btn?btn->qb_type:-1);
}
VM_FUNC_NEW(GetQuickButtonItem1, 355)
{
	CNWSQuickbarButton* btn = vm_pop_quick_button();
	vm_push_object(btn?btn->qb_objid1:OBJECT_INVALID);
}
VM_FUNC_NEW(GetQuickButtonItem2, 356)
{
	CNWSQuickbarButton* btn = vm_pop_quick_button();
	vm_push_object(btn?btn->qb_objid2:OBJECT_INVALID);
}
VM_FUNC_NEW(GetQuickButtonClass, 357)
{
	CNWSQuickbarButton* btn = vm_pop_quick_button();
	vm_push_int(btn?btn->qb_class:-1);
}
VM_FUNC_NEW(GetQuickButtonMetaMagic, 358)
{
	CNWSQuickbarButton* btn = vm_pop_quick_button();
	vm_push_int(btn?btn->qb_metamagic:-1);
}
VM_FUNC_NEW(GetQuickButtonSpellId, 359)
{
	CNWSQuickbarButton* btn = vm_pop_quick_button();
	vm_push_int(btn?btn->qb_id:-1);
}
VM_FUNC_NEW(GetQuickButtonCommand, 360)
{
	CNWSQuickbarButton* btn = vm_pop_quick_button();
	CExoString result(btn?btn->qb_command:"");
	vm_push_string(&result);
}
VM_FUNC_NEW(GetQuickButtonCommandLabel, 361)
{
	CNWSQuickbarButton* btn = vm_pop_quick_button();
	CExoString result(btn?btn->qb_label:"");
	vm_push_string(&result);
}
VM_FUNC_NEW(GetQuickButtonCreatorResRef, 362)
{
	CNWSQuickbarButton* btn = vm_pop_quick_button();
	CExoString result(btn?btn->qb_resref:"");
	vm_push_string(&result);
}
VM_FUNC_NEW(GetQuickButtonCreatorLabel, 363)
{
	CNWSQuickbarButton* btn = vm_pop_quick_button();
	CExoString result(btn?btn->qb_label2:"");
	vm_push_string(&result);
}
VM_FUNC_NEW(GetQuickButtonAssociateType, 364)
{
	CNWSQuickbarButton* btn = vm_pop_quick_button();
	vm_push_int(btn?btn->qb_associate_type:-1);
}
VM_FUNC_NEW(GetQuickButtonAssociate, 365)
{
	CNWSQuickbarButton* btn = vm_pop_quick_button();
	vm_push_object(btn?btn->qb_associate:OBJECT_INVALID);
}
int (*UpdateQuickbarButton)(CNWSMessage* message, CNWSPlayer*, uint8_t, int) = (int (*)(CNWSMessage* message, CNWSPlayer*, uint8_t, int))0x807c794;
VM_FUNC_NEW(SetQuickButton, 366)
{
	CNWSQuickbarButton* btn = NULL;
	CNWSCreature* creature = GetCreatureById(vm_pop_object());
	uint32_t position = vm_pop_int();
	if (creature && creature->cre_quickbar && position < 36)
	{
		btn = creature->cre_quickbar+position;
	}
	int type = vm_pop_int();
	uint32_t item1 = vm_pop_object();
	uint32_t item2 = vm_pop_object();
	uint32_t qb_class = vm_pop_int();
	uint32_t metamagic = vm_pop_int();
	uint32_t spellid = vm_pop_int();
	CExoString command = vm_pop_string();
	CExoString command_label = vm_pop_string();
	std::string creator_resref = vm_pop_string();
	CExoString creator_label = vm_pop_string();
	int associate_type = vm_pop_int();
	uint32_t associate = vm_pop_object();
	if (btn)
	{
		btn->qb_type = type;
		btn->qb_objid1 = item1;
		btn->qb_objid2 = item2;
		btn->qb_class = qb_class;
		btn->qb_metamagic = metamagic;
		btn->qb_id = spellid;
		btn->qb_command = command;
		btn->qb_label = command_label;
		strncpy(btn->qb_resref, creator_resref.c_str(), 16);
		btn->qb_resref[16] = 0;
		btn->qb_label2 = creator_label;
		btn->qb_associate_type = associate_type;
		btn->qb_associate = associate;
		CNWSPlayer* player = get_player_by_game_object_id(creature->obj.obj_id);
		if (player)
		{
			UpdateQuickbarButton(server_internal->srv_client_messages, player, position, 0);
		}
	}
}
}