#include "ai.h"
#include "nwscript.h"
#include <sys/time.h>
#include <unistd.h>
#include "server.h"

using namespace nwnx::core;
using namespace nwnx::nwscript;

namespace nwnx { namespace ai {

std::unordered_set<CNWSArea*> active_areas;

int (*AddEventDeltaTime)(CServerAIMaster*, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, void*) = (int (*)(CServerAIMaster*, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, void*))0x08096388;

unsigned char d_ret_code_addobjtoai[0x20];
int (*AddObjectToAI_Org)(CServerAIMaster*, CNWSObject*, int) = (int (*)(CServerAIMaster*, CNWSObject*, int))&d_ret_code_addobjtoai;
int AddObjectToAI_HookProc(CServerAIMaster* ai_master, CNWSObject* object, int level)
{
    if (module_loaded)
    {
        return AddObjectToAI_Org(ai_master, object, level);
    }
    return 1;
}

uint32_t re_use_timer_count = 0;
bool re_use_timer = false;
int64_t last_high_res_time = 0;
inline int64_t gettimeofday_int64()
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    last_high_res_time = tv.tv_sec;
    last_high_res_time *= 1000*1000;
    last_high_res_time += tv.tv_usec;
    return last_high_res_time;
}
unsigned char d_ret_code_highrestimer[0x20];
int64_t GetHighResolutionTimer_Override(CExoTimers* timer)
{
    if (re_use_timer_count < 1000)
    {
        re_use_timer_count++;
        return last_high_res_time;
    }
    else
    {
        //fprintf(stderr, "re use timer >= 1000 (%d)\n", re_use_timer_count);
        re_use_timer_count = 0;
        return gettimeofday_int64();
    }
}
int64_t GetHighResolutionTimer_HookProc(CExoTimers* timer)
{
    if (re_use_timer)
    {
        return GetHighResolutionTimer_Override(timer);
    }
    else
    {
        re_use_timer_count = 0;
        return gettimeofday_int64();
    }
}
unsigned char d_ret_code_aiupdateplc[0x20];
void (*AIUpdate_Placeable_Org)(CNWSPlaceable*) = (void (*)(CNWSPlaceable*))&d_ret_code_aiupdateplc;
void AIUpdate_Placeable_HookProc(CNWSPlaceable* placeable)
{
    re_use_timer = true;
    AIUpdate_Placeable_Org(placeable);
    re_use_timer = false;
}
unsigned char d_ret_code_aiupdatedoor[0x20];
void (*AIUpdate_Door_Org)(CNWSDoor*) = (void (*)(CNWSDoor*))&d_ret_code_aiupdatedoor;
void AIUpdate_Door_HookProc(CNWSDoor* placeable)
{
    re_use_timer = true;
    AIUpdate_Door_Org(placeable);
    re_use_timer = false;
}

int64_t main_loop_recover_time = 0;
int64_t last_main_loop_start_time = 0;
#define LOOP_PAUSE_TIME_USEC 10*1000
#define LOOP_MAX_ELAPSED_TIME_USEC 20*LOOP_PAUSE_TIME_USEC
void MainLoop_USleep(uint32_t usec)
{
    int64_t current_time_int64 = gettimeofday_int64();
    main_loop_recover_time += current_time_int64-last_main_loop_start_time;
    if (main_loop_recover_time > LOOP_MAX_ELAPSED_TIME_USEC) main_loop_recover_time = LOOP_MAX_ELAPSED_TIME_USEC;
    last_main_loop_start_time = current_time_int64;
    if (main_loop_recover_time >= LOOP_PAUSE_TIME_USEC)
    {
        main_loop_recover_time -= LOOP_PAUSE_TIME_USEC;
    }
    else
    {
        int64_t sleep_time_usec = LOOP_PAUSE_TIME_USEC-main_loop_recover_time;
        main_loop_recover_time = 0;
        usleep(sleep_time_usec);
        last_main_loop_start_time += sleep_time_usec;
    }
}

std::vector<std::function<void()>> pending_x_events;
void add_pending_x_event(std::function<void()> pending_x_event)
{
    pending_x_events.push_back(pending_x_event);
}

int (*GetPendingEvent)(CServerAIMaster*, uint32_t*, uint32_t*, uint32_t*, uint32_t*, uint32_t*, void**) = (int (*)
    (CServerAIMaster*, uint32_t*, uint32_t*, uint32_t*, uint32_t*, uint32_t*, void**))0x8098334;
void (*UpdateModule)(CNWSModule*) = (void (*)(CNWSModule*))0x81c05fc;
void (*EventHandler_Module)(CNWSModule*, uint32_t, uint32_t, void*, uint32_t, uint32_t) = (void (*)
    (CNWSModule*, uint32_t, uint32_t, void*, uint32_t, uint32_t))0x081b3f0c;
void (*EventHandler_Area)(CNWSArea*, uint32_t, uint32_t, void*, uint32_t, uint32_t) = (void (*)
    (CNWSArea*, uint32_t, uint32_t, void*, uint32_t, uint32_t))0x080cc5f0;
void (*GetWorldTime)(CWorldTimer*, uint32_t*, uint32_t*) = (void (*)(CWorldTimer*, uint32_t*, uint32_t*))0x82416a4;
int (*EventPending)(CServerAIMaster*, uint32_t, uint32_t) = (int (*)(CServerAIMaster*, uint32_t, uint32_t))0x080982dc;
int (*UpdateDialog)(CNWSObject*) = (int (*)(CNWSObject*))0x81d0c38;
int (*ModuleAIUpdate)(CNWSModule*) = (int (*)(CNWSModule*))0x81b3dec;
int (*AreaAIUpdate)(CNWSArea*) = (int (*)(CNWSArea*))0x080cc3e0;
int update_counter = 0;
int last_ai_level_index = 0;
uint32_t last_ai_update_object_id = OBJECT_INVALID;
int64_t last_ai_update_start_time = 0;
inline bool update_object_ai_state(uint32_t object_id)
{
    if (last_high_res_time-last_ai_update_start_time >= LOOP_PAUSE_TIME_USEC)
    {
        last_ai_update_object_id = object_id;
        return false;
    }
    CGameObject* o = GetGameObject(object_id);
    if (o)
    {
        o->vtable->AIUpdate(o);
        UpdateDialog((CNWSObject*)o);
    }
    return true;
}
void (*DestroyScriptEvent)(CScriptEvent*, int) = (void (*)(CScriptEvent*, int))0x0806189C;
void (*DeleteScriptSituation)(CVirtualMachine*, CVirtualMachineScript*) = (void (*)(CVirtualMachine*, CVirtualMachineScript*))0x08264550;
void (*DeleteEventData)(CServerAIMaster*, uint32_t, void*) = (void (*)(CServerAIMaster*, uint32_t, void*))0x080976c0;
void UpdateAIState(CServerAIMaster* ai_master)
{
    for (auto pending_x_event : pending_x_events)
    {
        pending_x_event();
    }
    pending_x_events.clear();

    last_ai_update_start_time = last_high_res_time;
    uint32_t time_high;
    uint32_t time_low;
    GetWorldTime(server_internal->srv_time_world, &time_high, &time_low);
    while (EventPending(ai_master, time_high, time_low))
    {
        uint32_t event_params1;
        uint32_t event_params2;
        uint32_t event_params3;
        uint32_t event_object_id;
        uint32_t event_type;
        void* event_data;
        if (GetPendingEvent(ai_master, &event_params1, &event_params2, &event_params3, &event_object_id, &event_type, &event_data))
        {
            CGameObject* o_event = GetGameObject(event_object_id);
            if (o_event)
            {
                if (o_event->type == OBJECT_TYPE_AREA)
                {
                    CNWSArea* area = o_event->vtable->AsNWSArea(o_event);
                    EventHandler_Area(area, event_type, event_params3, event_data, event_params1, event_params2);
                }
                else if (o_event->type == OBJECT_TYPE_MODULE)
                {
                    CNWSModule* module = o_event->vtable->AsNWSModule(o_event);
                    EventHandler_Module(module, event_type, event_params3, event_data, event_params1, event_params2);
                }
                else
                {
                    o_event->vtable->EventHandler(o_event, event_type, event_params3, event_data, event_params1, event_params2);
                }
            }
            else if (event_data)
            {
                DeleteEventData(ai_master, event_type, event_data);
            }
        }
    }

    //first complete the last_ai_level_index, if needed
    if (last_ai_update_object_id != OBJECT_INVALID)
    {
        uint32_t* p_object = ai_master->ai_objects[last_ai_level_index].objects.data;
        uint32_t* p_last_object = p_object+ai_master->ai_objects[last_ai_level_index].objects.len;
        //go to the last updated object
        while (p_object < p_last_object && *p_object != last_ai_update_object_id) p_object++;
        //last updated object not found: start from the first index
        if (p_object == p_last_object) p_object = ai_master->ai_objects[last_ai_level_index].objects.data;
        while (p_object < p_last_object && update_object_ai_state(*p_object)) p_object++;
        if (p_object < p_last_object) return; //time out
        last_ai_level_index++;
    }
    for (; last_ai_level_index<=4; last_ai_level_index++)
    {
        if (update_counter % (1+(4-last_ai_level_index)*2) == 0)
        {
            uint32_t* p_object = ai_master->ai_objects[last_ai_level_index].objects.data;
            uint32_t* p_last_object = p_object+ai_master->ai_objects[last_ai_level_index].objects.len;
            while (p_object < p_last_object && update_object_ai_state(*p_object)) p_object++;
            if (p_object < p_last_object) return; //time out
        }
    }
    last_ai_level_index = 0;
    last_ai_update_object_id = OBJECT_INVALID;
    if (update_counter % 12 == 0)
    {
        ModuleAIUpdate(module);
        for (auto iter=active_areas.begin(); iter!=active_areas.end(); iter++)
        {
            AreaAIUpdate(*iter);
        }
    }
    update_counter++;
}
int (*RunScriptSituation)(CVirtualMachine*, CVirtualMachineScript*, uint32_t, int) = (int (*)(CVirtualMachine*, CVirtualMachineScript*, uint32_t, int))0x08262534;
unsigned char d_ret_code_dooreventhandler[0x20];
void (*DoorEventHandler_Org)(CNWSDoor*, uint32_t, uint32_t, void*, uint32_t, uint32_t) = (void (*)(CNWSDoor*, uint32_t, uint32_t, void*, uint32_t, uint32_t))&d_ret_code_dooreventhandler;
void DoorEventHandler_Hook(CNWSDoor* door, uint32_t event_type, uint32_t p3, void* event_data, uint32_t p5, uint32_t p6)
{
    if (p3!=OBJECT_INVALID && cgoa.find(p3)==cgoa.end())
    {
        fprintf(stderr, "crash fix:DoorEventHandler:creature is null\n");
        return;
    }

    return DoorEventHandler_Org(door, event_type, p3, event_data, p5, p6);
}
void (*RemoveEncounterFromArea)(CNWSEncounter*) = (void (*)(CNWSEncounter*))0x0818402c;
void (*DestroyEncounter)(CNWSEncounter*, int) = (void (*)(CNWSEncounter*, int))0x0817fae4;
unsigned char d_ret_code_encountereventhandler[0x20];
void (*EncounterEventHandler_Org)(CNWSEncounter*, uint32_t, uint32_t, void*, uint32_t, uint32_t) = (void (*)(CNWSEncounter*, uint32_t, uint32_t, void*, uint32_t, uint32_t))&d_ret_code_encountereventhandler;
void EncounterEventHandler_Hook(CNWSEncounter* encounter, uint32_t event_type, uint32_t p3, void* event_data, uint32_t p5, uint32_t p6)
{
    if (event_type == 1 /* timed event */)
    {
        if (event_data)
        {
            RunScriptSituation(virtual_machine, static_cast<CVirtualMachineScript*>(event_data), encounter->obj.obj_id, 1);
        }
    }
    else if (event_type == 11 /* destroy object*/)
    {
        RemoveEncounterFromArea(encounter);
        DestroyEncounter(encounter, 3);
    }
    else if (event_type == 10 /* signal event */)
    {
        EncounterEventHandler_Org(encounter, event_type, p3, event_data, p5, p6);
    }
    else
    {
        fprintf(stderr, "EncounterEventHandler_Hook: unknonw event:%d\n", event_type);
    }
}
unsigned char d_ret_code_wpeventhandler[0x20];
void (*WaypointEventHandler_Org)(CNWSWaypoint*, uint32_t, uint32_t, void*, uint32_t, uint32_t) = (void (*)(CNWSWaypoint*, uint32_t, uint32_t, void*, uint32_t, uint32_t))&d_ret_code_wpeventhandler;
void WaypointEventHandler_Hook(CNWSWaypoint* waypoint, uint32_t event_type, uint32_t p3, void* event_data, uint32_t p5, uint32_t p6)
{
    if (event_type == 1 /* timed event */)
    {
        if (event_data)
        {
            RunScriptSituation(virtual_machine, static_cast<CVirtualMachineScript*>(event_data), waypoint->obj.obj_id, 1);
        }
    }
    else
    {
        return WaypointEventHandler_Org(waypoint, event_type, p3, event_data, p5, p6);
    }
}
void (*DestroySound)(CNWSSoundObject*, int) = (void (*)(CNWSSoundObject*, int))0x081e9948;
void (*RemoveSoundFromArea)(CNWSSoundObject*) = (void (*)(CNWSSoundObject*))0x081e9a10;
void SoundObjectEventHandler_Hook(CNWSSoundObject* sound, uint32_t event_type, uint32_t p3, void* event_data, uint32_t p5, uint32_t p6)
{
    if (event_type == 1 /* timed event */)
    {
        if (event_data)
        {
            RunScriptSituation(virtual_machine, static_cast<CVirtualMachineScript*>(event_data), sound->obj.obj_id, 1);
        }
    }
    else if (event_type == 11 /* destroy object*/)
    {
        RemoveSoundFromArea(sound);
        DestroySound(sound, 3);
    }
    else
    {
        fprintf(stderr, "SoundObjectEventHandler_Hook: unknonw event:%d\n", event_type);
    }
}
bool validate_timed_event_object(uint32_t object_id)
{
    CGameObject* go = GetGameObject(object_id);
    if (!go) return false;
    switch (go->type)
    {
        case OBJECT_TYPE_STORE:
        case OBJECT_TYPE_AREA:
        case OBJECT_TYPE_CREATURE:
        case OBJECT_TYPE_DOOR:
        case OBJECT_TYPE_ITEM:
        case OBJECT_TYPE_MODULE:
        case OBJECT_TYPE_PLACEABLE:
        case OBJECT_TYPE_AREA_OF_EFFECT:
        case OBJECT_TYPE_TRIGGER:
        case OBJECT_TYPE_WAYPOINT:
        case OBJECT_TYPE_ENCOUNTER:
        case OBJECT_TYPE_SOUND:
            return true;
    }
    fprintf(stderr, "trying to assign a command to an invalid object(%d|%s)\n", go->type, get_last_script_ran().c_str());
    return false;
}
CVirtualMachineScript* last_vm_script = NULL;
unsigned char d_ret_code_popcommand[0x20];
int (*StackPopCommand_Org)(CVirtualMachine*, CVirtualMachineScript**) = (int (*)(CVirtualMachine*, CVirtualMachineScript**))&d_ret_code_popcommand;
int StackPopCommand_Hook(CVirtualMachine* vm, CVirtualMachineScript** p_vm_script)
{
    if (last_vm_script)
    {
        DeleteScriptSituation(virtual_machine, last_vm_script);
        last_vm_script = NULL;
    }
    int ret = StackPopCommand_Org(vm, p_vm_script);
    if (!ret) return ret;
    last_vm_script = *p_vm_script;
    return true;
}
int AddTimedEvent(CServerAIMaster* server_ai, uint32_t p1, uint32_t p2, uint32_t p3, uint32_t event_object_id, uint32_t event_type, void* event_data)
{
    if (event_data != last_vm_script)
    {
        fprintf(stderr, "AddTimedEvent called without the last popped command\n");
    }
    last_vm_script = NULL;
    if (!validate_timed_event_object(event_object_id))
    {
        CGameObject* go = GetGameObject(event_object_id);
        fprintf(stderr, "Assign/DelayComand called with a bad object type (%d|%x|%s)\n", (go?go->type:-1), event_object_id, get_last_script_ran().c_str());
        DeleteScriptSituation(virtual_machine, static_cast<CVirtualMachineScript*>(event_data));
        return false;
    }
    int ret = AddEventDeltaTime(server_ai, p1, p2, p3, event_object_id, event_type, event_data);
    if (!ret)
    {
        DeleteScriptSituation(virtual_machine, static_cast<CVirtualMachineScript*>(event_data));
    }
    return ret;
}
int (*AddCommandAction)(CNWSObject*, CVirtualMachineScript*) = (int (*)(CNWSObject*, CVirtualMachineScript*))0x081C861C;
int ScriptAddCommandAction(CNWSObject* object, CVirtualMachineScript* vm_script)
{
    if (vm_script != last_vm_script)
    {
        fprintf(stderr, "ScriptAddCommandAction called without the last popped command\n");
    }
    last_vm_script = NULL;
    return AddCommandAction(object, vm_script);
}

int (*AddObjectToAI)(CServerAIMaster*, CNWSObject*, int) = (int (*)(CServerAIMaster*, CNWSObject*, int))0x080980e0;
int (*RemoveObjectFromAI)(CServerAIMaster*, CNWSObject*) = (int (*)(CServerAIMaster*, CNWSObject*))0x080962a8;
int OnAddObjectToAI_Bypass(CServerAIMaster* server_ai, CNWSObject* object, int ai_level)
{
	return 1;
}
int (*GetItemWeight)(CNWSItem*) = (int (*)(CNWSItem*))0x81a6b90;
int OnEquip_GetWeight(CNWSItem* item)
{
	AddObjectToAI(server_internal->srv_ai, &(item->obj), 0);
	return GetItemWeight(item);
}
int OnUnequip_GetWeight(CNWSItem* item)
{
	RemoveObjectFromAI(server_internal->srv_ai, &(item->obj));
	return GetItemWeight(item);
}

CNWSArea* OnUpdateModule_GetAreaById(CServerExoApp* server, uint32_t object_id)
{
	CNWSArea* area = GetAreaById(object_id);
	if (area && area->num_players > 0) return area;
	return NULL;
}

void UpdatePlayerCountInArea_Hook(CNWSArea* area)
{
	//handled in different events
}

void init()
{
    //sleep longer when the server is not busy, to reduce the CPU usage and not the lag
    hook_call(0x0804bbf6, (uint32_t)MainLoop_USleep);

    hook_function(0x82cc7a8, (unsigned long)GetHighResolutionTimer_HookProc, d_ret_code_highrestimer, 11);
	hook_function(0x081e0614,(unsigned long)AIUpdate_Placeable_HookProc, d_ret_code_aiupdateplc, 9);
	hook_function(0x08166d64,(unsigned long)AIUpdate_Door_HookProc, d_ret_code_aiupdatedoor, 9);
	//all high res timer request in UpdateState()
	hook_call(0x08095dcf, (uint32_t)GetHighResolutionTimer_Override);
	hook_call(0x08095e47, (uint32_t)GetHighResolutionTimer_Override);
	hook_call(0x08095e73, (uint32_t)GetHighResolutionTimer_Override);
	hook_call(0x08095f0b, (uint32_t)GetHighResolutionTimer_Override);
	hook_call(0x08096053, (uint32_t)GetHighResolutionTimer_Override);
	hook_call(0x080960fe, (uint32_t)GetHighResolutionTimer_Override);
	hook_call(0x08096189, (uint32_t)GetHighResolutionTimer_Override);
	hook_call(0x080961c2, (uint32_t)GetHighResolutionTimer_Override);
	hook_call(0x08096243, (uint32_t)GetHighResolutionTimer_Override);

    hook_function(0x080980e0, (unsigned long)AddObjectToAI_HookProc, d_ret_code_addobjtoai, 12);
	hook_function(0x08095d60, (unsigned long)UpdateAIState, d_ret_code_nouse, 9);

	hook_function(0x08166e4c, (unsigned long)DoorEventHandler_Hook, d_ret_code_dooreventhandler, 12);
	hook_function(0x0817fef0, (unsigned long)EncounterEventHandler_Hook, d_ret_code_encountereventhandler, 10);
	hook_function(0x081f5110, (unsigned long)WaypointEventHandler_Hook, d_ret_code_wpeventhandler, 10);
	hook_function(0x081E999C, (unsigned long)SoundObjectEventHandler_Hook, d_ret_code_nouse, 8);

	//prevent StackPop without storing the result
	hook_function(0x08262ebc, (unsigned long)StackPopCommand_Hook, d_ret_code_popcommand, 12);
	hook_call(0x081FDD32, (uint32_t)AddTimedEvent);
	hook_call(0x081FDE55, (uint32_t)AddTimedEvent);
	hook_call(0x08233256, (uint32_t)ScriptAddCommandAction);
    
    //set the default AI level of AoE
	enable_write(0x081e9fea);
	*(uint8_t*)0x081e9fea = 1;
	//"" for creatures
	enable_write(0x08113ff9);
	*(uint8_t*)0x08113ff9 = 3;
    
    //add object to ai... bypass for objects that dont need ai
	enable_write(0x0819ee8b);
	*(uint32_t*)(0x0819ee8b) = ((uint32_t)OnAddObjectToAI_Bypass-(uint32_t)0x0819ee8f);
	enable_write(0x08083d16);
	*(uint32_t*)(0x08083d16) = ((uint32_t)OnAddObjectToAI_Bypass-(uint32_t)0x08083d1a);
	//but then need to add equipped items back in the ai array
	enable_write(0x0811b773);
	*(uint32_t*)(0x0811b773) = ((uint32_t)OnEquip_GetWeight-(uint32_t)0x0811b777);
	enable_write(0x0811b861);
	*(uint32_t*)(0x0811b861) = ((uint32_t)OnUnequip_GetWeight-(uint32_t)0x0811b865);
    
    //do not update area with no players
	hook_call(0x081c063a, (uint32_t)OnUpdateModule_GetAreaById);
	
	hook_function(0x080d489c, (uint32_t)UpdatePlayerCountInArea_Hook, d_ret_code_nouse, 8);
	hook_function(0x080d48cc, (uint32_t)UpdatePlayerCountInArea_Hook, d_ret_code_nouse, 9);
}
REGISTER_INIT(init);

VM_FUNC_NEW(SuspendArea, 285)
{
	CNWSArea* area = GetAreaById(vm_pop_object());
	if (area)
	{
		active_areas.erase(area);
		if (area->num_players != 0)
		{
			fprintf(stderr, "area: %s has an invalid number of players: %d instead of 0\n", area->area_tag.text, area->num_players);
		}
	}
}
VM_FUNC_NEW(WokeUpArea, 286)
{
	CNWSArea* area = GetAreaById(vm_pop_object());
	if (area)
	{
		active_areas.insert(area);
	}
}

}
}
