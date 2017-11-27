#include "core.h"
#include <unordered_set>

using namespace nwnx::core;

namespace
{
	
void (*RemoveFactionMember)(CNWSFaction*, uint32_t) = (void (*)(CNWSFaction*, uint32_t))0x081d6bc8;
int on_send_party_list_destroy_faction = 0;
void RemoveFactionMemberAndDestroyNewFaction(CNWSFaction* faction, uint32_t creature_id)
{
	on_send_party_list_destroy_faction++;
	RemoveFactionMember(faction, creature_id);
	on_send_party_list_destroy_faction--;
}
	
std::unordered_set<uint32_t> standart_factions;
void (*CreateFaction)(CNWSFaction*) = (void (*)(CNWSFaction*))0x81d68ec;
void OnCreateStandartFaction(CNWSFaction* faction)
{
	CreateFaction(faction);
	standart_factions.insert(faction->fac_id);
}
void (*DestroyFaction)(CNWSFaction*, int) = (void (*)(CNWSFaction*, int))0x081d7d30;
void (*SendServerToPlayersPartyList)(CNWSMessage*, uint32_t, int, uint32_t*, uint8_t, uint32_t) = (void (*)(CNWSMessage*, uint32_t, int, uint32_t*, uint8_t, uint32_t))0x8081138;
void SendFactionUpdateList(CNWSFaction* faction)
{
	if (standart_factions.count(faction->fac_id) > 0) return;

	if (on_send_party_list_destroy_faction)
	{
		server_internal->srv_factions->factions->delvalue(faction);
		DestroyFaction(faction, 3);
	}
	else
	{
		CNWSMessage* message = get_nws_message();
		uint32_t* members_start = faction->fac_members.data;
		uint32_t members_len = faction->fac_members.len;
		uint32_t* members_end = members_start+members_len;
		uint32_t* members = members_start;
		while (members < members_end)
		{
			CNWSPlayer* member_player = get_player_by_game_object_id(*members);
			if (member_player)
			{
				SendServerToPlayersPartyList(message, member_player->pl_id, members_len, members_start, 1, *members);
			}
			members++;
		}
	}
}
void SendFactionUpdateAdd(CNWSFaction* faction, uint32_t object_id)
{
	if (standart_factions.count(faction->fac_id)) return;

	CNWSMessage* message = get_nws_message();

	uint32_t* members_start = faction->fac_members.data;
	uint32_t* members_end = members_start+faction->fac_members.len;
	uint32_t* members = members_start;
	while (members < members_end)
	{
		CNWSPlayer* member_player = get_player_by_game_object_id(*members);
		if (member_player)
		{
			SendServerToPlayersPartyList(message, member_player->pl_id, 1, &object_id, 3, *members);
		}
		members++;
	}
}
void SendFactionUpdateRemove(CNWSFaction* faction, uint32_t object_id)
{
	if (standart_factions.count(faction->fac_id)) return;

	CNWSMessage* message = get_nws_message();

	uint32_t* members_start = faction->fac_members.data;
	uint32_t* members_end = members_start+faction->fac_members.len;
	uint32_t* members = members_start;
	while (members < members_end)
	{
		CNWSPlayer* member_player = get_player_by_game_object_id(*members);
		if (member_player)
		{
			SendServerToPlayersPartyList(message, member_player->pl_id, 1, &object_id, 4, OBJECT_INVALID);
		}
		members++;
	}
}	
	
void init()
{
	//create standart faction: save it
	hook_call(0x080b9e1d, (long)OnCreateStandartFaction);
	
	hook_call(0x080A4EF9, (uint32_t)RemoveFactionMemberAndDestroyNewFaction);
	hook_call(0x081d69db, (uint32_t)RemoveFactionMemberAndDestroyNewFaction);
	
	hook_function(0x081d7fcc, (unsigned long)SendFactionUpdateAdd, d_ret_code_nouse, 9);
	hook_function(0x081d8048, (unsigned long)SendFactionUpdateRemove, d_ret_code_nouse, 9);
	hook_function(0x081d7f50, (unsigned long)SendFactionUpdateList, d_ret_code_nouse, 9);	
}
REGISTER_INIT(init);
	
}