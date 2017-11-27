#include "core.h"
#include "cstmfac.h"
#include "nwscript.h"
#include "mysql.h"
#include "creature.h"

using namespace nwnx::core;
using namespace nwnx::nwscript;
using namespace nwnx::mysql;
using namespace nwnx::creature;

namespace nwnx { namespace cstmfac {
	
typedef std::unordered_map<int, signed char> CUSTOM_FACTION_REPUTATIONS;
std::unordered_map<int, CUSTOM_FACTION_REPUTATIONS> custom_factions_reputations;

inline void SetCustomFactionReputation(int from_faction_id, int to_faction_id, signed char reputation)
{
	custom_factions_reputations[from_faction_id][to_faction_id] = reputation;
}
inline signed char GetCustomFactionReputation(int from_faction_id, int to_faction_id)
{
	std::unordered_map<int, CUSTOM_FACTION_REPUTATIONS>::iterator factions_iter = custom_factions_reputations.find(from_faction_id);
	if (factions_iter != custom_factions_reputations.end())
	{
		CUSTOM_FACTION_REPUTATIONS::iterator reputations_iter = factions_iter->second.find(to_faction_id);
		if (reputations_iter != factions_iter->second.end())
		{
			return reputations_iter->second;
		}
	}
	return -1;
}
void LoadAllCustomFactionsReputations()
{
	if (mysql_admin->query("SELECT from_id,to_id,reputation FROM custom_reputations"))
	{
		custom_factions_reputations.clear();
		CMySQLRes result = mysql_admin->store_result();
		MYSQL_ROW row;
		while ((row = result.fetch_row()) != NULL)
		{
			SetCustomFactionReputation(atoi(row[0]), atoi(row[1]), atoi(row[2]));
		}
	}
}
void load_creature_cstmfac(CNWSCreature* creature)
{
	std::vector<int>* cre_custom_factions = &(GetCreatureExtra(creature)->custom_factions);
	cre_custom_factions->clear();
	CNWSScriptVarTable* var_table = &(creature->obj.obj_vartable);
	uint32_t nVarCount = var_table->vt_len;
	for (uint32_t nVar=0; nVar<nVarCount; nVar++)
	{
		CScriptVariable* var = &(var_table->vt_list[nVar]);
		if (var->var_type == 1)//int
		{
			if (var->var_name.text && strncmp(var->var_name.text, "AI_FACTION_ID", 13) == 0)
			{
				cre_custom_factions->push_back(var->var_value);
			}
		}
	}
}
signed char GetCustomReputation(CNWSCreature* from, CNWSCreature* to)
{
	std::vector<int>* from_custom_factions = &(GetCreatureExtra(from)->custom_factions);
	std::vector<int>* to_custom_factions = &(GetCreatureExtra(to)->custom_factions);
	size_t from_fac_count = from_custom_factions->size();
	size_t to_fac_count = to_custom_factions->size();
	size_t match_count = 0;
	int reputation = 0;
	//custom faction
	for (size_t from_fac_index=0; from_fac_index<from_fac_count; from_fac_index++)
	{
		for (size_t to_fac_index=0; to_fac_index<to_fac_count; to_fac_index++)
		{
			signed char current_reputation = GetCustomFactionReputation(
				from_custom_factions->at(from_fac_index), to_custom_factions->at(to_fac_index));
			if (current_reputation != -1)
			{
				reputation += current_reputation;
				match_count++;
			}
		}
	}
	/*
	//standart faction of from
	for (size_t to_fac_index=0; to_fac_index<to_fac_count; to_fac_index++)
	{
		signed char current_reputation = GetCustomFactionReputation(
			(from->cre_is_pc?0:from->cre_stats->cs_faction_id), to_custom_factions->at(to_fac_index));
		fflush(stdout);
		if (current_reputation != -1)
		{
			reputation += current_reputation;
			match_count++;
		}
	}
	//standart faction of to
	for (size_t from_fac_index=0; from_fac_index<from_fac_count; from_fac_index++)
	{
		signed char current_reputation = GetCustomFactionReputation(
			from_custom_factions->at(from_fac_index), (to->cre_is_pc?0:to->cre_stats->cs_faction_id));
		if (current_reputation != -1)
		{
			reputation += current_reputation;
			match_count++;
		}
	}
	*/
	if (match_count > 0)
	{
		reputation /= match_count;
		return (signed char)reputation;
	}
	return -1;
}

int (*GetPersonalReputationAdjustment)(CNWSCreature*, uint32_t) = (int (*)(CNWSCreature*, uint32_t))0x81121ec;
int GetPersonalReputationAdjustment_HookProc(CNWSCreature* creature, uint32_t target_id)
{
	CNWSCreature* target = GetCreatureById(target_id);
	if (target != NULL)
	{
		if (!(creature->cre_is_pc && target->cre_is_pc))
		{
			int custom_reputation = GetCustomReputation(creature, target);
			if (custom_reputation != -1)
			{
				custom_reputation += GetPersonalReputationAdjustment(creature, target_id);
				*(uint32_t*)0x08108393 = 0x90909090;
				return custom_reputation;
			}
		}
	}
	*(uint32_t*)0x08108393 = 0xd889c301;
	return GetPersonalReputationAdjustment(creature, target_id);
}

void init()
{
	//personal reputation adjustment: check for custom faction first
	hook_call(0x0810838e, (long)GetPersonalReputationAdjustment_HookProc);
	
	register_hook(hook::module_loaded, []{
		LoadAllCustomFactionsReputations();
	});
}
REGISTER_INIT(init);
	
VM_FUNC_NEW(ReloadCustomFactionsReputations, 259)
{
	LoadAllCustomFactionsReputations();
}
VM_FUNC_NEW(UpdateCustomFactionsList, 260)
{
	CNWSCreature* creature = GetCreatureById(vm_pop_object());
	if (creature)
	{
		load_creature_cstmfac(creature);
	}
}
VM_FUNC_NEW(GetCustomFactionReputation, 261)
{
	int custom_rep = -1;
	CNWSCreature* from = GetCreatureById(vm_pop_object());
	CNWSCreature* to = GetCreatureById(vm_pop_object());
	if (from && to)
	{
		custom_rep = GetCustomReputation(from, to);
	}
	vm_push_int(custom_rep);
}

}
}