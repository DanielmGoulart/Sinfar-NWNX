#include "core.h"
#include "creature.h"

#include <sys/time.h>

using namespace nwnx::core;
using namespace nwnx::creature;

namespace nwnx { namespace pathfinding {

unsigned char d_ret_code_walk[0x20];
int (*Walk_Org)(CNWSCreature*) = (int (*)(CNWSCreature*))&d_ret_code_walk;
int Walk_HookProc(CNWSCreature* creature)
{
	CREATURE_EXTRA* creature_extra = GetCreatureExtra(creature);
	creature_extra->DetachAttachedTo();

	CExoLinkedList* actions_list = &creature->obj.obj_actions;
	int nActionCount = actions_list->header->len;
	CExoLinkedListNode* action_node = actions_list->header->first;
	int nAction;
	for (nAction=0; nAction<nActionCount; nAction++)
	{
		CNWSAction* action = (CNWSAction*)action_node->data;
		int nActionType = action->act_type;
		if (nActionType == 12 || nActionType == 15 || nActionType == 46)
		{
			break;
		}
		action_node = action_node->next;
	}
	if (nAction == nActionCount)
	{
		gettimeofday(&(creature_extra->lastmove), NULL);
	}

	return Walk_Org(creature);
}

unsigned char d_ret_code_drive[0x20];
int (*Drive_Org)(CNWSCreature*, int) = (int (*)(CNWSCreature*, int))&d_ret_code_drive;
int Drive_HookProc(CNWSCreature* creature, int p2)
{
	CREATURE_EXTRA* creature_extra = GetCreatureExtra(creature);
	creature_extra->DetachAttachedTo();

	gettimeofday(&(creature_extra->lastmove), NULL);

	return Drive_Org(creature, p2);
}

void OnAddPlcToArea_BudgeCreatures(CNWSArea* area, Vector* v1, Vector* v2, Vector* v3, uint32_t p4, int p5)
{
}

void init()
{
	hook_function(0x08104174, (unsigned long)Walk_HookProc, d_ret_code_walk, 12);
	hook_function(0x08106580, (unsigned long)Drive_HookProc, d_ret_code_drive, 12);
    
    //do not bump creatures for a placeable
	hook_call(0x081df577, (uint32_t)OnAddPlcToArea_BudgeCreatures);
}
REGISTER_INIT(init);
 
}
}