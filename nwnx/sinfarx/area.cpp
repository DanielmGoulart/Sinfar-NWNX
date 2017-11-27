#include "area.h"
#include "mysql.h"
#include "nwscript.h"
#include "player.h"
#include "ai.h"
#include "script_event.h"

using namespace nwnx::core;
using namespace nwnx::mysql;
using namespace nwnx::nwscript;

namespace nwnx { namespace area {

int GetAreaBdIdByResRef(char* area_resref)
{
	char sql[100];
	sprintf(sql, "SELECT id FROM areas WHERE resref='%s'", area_resref);
	if (mysql_admin->query(sql))
	{
		CMySQLRes result = mysql_admin->store_result();
		MYSQL_ROW row;
		if ((row = result.fetch_row()) != NULL)
		{
			return atoi(row[0]);
		}
	}
	//no id for that resref yet
	sprintf(sql, "INSERT INTO areas(resref) VALUES('%s')", area_resref);
	mysql_admin->query(sql);
	sprintf(sql, "SELECT LAST_INSERT_ID()");
	if (mysql_admin->query(sql))
	{
		CMySQLRes result = mysql_admin->store_result();
		MYSQL_ROW row;
		if ((row = result.fetch_row()) != NULL)
		{
			return atoi(row[0]);
		}
	}
	return 0;
}

std::unordered_set<std::string> dm_client_prefix;
std::unordered_set<CNWSArea*> dm_client_areas;
inline bool TryAddAreaToDMClient(CNWSArea* area)
{
	const char* area_resref = area->res_helper.resref.value;
	char prefix[17];
	int i;
	for (i=0; i<16; i++)
	{
		if (area_resref[i] == '_') break;
		prefix[i] = area_resref[i];
	}
	prefix[i] = 0;
	if (dm_client_prefix.count(prefix) > 0)
	{
		dm_client_areas.insert(area);
		return true;
	}
	return false;
}
void load_dm_areas_prefix()
{
	dm_client_prefix.clear();
	if (mysql_admin->query("SELECT reserved_prefix.prefix FROM reserved_prefix LEFT JOIN erf ON reserved_prefix.erf_id=erf.id WHERE erf.options&1"))
	{
		CMySQLRes result = mysql_admin->store_result();
		MYSQL_ROW row;
		while ((row = result.fetch_row()) != NULL)
		{
			dm_client_prefix.insert(row[0]);
		}
	}

	dm_client_areas.clear();
	CNWSModule* module = get_module();
	if (module)
	{
		for (uint32_t nArea=0; nArea<module->mod_areas.len; nArea++)
		{
			CNWSArea* area = GetAreaById(module->mod_areas.data[nArea]);
			if (area)
			{
				TryAddAreaToDMClient(area);
			}
		}
	}
}
void WriteDMAreaToMessage(CNWSMessage* message, CNWSArea* area)
{
	WriteObjectId(message, area->area_id);
	WriteCExoLocString(message, &(area->name), 0);
	WriteCExoString((CNWMessage*)message, area->area_tag, 0x20);
}
void OnSendAreasToDM_CreateWriteMessage(CNWSMessage* message, uint32_t p2, uint32_t p3, int p4)
{
	CreateWriteMessage(message, p2, p3, p4);
	int num_dm_areas_total = dm_client_areas.size();
	for (CNWSArea* active_area : ai::active_areas)
	{
		if (dm_client_areas.count(active_area) == 0)
		{
			num_dm_areas_total++;
		}
	}
	WriteInt((CNWMessage*)message, num_dm_areas_total, 0x20);
	for (CNWSArea* dm_area : dm_client_areas)
	{
		WriteDMAreaToMessage(message, dm_area);
	}
	for (CNWSArea* active_area : ai::active_areas)
	{
		if (dm_client_areas.count(active_area) == 0)
		{
			WriteDMAreaToMessage(message, active_area);
		}
	}
}

void (*SendServerToPlayerDungeonMasterAreaList)(CNWSMessage*, CNWSPlayer*) = (void (*)(CNWSMessage*, CNWSPlayer*))0x08075960;
void update_areas_for_dms()
{
	CNWSMessage* message = get_nws_message();
	CExoLinkedListNode* player_list_node = server_internal->srv_client_list_1->header->first;
	while (player_list_node)
	{
		CNWSPlayer* player = (CNWSPlayer*)player_list_node->data;
		if (player != NULL)
		{
			if (player->pl_type & 2)//character type == DM
			{
				SendServerToPlayerDungeonMasterAreaList(message, player);
			}
		}
		player_list_node = player_list_node->next;
	}
}
void (*CNWSArea_Constructor)(CNWSArea*, CResRef, int, uint32_t) = (void (*) (CNWSArea*, CResRef, int, uint32_t))0x080cbd30;
int (*LoadArea)(CNWSArea*, int) = (int (*)(CNWSArea*, int))0x080cdfdc;
void (*CNWSArea_Destructor)(CNWSArea*, int) = (void (*)(CNWSArea*, int))0x080cc244;
CExoArrayList<uint32_t>* (*GetAreaList)(CNWSModule*) = (CExoArrayList<uint32_t>* (*)(CNWSModule*))0x081c1044;
uint32_t CreateArea(char* area_resref)
{
	CResRef res;
	memset(res.value, 0, 16);
	strncpy(res.value, area_resref, 16);
	CNWSArea* area = (CNWSArea*)malloc(0x210);
	CNWSArea_Constructor(area, res, 0, OBJECT_INVALID);
	CNWSModule* module = get_module();
	if (LoadArea(area, 0))
	{
		uint32_t area_id = area->area_id;
		module->mod_areas.add(area_id);
		player::update_players_auto_map();
		if (TryAddAreaToDMClient(area))
		{
			update_areas_for_dms();
		}
		return area_id;
	}
	else
	{
		CNWSArea_Destructor(area, 3);
		return OBJECT_INVALID;
	}
}
bool DestroyArea(CNWSArea* area)
{
	if (area == NULL || area->num_players > 0) return false;
	CNWSModule* module = get_module();
	module->mod_areas.delvalue(area->area_id);
	CNWSArea_Destructor(area, 3);
	player::update_players_auto_map();
	dm_client_areas.erase(area);
	ai::active_areas.erase(area);
	update_areas_for_dms();
	return true;
}
void SetAutomap_OnLoadTURD_HookProc(CNWSCreature*, int, uint32_t*, char**)
{
	return;
}
void OnDropTurd_CopyAutoMap(CNWSPlayerTURD* turd, int p2, CExoArrayList<void*>* automap, uint8_t** p4)
{
	return;
}

void SetMapPinAt_HookProc(CNWSMessage* message, CNWSPlayer* player)
{
	if (ReadMessageOverflow(message, 1)) return;

	Vector pin_position;
	pin_position.x = ReadFloat(message, 1.0, 0x20);
	pin_position.y = ReadFloat(message, 1.0, 0x20);
	pin_position.z = ReadFloat(message, 1.0, 0x20);
	std::string map_pin_text = ReadExoString(message, 0x20);

	if (ReadMessageUnderflow(message, 1)==0)
	{
		pin_position.z = 0;
		script_event::run("mappin_add", player->pl_oid, {map_pin_text, pin_position});
	}

}
void DestroyMapPin_HookProc(CNWSMessage* message, CNWSPlayer* player)
{
	if (ReadMessageOverflow(message, 1)) return;

	int map_pin_id = ReadInt(message, 0x20);

	if (ReadMessageUnderflow(message, 1)) return;

	script_event::run("mappin_remove", player->pl_oid, {map_pin_id});
}
void EditMapPin_HookProc(CNWSMessage* message, CNWSPlayer* player)
{
	if (ReadMessageOverflow(message, 1)) return;

	ReadFloat(message, 1.0, 0x20);
	ReadFloat(message, 1.0, 0x20);
	ReadFloat(message, 1.0, 0x20);
	std::string map_pin_text = ReadExoString(message, 0x20);
	int map_pin_id = ReadInt(message, 0x20);

	if (ReadMessageUnderflow(message, 1)==0)
	{
		script_event::run("mappin_edit", player->pl_oid, {map_pin_id, map_pin_text});
	}
}

void init()
{
	//do not send all areas when a player log-in, this prevent people from seeing their party member's area
	enable_write(0x081b8e1e);
	*(uint16_t*)0x081b8e1e = 0xc031; //xor eax, eax
	*(uint8_t*)(0x081b8e1e + 2) = 0x40; //inc eax

	//send specific areas to DMs
	hook_call(0x08075a52, (uint32_t)OnSendAreasToDM_CreateWriteMessage);
	//do not send areas to DMs
	enable_write(0x08075a5f);
	*(uint16_t*)0x08075a5f = 0xE990;
	
	//dont retreive the automap from the player TURD, it may be invalid because of nwnx_areas
	hook_call(0x080546a8, (uint32_t)SetAutomap_OnLoadTURD_HookProc);
	//dont save it either, at all, to save memory
	hook_call(0x08054347, (uint32_t)OnDropTurd_CopyAutoMap);

	hook_function(0x0819995c, (unsigned long)SetMapPinAt_HookProc, d_ret_code_nouse, 12);
	hook_function(0x08199e8c, (unsigned long)DestroyMapPin_HookProc, d_ret_code_nouse, 12);
	hook_function(0x0819a0e8, (unsigned long)EditMapPin_HookProc, d_ret_code_nouse, 12);
	
	register_hook(hook::module_loaded, []{
		load_dm_areas_prefix();
	});
}
REGISTER_INIT(init);

VM_FUNC_NEW(CreateArea, 180)
{
	CExoString area_resref = vm_pop_string();
	uint32_t result = OBJECT_INVALID;
	if (area_resref.text)
	{
		result = CreateArea(area_resref.text);
	}
	vm_push_object(result);
}
VM_FUNC_NEW(DestroyArea, 181)
{
	uint32_t area_id = vm_pop_object();
	int result = false;
	CNWSArea* area = GetAreaById(area_id);
	if (area)
	{
		result = DestroyArea(area);
	}
	vm_push_int(result);
}

VM_FUNC_NEW(LoadDMAreasPrefix, 262)
{
	load_dm_areas_prefix();
	update_areas_for_dms();
}

VM_FUNC_NEW(GetAreaPersistentId, 546)
{
	CNWSArea* area = GetAreaById(vm_pop_object());
	int result = 0;
	if (area)
	{
		char area_resref[17];
		strncpy(area_resref,  ((char*)area)+176, 16);
		area_resref[16] = 0;
		result = GetAreaBdIdByResRef(area_resref);
	}
	vm_push_int(result);	
}

}
}
