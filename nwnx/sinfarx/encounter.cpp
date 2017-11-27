#include "core.h"
#include "creature.h"
#include "nwscript.h"

using namespace nwnx::core;
using namespace nwnx::creature;
using namespace nwnx::nwscript;

namespace nwnx { namespace encounter {

struct ENCOUNTER_DATA
{
	uint8_t spawn_min;
	uint8_t spawn_max;
};
std::unordered_map<CNWSEncounter*, ENCOUNTER_DATA*/*, map_encountercmp*/> encounters;
unsigned char d_ret_code_createe[0x20];
void (*EncounterSpawn_Org)(CNWSEncounter*) = (void (*)(CNWSEncounter*))&d_ret_code_createe;
void EncounterSpawn_HookProc(CNWSEncounter* encounter)
{
	ENCOUNTER_DATA* encounter_data;
	std::unordered_map<CNWSEncounter*, ENCOUNTER_DATA*/*, map_encountercmp*/>::iterator iter_encounters = encounters.find(encounter);
	if (iter_encounters != encounters.end())
	{
		encounter_data = iter_encounters->second;
	}
	else
	{
		encounter_data = new ENCOUNTER_DATA;
		encounter_data->spawn_min = *(((char*)encounter)+492);
		encounter_data->spawn_max = *(((char*)encounter)+496);
		encounters[encounter] = encounter_data;
	}

	uint8_t num_creature = (uint8_t)((rand()/(RAND_MAX+1.0))*(1 + encounter_data->spawn_max - encounter_data->spawn_min)) + encounter_data->spawn_min;
	*(((char*)encounter)+492) = num_creature;
	*(((char*)encounter)+496) = num_creature;

	return EncounterSpawn_Org(encounter);
}
unsigned char d_ret_code_destroye[0x20];
void (*DestroyEncounter_Org)(CNWSEncounter*) = (void (*)(CNWSEncounter*))&d_ret_code_destroye;
void DestroyEncounter_HookProc(CNWSEncounter* encounter)
{
	std::unordered_map<CNWSEncounter*, ENCOUNTER_DATA*/*, map_encountercmp*/>::iterator iter_encounters = encounters.find(encounter);
	if (iter_encounters != encounters.end())
	{
		encounters.erase(iter_encounters);
	}

	DestroyEncounter_Org(encounter);
}

void init()
{
	hook_function(0x08180510, (long)EncounterSpawn_HookProc, d_ret_code_createe, 12);
	hook_function(0x0817fae4, (long)DestroyEncounter_HookProc, d_ret_code_destroye, 12);
}
REGISTER_INIT(init);
	
}
}