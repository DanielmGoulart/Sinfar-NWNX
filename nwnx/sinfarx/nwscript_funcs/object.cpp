#include "nwscript_funcs.h"

namespace {

VM_FUNC_REPLACE(GetObjectType, 106)
{
	CGameObject* o = GetGameObject(vm_pop_object());
	int result = 0x7fff;
	if (o)
	{
		switch (o->type)
		{
			case OBJECT_TYPE_CREATURE: 			result = 0x0001; break;
			case OBJECT_TYPE_ITEM: 				result = 0x0002; break;
			case OBJECT_TYPE_TRIGGER: 			result = 0x0004; break;
			case OBJECT_TYPE_DOOR: 				result = 0x0008; break;
			case OBJECT_TYPE_AREA_OF_EFFECT: 	result = 0x0010; break;
			case OBJECT_TYPE_WAYPOINT: 			result = 0x0020; break;
			case OBJECT_TYPE_PLACEABLE: 		result = 0x0040; break;
			case OBJECT_TYPE_STORE: 			result = 0x0080; break;
			case OBJECT_TYPE_ENCOUNTER: 		result = 0x0100; break;
			case OBJECT_TYPE_SOUND: 		 	result = 0x0200; break;
			case OBJECT_TYPE_AREA: 				result = 0x0400; break;
			case OBJECT_TYPE_MODULE: 			result = 0x0800; break;
		}
	}
	vm_push_int(result);
}

char (*GetDamageImmunityByFlags)(CNWSObject*, uint16_t) = (char (*)(CNWSObject*, uint16_t))0x081cdea4;
VM_FUNC_NEW(GetDamageImmunity, 427)
{
	CNWSObject* object = GetObjectById(vm_pop_object());
	int damage_type = vm_pop_int();
	int result = 0;
	if (object)
	{
		result = GetDamageImmunityByFlags(object, damage_type);
	}
	vm_push_int(result);
}

VM_FUNC_NEW(SetCurrentHitPoints, 457)
{
    CNWSObject* object = GetObjectById(vm_pop_object());
    int value = vm_pop_int();
    if (object)
    {
        object->obj_hp_cur = value;
    }
}
VM_FUNC_NEW(SetMaxHitPoints, 458)
{
    CNWSObject* object = GetObjectById(vm_pop_object());
    int value = vm_pop_int();
    if (object)
    {
        object->obj_hp_cur = value;
        object->obj_hp_max = value;
    }
}

int (*GetFactionOfObject)(CServerExoAppInternal*, uint32_t, int*) = (int (*)(CServerExoAppInternal*, uint32_t, int*))0x080ac4d4;
VM_FUNC_NEW(GetFactionId, 510)
{
	int faction_id = 0;
	GetFactionOfObject((*p_app_manager)->app_server->srv_internal, vm_pop_object(), &faction_id);
	vm_push_int(faction_id);
}
void (*AddFactionMember)(CNWSFaction*, uint32_t, int) = (void (*)(CNWSFaction*, uint32_t, int))0x081d6990;
CNWSFaction* (*GetFaction)(CFactionManager*, int) = (CNWSFaction* (*)(CFactionManager*, int))0x080ba9a0;
VM_FUNC_NEW(SetFactionId, 511)
{
	uint32_t object_id = vm_pop_object();
	CNWSFaction* faction = GetFaction((*p_app_manager)->app_server->srv_internal->srv_factions, vm_pop_int());
	if (faction)
	{
		AddFactionMember(faction, object_id, 0);
	}
}

VM_FUNC_NEW(SetLastHostileActor, 512)
{
    CNWSObject* object = GetObjectById(vm_pop_object());
	uint32_t actor = vm_pop_object();
	if (object)
	{
		object->obj_last_hostile_actor = actor;
	}
}

VM_FUNC_NEW(GetCurrentAnimation, 519)
{
    CNWSObject* object = GetObjectById(vm_pop_object());
	vm_push_int(object ? object->obj_anim : -1);
}
VM_FUNC_NEW(SetCurrentAnimation, 520)
{
    CNWSObject* object = GetObjectById(vm_pop_object());
	int anim = vm_pop_int();
	if (object)
	{
		object->obj_anim = anim;
	}
}

void (*ClearSpellsEffectsOnOthers)(CNWSObject*) = (void (*)(CNWSObject*))0x081cff48;
VM_FUNC_NEW(ClearMySpellsEffects, 224)
{
	CNWSObject* object = GetObjectById(vm_pop_object());
	if (object)
	{
		ClearSpellsEffectsOnOthers(object);
	}
}

void (*ClearAllActions)(CNWSObject*) = (void (*)(CNWSObject*))0x081cb77c;
VM_FUNC_NEW(ClearAllActions, 222)
{
	CNWSObject* object = GetObjectById(vm_pop_object());
	if (object)
	{
		ClearAllActions(object);
	}
}

int (*SetPosition)(CNWSObject*, Vector, int) = (int (*)(CNWSObject*, Vector, int))0x081d4e30;
void (*UpdateSubAreasOnJump)(CNWSCreature*, Vector, uint32_t) = (void (*)(CNWSCreature*, Vector, uint32_t))0x081037cc;
void (*AddCreatureToArea)(CNWSCreature*, CNWSArea*, float, float, float, int) = (void (*)(CNWSCreature*, CNWSArea*, float, float, float, int))0x08118d20;
void (*AddPlaceableToArea)(CNWSPlaceable*, CNWSArea*, float, float, float, int) = (void (*)(CNWSPlaceable*, CNWSArea*, float, float, float, int))0x081df174;
VM_FUNC_NEW(SetLocation, 174)
{
	uint32_t object_id = vm_pop_object();
	CScriptLocation location = vm_pop_location();
	CNWSArea* area = GetAreaById(location.loc_area);
	if (area)
	{
		CGameObject* o = GetGameObject(object_id);
		if (o)
		{
			if (o->type == OBJECT_TYPE_CREATURE)
			{
				CNWSCreature* creature = o->vtable->AsNWSCreature(o);
				if (creature)
				{
					ClearAllActions((CNWSObject*)creature);
					if (GetAreaById(creature->obj.obj_area_id))
					{
						UpdateSubAreasOnJump(creature, creature->obj.obj_position, OBJECT_INVALID);
					}
					AddCreatureToArea(creature, area, location.loc_position.x, location.loc_position.y, location.loc_position.z, 0);
				}
			}
			else if (o->type == OBJECT_TYPE_PLACEABLE)
			{
				CNWSPlaceable* placeable = o->vtable->AsNWSPlaceable(o);
				if (placeable)
				{
					AddPlaceableToArea(placeable, area, location.loc_position.x, location.loc_position.y, location.loc_position.z, 0);
				}
			}
		}
	}
}
void* (*GetTile)(CNWSArea*, Vector) = (void* (*)(CNWSArea*, Vector))0x080cdcc8;
VM_FUNC_NEW(SetPosition, 175)
{
	uint32_t object_id = vm_pop_object();
	Vector v = vm_pop_vector();
	CGameObject* o = GetGameObject(object_id);
	if (o)
	{
		CNWSObject* object = o->vtable->AsNWSObject(o);
		if (object)
		{
			CNWSArea* area = GetAreaById(object->obj_area_id);
			if (area && GetTile(area, v))
			{
				switch (o->type)
				{
					case OBJECT_TYPE_CREATURE:
						UpdateSubAreasOnJump((CNWSCreature*)object, object->obj_position, object->obj_area_id);
						SetPosition(object, v, 0);
						UpdateSubAreasOnJump((CNWSCreature*)object, object->obj_position, object->obj_area_id);
						break;
					case OBJECT_TYPE_PLACEABLE:
					{
						CNWSPlaceable* placeable = o->vtable->AsNWSPlaceable(o);
						if (placeable)
						{
							AddPlaceableToArea(placeable, area, v.x, v.y, v.z, 0);
						}
						break;
					}
					default:
						SetPosition(object, v, 0);
				}
			}
		}
	}
}

int (*SetCreatureScript)(CNWSCreature*, int, CExoString) = (int (*)(CNWSCreature*, int, CExoString))0x0813d720;
CExoString* (*GetCreatureScript)(CNWSCreature*, int) = (CExoString* (*)(CNWSCreature*, int))0x0813d704;
int (*SetPlaceableScript)(CNWSPlaceable*, int, CExoString) = (int (*)(CNWSPlaceable*, int, CExoString))0x081e23e4;
CExoString* (*GetPlaceableScript)(CNWSPlaceable*, int) = (CExoString* (*)(CNWSPlaceable*, int))0x081e23c8;
int (*SetDoorScript)(CNWSDoor*, int, CExoString) = (int (*)(CNWSDoor*, int, CExoString))0x0816af58;
CExoString* (*GetDoorScript)(CNWSDoor*, int) = (CExoString* (*)(CNWSDoor*, int))0x0816afb8;
int (*SetTriggerScript)(CNWSTrigger*, int, CExoString) = (int (*)(CNWSTrigger*, int, CExoString))0x081f2df4;
CExoString* (*GetTriggerScript)(CNWSTrigger*, int) = (CExoString* (*)(CNWSTrigger*, int))0x081f2cdc;
int (*SetAreaScript)(CNWSArea*, int, CExoString) = (int (*)(CNWSArea*, int, CExoString))0x080d55e8;
CExoString* (*GetAreaScript)(CNWSArea*, int) = (CExoString* (*)(CNWSArea*, int))0x080d55cc;
int (*SetStoreScript)(CNWSStore*, int, CExoString) = (int (*)(CNWSStore*, int, CExoString))0x0808614c;
CExoString* (*GetStoreScript)(CNWSStore*, int) = (CExoString* (*)(CNWSStore*, int))0x080861b4;
int (*SetModuleScript)(CNWSModule*, int, CExoString) = (int (*)(CNWSModule*, int, CExoString))0x081c106c;
CExoString* (*GetModuleScript)(CNWSModule*, int) = (CExoString* (*)(CNWSModule*, int))0x081c1050;
int (*SetEncounterScript)(CNWSEncounter*, int, CExoString) = (int (*)(CNWSEncounter*, int, CExoString))0x081845e4;
CExoString* (*GetEncounterScript)(CNWSEncounter*, int) = (CExoString* (*)(CNWSEncounter*, int))0x081845c8;
int (*SetAOEScript)(CNWSAreaOfEffectObject*, int, CExoString) = (int (*)(CNWSAreaOfEffectObject*, int, CExoString))0x081ece34;
CExoString* (*GetAOEScript)(CNWSAreaOfEffectObject*, int) = (CExoString* (*)(CNWSAreaOfEffectObject*, int))0x081ece18;
VM_FUNC_NEW(SetScript, 133)
{
	CGameObject* o = GetGameObject(vm_pop_object());
	int script_index = vm_pop_int();
	CExoString script_name = vm_pop_string();
	if (o)
	{
		switch (o->type)
		{
			case 5: SetCreatureScript(o->vtable->AsNWSCreature(o), script_index, script_name); break;
			case 9: SetPlaceableScript(o->vtable->AsNWSPlaceable(o), script_index, script_name); break;
			case 3: SetModuleScript(o->vtable->AsNWSModule(o), script_index, script_name); break;
			case 4: SetAreaScript(o->vtable->AsNWSArea(o), script_index, script_name); break;
			case 10: SetDoorScript(o->vtable->AsNWSDoor(o), script_index, script_name); break;
			case 14: SetStoreScript(o->vtable->AsNWSStore(o), script_index, script_name); break;
			case 7: SetTriggerScript(o->vtable->AsNWSTrigger(o), script_index, script_name); break;
			case 13: SetEncounterScript(o->vtable->AsNWSEncounter(o), script_index, script_name); break;
			case OBJECT_TYPE_AREA_OF_EFFECT: SetAOEScript(o->vtable->AsNWSAreaOfEffectObject(o), script_index, script_name); break;
		}
	}
}
VM_FUNC_NEW(GetScript, 134)
{
	CGameObject* o = GetGameObject(vm_pop_object());
	int script_index = vm_pop_int();
	CExoString* result = NULL;
	if (o)
	{
		switch (o->type)
		{
			case 5: result = GetCreatureScript(o->vtable->AsNWSCreature(o), script_index); break;
			case 9: result = GetPlaceableScript(o->vtable->AsNWSPlaceable(o), script_index); break;
			case 3: result = GetModuleScript(o->vtable->AsNWSModule(o), script_index); break;
			case 4: result = GetAreaScript(o->vtable->AsNWSArea(o), script_index); break;
			case 10: result = GetDoorScript(o->vtable->AsNWSDoor(o), script_index); break;
			case 14: result = GetStoreScript(o->vtable->AsNWSStore(o), script_index); break;
			case 7: result = GetTriggerScript(o->vtable->AsNWSTrigger(o), script_index); break;
			case 13: result = GetEncounterScript(o->vtable->AsNWSEncounter(o), script_index); break;
			case OBJECT_TYPE_AREA_OF_EFFECT: result = GetAOEScript(o->vtable->AsNWSAreaOfEffectObject(o), script_index); break;
		}
	}
	if (!result) result = &empty_cexostring;
	vm_push_string(result);
}

VM_FUNC_NEW(GetGameObjectByIndex, 292)
{
	uint32_t obj_index = vm_pop_int();
	cgoa_iter = cgoa.begin();
	uint32_t result = OBJECT_INVALID;
	for (uint32_t i=0; i<obj_index && cgoa_iter!=cgoa.end(); i++) cgoa_iter++;
	if (cgoa_iter != cgoa.end())
	{
		result = cgoa_iter->first;
	}
	vm_push_object(result);
}

VM_FUNC_NEW(GetDialogWith, 46)
{
    CNWSObject* object = GetObjectById(vm_pop_object());
	vm_push_object(object && object->obj_dialog ? object->obj_dialog->oid_with : OBJECT_INVALID);
}

VM_FUNC_NEW(GetGameObjectCount, 129)
{
	vm_push_int(cgoa.size());
}

VM_FUNC_NEW(SetTag, 176)
{
	uint32_t object_id = vm_pop_object();
	CExoString tag = vm_pop_string();
	CGameObject* o = GetGameObject(object_id);
	if (o)
	{
		CNWSObject* object = o->vtable->AsNWSObject(o);
		if (object)
		{
			remove_object_from_tags_map(object->obj_tag, object->obj_id);
			object->obj_tag = tag;
			safe_add_to_tags_map(object->obj_tag, object->obj_id);
		}
	}
}

void (*AddActionToFront)(CNWSObject*, uint32_t, uint16_t, uint32_t, void*, uint32_t, void*, uint32_t, void*, uint32_t, void*, uint32_t, void*,
	uint32_t, void*, uint32_t, void*, uint32_t, void*, uint32_t, void*, uint32_t, void*, uint32_t, void*, uint32_t, void*) =
		(void (*)(CNWSObject*, uint32_t, uint16_t, uint32_t, void*, uint32_t, void*, uint32_t, void*, uint32_t, void*, uint32_t, void*,
	uint32_t, void*, uint32_t, void*, uint32_t, void*, uint32_t, void*, uint32_t, void*, uint32_t, void*, uint32_t, void*))0x081ca5c0;
VM_FUNC_NEW(PlayAnimation, 220)
{
	uint32_t animation_type = vm_pop_int();
	float animation_speed = vm_pop_float();
	float animation_duration = vm_pop_float();
	CNWSObject* object = GetObjectById(vm_pop_object());
	if (object)
	{
		uint32_t animation_unknown = 1;
		AddActionToFront(object, 6/*action type*/, 0xFFFF/*action id?*/, 1, &animation_type, 2, &animation_speed, 2, &animation_duration, 1, &animation_unknown,
			0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
	}
}
void (*AddAction)(CNWSObject*, uint32_t, uint16_t, uint32_t, void*, uint32_t, void*, uint32_t, void*, uint32_t, void*, uint32_t, void*,
	uint32_t, void*, uint32_t, void*, uint32_t, void*, uint32_t, void*, uint32_t, void*, uint32_t, void*, uint32_t, void*) =
		(void (*)(CNWSObject*, uint32_t, uint16_t, uint32_t, void*, uint32_t, void*, uint32_t, void*, uint32_t, void*, uint32_t, void*,
	uint32_t, void*, uint32_t, void*, uint32_t, void*, uint32_t, void*, uint32_t, void*, uint32_t, void*, uint32_t, void*))0x081c9384;
VM_FUNC_NEW(ActionPlayAnimation, 221)
{
	uint32_t animation_type = vm_pop_int();
	float animation_speed = vm_pop_float();
	float animation_duration = vm_pop_float();
	CNWSObject* object = GetObjectById(vm_pop_object());
	if (object)
	{
		uint32_t animation_unknown = 1;
		AddAction(object, 6/*action type*/, 0xFFFF/*action id?*/, 1, &animation_type, 2, &animation_speed, 2, &animation_duration, 1, &animation_unknown,
			0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
	}
}

}
