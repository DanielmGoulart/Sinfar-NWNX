#include "chat.h"
#include "nwscript.h"
#include "player.h"
#include "mysql.h"
#include "script_event.h"
#include "server.h"

#include <boost/format.hpp>
#include <boost/lexical_cast.hpp>

#include <sys/types.h>
#include <dirent.h>

using namespace nwnx::core;
using namespace nwnx::nwscript;
using namespace nwnx::script_event;
using namespace nwnx::player;
using namespace nwnx::mysql;

namespace nwnx { namespace chat {

CGameObject* (*GetGameObjectById)(CServerExoApp*, uint32_t) = (CGameObject* (*)(CServerExoApp*, uint32_t))0x080b1d98;

int invalid_player_id = 255;
CNWSPlayer* OnChat_GetPlayerObjectForId(CServerExoApp* server_exo_app, uint32_t object_id)
{
	CNWSPlayer* player = get_player_by_game_object_id(object_id);
	if (player) return player;
	return (CNWSPlayer*)&invalid_player_id;
}

void init()
{
    //send chat message from non player game object
	hook_call(0x08068bba, (long)GetGameObjectById);
	for (uint8_t* p_b=(uint8_t*)0x08068bbf; p_b<(uint8_t*)0x08068bc7; p_b++) *p_b = 0x90;
	hook_call(0x08068ca2, (long)GetGameObjectById);
	for (uint8_t* p_b=(uint8_t*)0x08068ca7; p_b<(uint8_t*)0x08068caf; p_b++) *p_b = 0x90;
	//when sending tell, GetClientGameObject must at least return a pointer of an int
	hook_call(0x0806846a, (long)OnChat_GetPlayerObjectForId);

    //send party chat: GetClientByGameObjectId: only one object per player to prevent double chat message
	hook_call(0x081d7ed3, (long)GetClientByGameObjectId_Unique_Hook);
	//add/remove from area: only one object per player to not break the counter (which then set the AILevel to 0 of all creatures in the area)
	hook_call(0x080cdc1e, (long)GetClientByGameObjectId_Unique_Hook);
	hook_call(0x080cd7e4, (long)GetClientByGameObjectId_Unique_Hook);
}
REGISTER_INIT(init);

void (*SendServerToPlayerChatMessage)(CNWSMessage*, uint8_t, uint32_t, CExoString, uint32_t, CExoString*) = (void (*)(CNWSMessage*, uint8_t, uint32_t, CExoString, uint32_t, CExoString*))0x0806839c;
VM_FUNC_NEW(SendChatMessage, 248)
{
	uint32_t from_id = vm_pop_object();;
	int mode = vm_pop_int();
	CExoString chat_message = vm_pop_string();
	uint32_t to_id = vm_pop_object();
	if (mode < 1 || (mode > 6 && mode != 0x11 && mode != 0x13)) return;
	if (!GetGameObject(from_id)) return;
	switch (mode)
	{
		case 4: //tell
			if (!get_player_by_game_object_id(to_id)) return;
			break;
		case 6: //party
			if (!get_player_by_game_object_id(from_id)) return; //this should not be needed
			if (!GetCreatureById(from_id)) return;
			break;
	}
	CExoString xz;
	SendServerToPlayerChatMessage(get_nws_message(), mode, from_id, chat_message, to_id, &xz);
}
void (*SendServerToPlayerChat_Talk)(CNWSMessage*, uint32_t, uint32_t, CExoString) = (void (*)(CNWSMessage*, uint32_t, uint32_t, CExoString))0x807fe10;
void (*SendServerToPlayerChat_Party)(CNWSMessage*, uint32_t, uint32_t, CExoString) = (void (*)(CNWSMessage*, uint32_t, uint32_t, CExoString))0x08068fe4;
void (*SendServerToPlayerChat_Tell)(CNWSMessage*, uint32_t, uint32_t, CExoString) = (void (*)(CNWSMessage*, uint32_t, uint32_t, CExoString))0x080694ac;
void (*SendServerToPlayerChat_Shout)(CNWSMessage*, uint32_t, uint32_t, CExoString) = (void (*)(CNWSMessage*, uint32_t, uint32_t, CExoString))0x08069710;
void (*SendServerToPlayerChat_DMTalk)(CNWSMessage*, uint32_t, uint32_t, CExoString) = (void (*)(CNWSMessage*, uint32_t, uint32_t, CExoString))0x08069248;
void (*SendServerToPlayerChat_SilentShout)(CNWSMessage*, uint32_t, uint32_t, CExoString) = (void (*)(CNWSMessage*, uint32_t, uint32_t, CExoString))0x08069a74;
void (*SendServerToPlayerChat_DMSilentShout)(CNWSMessage*, uint32_t, uint32_t, CExoString) = (void (*)(CNWSMessage*, uint32_t, uint32_t, CExoString))0x08069dd8;
void (*SendServerToPlayerChat_DMWhisper)(CNWSMessage*, uint32_t, uint32_t, CExoString) = (void (*)(CNWSMessage*, uint32_t, uint32_t, CExoString))0x0806a03c;
void (*SendServerToPlayerChat_ServerTell)(CNWSMessage*, uint32_t, CExoString) = (void (*)(CNWSMessage*, uint32_t, CExoString))0x0807ff68;
void (*SendServerToPlayerChat_Whisper)(CNWSMessage*, uint32_t, uint32_t, CExoString) = (void (*)(CNWSMessage*, uint32_t, uint32_t, CExoString))0x0808003c;
VM_FUNC_NEW(SendSingleChatMessage, 249)
{
	uint32_t from_id = vm_pop_object();
	int channel = vm_pop_int();
	CExoString chat_message = vm_pop_string();
	uint32_t to_id = vm_pop_object();
	CNWSPlayer* player_to = get_player_by_game_object_id(to_id);
	if (player_to!=NULL)
	{
		CExoString chat(chat_message);
		if (channel == 1)
			SendServerToPlayerChat_Talk(get_nws_message(), player_to->pl_id, from_id, chat);
		else if (channel == 2)
			SendServerToPlayerChat_Party(get_nws_message(), player_to->pl_id, from_id, chat);
		else if (channel == 3)
			SendServerToPlayerChat_Tell(get_nws_message(), player_to->pl_id, from_id, chat);
		else if (channel == 4)
			SendServerToPlayerChat_Shout(get_nws_message(), player_to->pl_id, from_id, chat);
		else if (channel == 5)
			SendServerToPlayerChat_DMTalk(get_nws_message(), player_to->pl_id, from_id, chat);
		else if (channel == 6)
			SendServerToPlayerChat_SilentShout(get_nws_message(), player_to->pl_id, from_id, chat);
		else if (channel == 7)
			SendServerToPlayerChat_DMSilentShout(get_nws_message(), player_to->pl_id, from_id, chat);
		else if (channel == 8)
			SendServerToPlayerChat_DMWhisper(get_nws_message(), player_to->pl_id, from_id, chat);
		else if (channel == 9)
			SendServerToPlayerChat_ServerTell(get_nws_message(), player_to->pl_id, chat);
		else if (channel == 10)
			SendServerToPlayerChat_Whisper(get_nws_message(), player_to->pl_id, from_id, chat);
	}
}
VM_FUNC_NEW(AddChatSource, 250)
{
	CGameObject* o = GetGameObject(vm_pop_object());
	int channel = vm_pop_int();
	std::string chat_message = vm_pop_string();
	if (o)
	{
		server::send_udp_message(boost::str(boost::format("add_chat_source %1% %2%")
        % chat_message
        % channel), 5122);
	}
}
VM_FUNC_NEW(AddChatTarget, 251)
{
	int sf_from_palyer_id = vm_pop_int();
	int sf_to_player_id = vm_pop_int();
	CExoString chat_message = vm_pop_string();
	if (chat_message.text)
	{
		server::send_udp_message(boost::str(boost::format("add_target %1% %2% %3%")
        % sf_from_palyer_id
        % sf_to_player_id
		% sf_to_player_id), 5122);
	}
}
VM_FUNC_NEW(ReplyToWebClient, 252)
{
	int sf_player_id = vm_pop_int();
	CExoString reply_msg = vm_pop_string();
	if (reply_msg.text)
	{
		on_chat_target_update_send_udp(sf_player_id, reply_msg.text);
	}
}
VM_FUNC_NEW(SendWebClientCommand, 539)
{
    int player_id = vm_pop_int();
    std::string tag = vm_pop_string();
    std::string params = vm_pop_string();
    uint32_t duration = vm_pop_int();
    if (duration > 30) duration = 30;
    std::string reason = vm_pop_string();
    mysql_admin->query(boost::str(boost::format("INSERT INTO web_clients_cmd(player_id,tag,params,expire) VALUES(%1%,%2%,%3%,DATE_ADD(NOW(),INTERVAL %4% SECOND))")
        % player_id
        % to_sql(tag)
		% to_sql(params)
		% duration));
    on_chat_target_update_send_udp(player_id, reason.c_str());
}

}
}
