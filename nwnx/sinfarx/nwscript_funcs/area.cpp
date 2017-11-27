#include "nwscript_funcs.h"
#include "../area.h"

using namespace nwnx::area;

namespace {

uint8_t (*GetPVPSetting)(CNWSArea*) = (uint8_t (*)(CNWSArea*))0x080d5234;
VM_FUNC_NEW(GetAreaPVPSetting, 236)
{
	CNWSArea* area = GetAreaById(vm_pop_object());
	vm_push_int(area ? GetPVPSetting(area) : -1);
}
VM_FUNC_NEW(SetUseDayNightCycle, 150)
{
	CNWSArea* area = GetAreaById(vm_pop_object());
	int use_day_night_cycle = vm_pop_int();
	if (area)
	{
		area->area_use_day_night_cyle = use_day_night_cycle;
	}
}
VM_FUNC_NEW(GetUseDayNightCycle, 151)
{
	CNWSArea* area = GetAreaById(vm_pop_object());
	int result = false;
	if (area)
	{
		result = (area->area_use_day_night_cyle != 0);
	}
	vm_push_int(result);
}
VM_FUNC_NEW(SetIsNight, 152)
{
	CNWSArea* area = GetAreaById(vm_pop_object());
	int is_night = vm_pop_int();
	if (area)
	{
		area->area_is_night = is_night;
	}
}

float (*ComputeHeight)(CNWSArea*, Vector) = (float (*)(CNWSArea*, Vector))0x080d65fc;
VM_FUNC_NEW(GetGroundHeight, 503)
{
    CNWSArea* area = GetAreaById(vm_pop_object());
    Vector pos = vm_pop_vector();
    float result = 0;
    if (area)
    {
        result = ComputeHeight(area, pos);
    }
    vm_push_float(result);
}
VM_FUNC_NEW(GetGroundHeightFromLocation, 504)
{
    CScriptLocation loc = vm_pop_location();
    CNWSArea* area = GetAreaById(loc.loc_area);
    float result = 0;
    if (area)
    {
        result = ComputeHeight(area, loc.loc_position);
    }
    vm_push_float(result);
}

int (*TestSafeLocationPoint)(CNWSArea*, Vector, CPathfindInformation*) = (int (*)(CNWSArea*, Vector, CPathfindInformation*))0x080d67a8;
VM_FUNC_NEW(GetIsWalkable, 505)
{
    CNWSArea* area = GetAreaById(vm_pop_object());
    Vector pos = vm_pop_vector();
    int result = 0;
    if (area && area->are_cre_pathfinding)
    {
        result = TestSafeLocationPoint(area, pos, area->are_cre_pathfinding);
    }
    vm_push_int(result);
}
VM_FUNC_NEW(GetIsWalkableLocation, 506)
{
    CScriptLocation loc = vm_pop_location();
    CNWSArea* area = GetAreaById(loc.loc_area);
    int result = 0;
    if (area && area->are_cre_pathfinding)
    {
        result = TestSafeLocationPoint(area, loc.loc_position, area->are_cre_pathfinding);
    }
    vm_push_int(result);
}
int (*GetSurfaceMaterial)(CNWSArea*, Vector) = (int (*)(CNWSArea*, Vector))0x080d528c;
VM_FUNC_NEW(GetSurface, 375)
{
	CScriptLocation location = vm_pop_location();
	CNWSArea* area = GetAreaById(location.loc_area);
	vm_push_int(area?GetSurfaceMaterial(area, location.loc_position):-1);
}

VM_FUNC_NEW(SetAreaName, 247)
{
	char* area = (char*)GetAreaById(vm_pop_object());
	CExoString name = vm_pop_string();
	if (area)
	{
		CExoLocString* loc_name = (CExoLocString*)(area+0xfc);
		ClearLocString(loc_name);
		AddLocString(loc_name, 0, name, 0);
		update_areas_for_dms();
	}
}

inline uint32_t vm_get_next_sound_object(CNWSArea* area)
{
	for (area->are_next_object_loop_index=area->are_next_object_loop_index+1; area->are_next_object_loop_index<area->are_objects.len; area->are_next_object_loop_index++)
	{
		CGameObject* o = GetGameObject(area->are_objects.data[area->are_next_object_loop_index]);
		if (o->type == OBJECT_TYPE_SOUND)
		{
			return o->id;
		}
	}
	return OBJECT_INVALID;
}
VM_FUNC_NEW(GetFirstSoundObjectInArea, 257)
{
	CNWSArea* area = GetAreaById(vm_pop_object());
	uint32_t result = OBJECT_INVALID;
	if (area)
	{
		area->are_next_object_loop_index = -1;
		result = vm_get_next_sound_object(area);
	}
	vm_push_object(result);
}
VM_FUNC_NEW(GetNextSoundObjectInArea, 258)
{
	CNWSArea* area = GetAreaById(vm_pop_object());
	uint32_t result = OBJECT_INVALID;
	if (area)
	{
		result = vm_get_next_sound_object(area);
	}
	vm_push_object(result);
}

void (*SendServerToPlayerMapPinAt)(CNWSMessage*, CNWSPlayer*, Vector, CExoString, uint32_t) = (void (*)(CNWSMessage*, CNWSPlayer*, Vector, CExoString, uint32_t))0x807cc80;
VM_FUNC_NEW(SendMapPinAdded, 267)
{
	CNWSPlayer* player = get_player_by_game_object_id( vm_pop_object());
	Vector pin_position = vm_pop_vector();
	CExoString description = vm_pop_string();
	int pin_id = vm_pop_int();
	if (player)
	{
		SendServerToPlayerMapPinAt(get_nws_message(), player, pin_position, description, pin_id);
	}
}
void (*SetMapNote)(CNWSWaypoint*, CExoLocString*) = (void (*)(CNWSWaypoint*, CExoLocString*))0x081f526c;
VM_FUNC_NEW(SetMapNote, 179)
{
	CGameObject* o = GetGameObject(vm_pop_object());
	CExoString note = vm_pop_string();
	if (o)
	{
		CNWSWaypoint* waypoint = o->vtable->AsNWSWaypoint(o);
		if (waypoint)
		{
			if (note.text && *note.text)
			{
				CExoLocString mapnote;
				CExoLocString_Constructor(&mapnote);
				AddLocString(&mapnote, 0, note, 0);
				SetMapNote(waypoint, &mapnote);
				waypoint->wp_has_mapnote = true;
				waypoint->wp_mapnote_enabled = true;
			}
			else
			{
				waypoint->wp_has_mapnote = false;
			}
		}
	}
}

VM_FUNC_NEW(GetNumPlayersInArea, 143)
{
	CNWSArea* area = GetAreaById(vm_pop_object());
	int result = 0;
	if (area)
	{
		result = area->num_players;
	}
	vm_push_int(result);
}

int (*TestLineWalkable)(CNWSArea*, float, float, float, float, float) = (int (*)(CNWSArea*, float, float, float, float, float))0x080dc890;
VM_FUNC_NEW(TestLineWalkable, 6)
{
	uint32_t area_id = vm_pop_object();
	Vector from_point = vm_pop_vector();
	Vector to_point = vm_pop_vector();
	int result = false;
	CGameObject* o = GetGameObject(area_id);
	if (o)
	{
		CNWSArea* area = o->vtable->AsNWSArea(o);
		if (area)
		{
			result = TestLineWalkable(area, from_point.x, from_point.y, to_point.x, to_point.y, from_point.z);
		}
	}
	vm_push_int(result);
}

}
