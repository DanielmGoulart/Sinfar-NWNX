#include "core.h"
#include "creature.h"
#include "nwscript.h"
#include "player.h"

using namespace nwnx::core;
using namespace nwnx::creature;
using namespace nwnx::player;
using namespace nwnx::nwscript;

namespace nwnx { namespace possess {

unsigned char d_ret_code_rest[0x20];
void (*CreatureRest_Org)(CNWSCreature*, int, int) = (void (*)(CNWSCreature*, int, int))&d_ret_code_rest;
void CreatureRest_HookProc(CNWSCreature* creature, int p2, int p3)
{
	if (creature->obj.obj_id < OBJECT_INVALID && get_player_by_game_object_id(creature->obj.obj_id))
	{
		return;
	}
	return CreatureRest_Org(creature, p2, p3);
}

unsigned char d_ret_code_barter[0x20];
int (*StartBarter_Org)(CNWSCreature*, uint32_t, uint32_t, int) = (int (*)(CNWSCreature*, uint32_t, uint32_t, int))&d_ret_code_barter;
int StartBarter_HookProc(CNWSCreature* source, uint32_t target_id, uint32_t p3, int p4)
{
	CNWSCreature* target = GetCreatureById(target_id);
	if (target == NULL ||
		target_id <= OBJECT_INVALID ||
		source->obj.obj_id <= OBJECT_INVALID ||
		get_player_by_game_object_id(target_id) == NULL ||
		get_player_by_game_object_id(source->obj.obj_id) == NULL)
	{
		*(uint8_t*)0x0812fca9 = 0xEB;
	}
	else
	{
		*(uint8_t*)0x0812fca9 = 0x75;
	}

	return StartBarter_Org(source, target_id, p3, p4);
}

CNWSCreature* OnCheckEncounter_GetCreatureById(CServerExoApp* server, uint32_t object_id)
{
	CNWSCreature* creature = GetCreatureById(object_id);
	if (creature)
	{
		if (creature->cre_stats->cs_is_pc || get_player_by_game_object_id(object_id))
		{
			*(uint16_t*)0x081804d7 = 0x9090;
		}
		else
		{
			*(uint16_t*)0x081804d7 = 0x1174;
		}
	}
	return creature;
}

int (*GetDead)(CNWSObject*) = (int (*)(CNWSObject*))0x081d1c10;
bool ignore_next_is_dead_check = false;
int OnPossessCreatureGetDead_HookProc(CNWSObject* object)
{
	if (ignore_next_is_dead_check)
	{
		return false;
	}
	else
	{
		return GetDead(object);
	}
}
unsigned char d_ret_code_possessfamiliar[0x20];
void (*PossessFamiliar_Org)(CNWSCreature*) = (void (*)(CNWSCreature*))&d_ret_code_possessfamiliar;
void PossessFamiliar_HookProc(CNWSCreature* creature)
{
	if (get_local_object((CNWSObject*)creature, "POSSESSED_BY") != OBJECT_INVALID) return;

	return PossessFamiliar_Org(creature);
}
unsigned char d_ret_code_rmassociate[0x20];
void (*RemoveAssociate_Org)(CNWSCreature*, uint32_t) = (void (*)(CNWSCreature*, uint32_t))&d_ret_code_rmassociate;
void RemoveAssociate_HookProc(CNWSCreature* creature, uint32_t nAssociateId)
{
	if (get_local_object((CNWSObject*)creature, "UNPOSSESSED_BY") == nAssociateId)
	{
		run_script("possess_stop", nAssociateId);
	}
	return RemoveAssociate_Org(creature, nAssociateId);
}

unsigned char d_ret_code_aidialog[0x20];
int (*AIDialog_Org)(CNWSObject*, CNWSAction*) = (int (*)(CNWSObject*, CNWSAction*))&d_ret_code_aidialog;
int AIDialog_HookProc(CNWSObject* object, CNWSAction* ai_action)
{
	if (object->obj_type == OBJECT_TYPE_CREATURE)
	{
		GetCreatureExtra((CNWSCreature*)object)->last_dialog_reply_time = time(NULL);
	}

	uint32_t target_id = *(uint32_t*)(((char*)ai_action)+0x34);

	if (target_id == object->obj_id)
	{
		//remove the animation
		*(uint8_t*)0x081c369a = 0xEB;

		//let familar talk to themselves
		*(uint8_t*)0x081C323A = 0xEB;
	}
	else
	{
		CGameObject* o_target = GetGameObject(target_id);
		if (o_target)
		{
			CNWSObject* target = o_target->vtable->AsNWSObject(o_target);
			if (target)
			{
				if (target->obj_dialog && target->obj_dialog->oid_with != object->obj_id)
				{
					run_script("inc_ev_dblconv", target_id);
				}
			}
		}

		if (get_player_by_game_object_id(object->obj_id) != NULL &&
			get_player_by_game_object_id(target_id) != NULL)
		{
			return CNWSObject_ACTION_COMPLETE;
		}

		if (get_local_object(object, "POSSESSED_BY") != OBJECT_INVALID)
		{
			run_script("possess_noconv", object->obj_id);
			return CNWSObject_ACTION_COMPLETE;
		}

		*(uint8_t*)0x081c369a = 0x74;
		*(uint8_t*)0x081C323A = 0x75;
	}

	return AIDialog_Org(object, ai_action);
}
int OnGetIsPossessedFamiliar_ReturnFalse(CNWSCreature* creature)
{
	return false;
}
uint32_t (*GetAssociateId)(CNWSCreature*, uint16_t, int) = (uint32_t (*)(CNWSCreature*, uint16_t, int))0x0810F9B4;
int (*GetIsPossessedFamiliar)(CNWSCreature*) = (int (*)(CNWSCreature*))0x08113574;
uint32_t OnRunOnConvScript_GetAssociateId(CNWSCreature* creature, uint16_t associate_type, int p3)
{
	uint32_t ret = GetAssociateId(creature, associate_type, p3);
	if (GetIsPossessedFamiliar(creature))
	{
		*(uint8_t*)0x0811D166 = 0xEB;
		*(uint32_t*)0x0811D195 = ((uint32_t)OnGetIsPossessedFamiliar_ReturnFalse-(uint32_t)0x0811D199);
	}
	else
	{
		*(uint8_t*)0x0811D166 = 0x75;
		*(uint32_t*)0x0811D195 = 0xffff63db;
	}
	return ret;
}
unsigned char d_ret_code_aipick[0x20];
int (*AIPickUpItem_Org)(CNWSCreature*, CNWSAction*) = (int (*)(CNWSCreature*, CNWSAction*))&d_ret_code_aipick;
int AIPickUpItem_HookProc(CNWSCreature* creature, CNWSAction* ai_action)
{
	if (get_local_object((CNWSObject*)creature, "POSSESSED_BY") != OBJECT_INVALID)
	{
		run_script("possess_nopick", creature->obj.obj_id);
		return CNWSObject_ACTION_COMPLETE;
	}

	CGameObject* o = GetGameObject(*(uint32_t*)(((char*)(creature->obj.obj_ai_action))+0x34));
	CNWSObject* object = NULL;
	if (o != NULL)
	{
		object = o->vtable->AsNWSObject(o);
	}
	if (object != NULL && get_local_int(object, "NO_PICKUP"))
	{
		send_message_to_pc(creature, "You can not pick up this item.");
		return CNWSObject_ACTION_COMPLETE;
	}

	return AIPickUpItem_Org(creature, ai_action);
}
unsigned char d_ret_code_aidrop[0x20];
int (*AIDropItem_Org)(CNWSCreature*, CNWSAction*) = (int (*)(CNWSCreature*, CNWSAction*))&d_ret_code_aidrop;
int AIDropItem_HookProc(CNWSCreature* creature, CNWSAction* ai_action)
{
	if (get_local_object((CNWSObject*)creature, "POSSESSED_BY") != OBJECT_INVALID)
	{
		run_script("possess_nodrop", creature->obj.obj_id);
		return CNWSObject_ACTION_COMPLETE;
	}

	return AIDropItem_Org(creature, ai_action);
}

void init()
{
	//possessed creatures trigger encounters too
	hook_call(0x081803b0, (uint32_t)OnCheckEncounter_GetCreatureById);
	enable_write(0x081804d7);
	
	//barder only between possessed player characters
	hook_function(0x0812fc6c, (unsigned long)StartBarter_HookProc, d_ret_code_barter, 12);
	
	//no rest for possessed creatures
	hook_function(0x0812d45c, (unsigned long)CreatureRest_HookProc, d_ret_code_rest, 12);

	//possessing a creature is possible when dead
	hook_call(0x0810e695, (long)OnPossessCreatureGetDead_HookProc);
	
	//hook_function(0x0810de48, (unsigned long)PossessFamiliar_HookProc, d_ret_code_possessfamiliar, 11);
	hook_function(0x0810c8f4, (unsigned long)RemoveAssociate_HookProc, d_ret_code_rmassociate, 9);
	//illegal ai for possessed creature
	hook_function(0x081c3204, (unsigned long)AIDialog_HookProc, d_ret_code_aidialog, 12);
	enable_write(0x081C323A);
	enable_write(0x081c369a);
	hook_call(0x0811D058, (uint32_t)OnRunOnConvScript_GetAssociateId);
	enable_write(0x0811D195);
	enable_write(0x0811D166);
	hook_function(0x08106bb0, (unsigned long)AIPickUpItem_HookProc, d_ret_code_aipick, 12);
	hook_function(0x0810735c, (unsigned long)AIDropItem_HookProc, d_ret_code_aidrop, 12);	
}
REGISTER_INIT(init);

void (*SendServerToPlayerClientArea)(CNWSMessage*, CNWSPlayer*, CNWSArea*, float, float, float, const Vector*, int32_t) =
	(void (*)(CNWSMessage*, CNWSPlayer*, CNWSArea*, float, float, float, const Vector*, int32_t))0x0806504c;
int (*PossessCreature)(CNWSCreature*, uint32_t) = (int (*)(CNWSCreature*, uint32_t))0x0810e658;
VM_FUNC_NEW(SetPCCreature, 232)
{
	CNWSCreature* creature = GetCreatureById(vm_pop_object());
	CNWSCreature* target = GetCreatureById(vm_pop_object());
	if (creature && target)
	{
		CNWSPlayer* creature_player = get_player_by_game_object_id(creature->obj.obj_id);
		if (creature_player)
		{
			ignore_next_is_dead_check = true;
			PossessCreature(creature, target->obj.obj_id);
			ignore_next_is_dead_check = false;
			if (creature->obj.obj_area_id != target->obj.obj_area_id)
			{
				CNWSArea* target_area = GetAreaById(target->obj.obj_area_id);
				CNWSArea* source_area =	GetAreaById(creature->obj.obj_area_id);
				if (target_area && source_area)
				{
					target->cre_desired_area = target->obj.obj_area_id;
					target->cre_desired_pos = target->obj.obj_position;
					target->cre_desired_complete = false;
					SendServerToPlayerClientArea(get_nws_message(), creature_player, target_area, target->obj.obj_position.x, target->obj.obj_position.y, target->obj.obj_position.z, &(target->obj.obj_orientation), 0);
				}
			}
		}
	}
}
	
}
}