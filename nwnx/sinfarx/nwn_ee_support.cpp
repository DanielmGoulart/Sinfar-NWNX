#include "player.h"
#include "nwncx.h"

using namespace nwnx::core;
using namespace nwnx::player;
using namespace nwnx::nwncx;

uint32_t last_player_to_server_id = 0;
unsigned char d_ret_code_handleplayertoservmsg[0x20];
int (*HanldePlayertoServerMessage_Org)(CNWSMessage*, uint32_t, uint8_t*, uint32_t) = (int (*)(CNWSMessage*, uint32_t, uint8_t*, uint32_t))&d_ret_code_handleplayertoservmsg;
int HanldePlayertoServerMessage_Hook(CNWSMessage* msg, uint32_t player_id, uint8_t* data, uint32_t data_size)
{
	last_player_to_server_id = player_id;
	return HanldePlayertoServerMessage_Org(msg, player_id, data, data_size);
}


void OnWritePlcAppearance_AddBool(CNWSMessage* msg, bool value)
{
	WriteBool(msg, value);
	if (players_extra_info[sending_obj_update_player->pl_id].game_version > 8109)
	{
		WriteBool(msg, 0);
	}		
}


float OnDriveControl_ReadObjId(CNWSMessage* msg, float p2, int p3)
{
	float result = ReadFloat(msg, p2, p3);
	if (players_extra_info[last_player_to_server_id].game_version > 8109)
	{
		ReadObjectId(msg);
	}
	return result;
}

namespace nwnx {
	void init()
	{
		hook_call(0x08061EE4, (long)OnWritePlcAppearance_AddBool);
		
		hook_call(0x0818F8E7, (long)OnDriveControl_ReadObjId);
		
		hook_function(0x08196544, (long)HanldePlayertoServerMessage_Hook, d_ret_code_handleplayertoservmsg, 12);
		
		enable_write(0x0830ED43);
		strcpy((char*)0x0830ED43, "8149");
	}
	REGISTER_INIT(init);
}
