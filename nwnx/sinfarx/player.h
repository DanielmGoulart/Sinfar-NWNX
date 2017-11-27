#pragma once

#include "core.h"

#include <map>
#include <vector>

namespace nwnx { namespace player {

struct PLAYER_EXTRA : public CNWSPlayer
{
	void remove_from_current_area();
	PLAYER_EXTRA();
	void set_db_pc_id(uint32_t db_pc_id);
	~PLAYER_EXTRA();
	//dm fields
	uint32_t            field_98;
	uint32_t            field_9C;
	//extra fields
	uint32_t loaded_area_id;
	std::string fake_name;
	int initialized;
	uint32_t db_pc_id;
	int server_roles;
};

struct REMOTE_PLAYER
{
	REMOTE_PLAYER();
	int player_id;
	std::string player_name;
	uint32_t creature_id;
	uint32_t possessed_creature_id;
	bool is_dm;
	bool show_joined_msg;
};

struct PlayerExtraInfo
{
	PlayerExtraInfo();
	void reset();
	uint32_t banned_from_servers;
	uint32_t nwncx_version;
	int db_player_id;
	uint32_t game_version;
};
extern PlayerExtraInfo players_extra_info[0x60];

PlayerInfo* get_player_info(uint32_t player_id);
PlayerInfo* get_player_info(CNWSPlayer* player);

void send_message_to_pc(CNWSCreature* pc_creature, const char* message);

uint32_t custom_player_id_by_player_name(const std::string& player_name);
uint32_t custom_player_id_by_player(CNWSPlayer* player);

extern std::unordered_map<uint32_t, REMOTE_PLAYER> remote_players;
extern std::unordered_map<uint32_t, REMOTE_PLAYER>::iterator remote_players_iter;

uint32_t pc_id_by_player_id(uint32_t player_id);
uint32_t db_player_id_from_player(CNWSPlayer* player);
uint32_t db_player_id_from_creature(CNWSCreature* creature);
uint32_t db_player_id_from_object_id(uint32_t object_id);

void update_players_auto_map();

extern std::map<uint32_t,uint16_t> wc_get_chat_waiting;
void on_chat_target_update_send_udp(uint32_t player_id, const char* state);

REMOTE_PLAYER player_to_remote_player(CNWSPlayer* player);

void force_players_to_update_object(uint32_t object_id);
CLastUpdateObject* get_last_update_object(CNWSPlayer* player, uint32_t object_id);

}
}
