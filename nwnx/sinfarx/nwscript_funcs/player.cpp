#include "nwscript_funcs.h"
#include "../player.h"
#include "../mysql.h"
#include "../server.h"

using namespace nwnx::player;
using namespace nwnx::mysql;
using namespace nwnx::server;

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <boost/format.hpp>
#include <boost/lexical_cast.hpp>

namespace {

void (*ClosePlayerStore)(CNWSPlayerStoreGUI*, CNWSPlayer*, int) = (void (*)(CNWSPlayerStoreGUI*, CNWSPlayer*, int))0x0808067c;
VM_FUNC_NEW(ClosePlayerStore, 429)
{
	CNWSPlayer* player = get_player_by_game_object_id(vm_pop_object());
	if (player && player->pl_store_gui)
	{
		ClosePlayerStore(player->pl_store_gui, player, 1);
	}
}

int GetPlayerPort(uint32_t player_id)
{
    void* net_layer_internal = *(void **)((*p_app_manager)->app_server->srv_internal->srv_network);
    void* exo_net = *(void **)((char*)net_layer_internal + 0x4);
    if (!exo_net) return -1;
    void* exo_net_internal = *(void **)exo_net;
    if (!exo_net_internal) return -2;
    /* Yes, this is ugly. But I don't want to describe 4 or 5 nested structures. :) */
    for (uint32_t i = 0; i < 0x60; i++) {
        void* client_struct = (void *)((char *)net_layer_internal + 0xC + i * 0x91C);

        if (*(uint32_t *)((char*)client_struct + 0x8) == 1) {
            if (*(uint32_t *)((char*)client_struct + 0xC) == player_id) {
                uint32_t num = *(uint32_t *)((char*)client_struct + 0x14);
                uint8_t *flag_list = *(uint8_t **)((char*)exo_net_internal + 0x34);
                if (!flag_list || !flag_list[num]) return -3;

                char* net_info_base = *(char**)((char*)exo_net_internal + 0x3c);
                struct sockaddr_in *ip = (struct sockaddr_in *)(net_info_base + num * 16);
                if (!ip) return -4;

                return ip->sin_port;
            }
        }
    }
    return -5;
}
VM_FUNC_NEW(GetPCPort, 507)
{
	CNWSPlayer* player = get_player_by_game_object_id(vm_pop_object());
	vm_push_int(player ? GetPlayerPort(player->pl_id) : 0);
}

int (*DisconnectPlayer)(CNetLayer*, uint32_t, uint32_t, int) = (int (*)(CNetLayer*, uint32_t, uint32_t, int))0x082ad478;
VM_FUNC_NEW(BootPCWithMessage, 508)
{
	CNWSPlayer* player = get_player_by_game_object_id(vm_pop_object());
	uint32_t strref = vm_pop_int();
	if (player)
	{
		DisconnectPlayer((*p_app_manager)->app_server->srv_internal->srv_network, player->pl_id, strref, 1);
	}
}

VM_FUNC_NEW(GetPCFileName, 509)
{
	CNWSPlayer* player = get_player_by_game_object_id(vm_pop_object());
	std::string result;
	if (player)
	{
		result = player->pl_bicfile.to_str();
	}
	vm_push_string(result);
}

VM_FUNC_NEW(GetPlayerArea, 158)
{
	PLAYER_EXTRA* player = (PLAYER_EXTRA*)get_player_by_game_object_id(vm_pop_object());
	uint32_t result = OBJECT_INVALID;
	if (player)
	{
		result = player->loaded_area_id;
	}
	vm_push_object(result);
}

VM_FUNC_NEW(GetPlayerCharacter, 288)
{
	CNWSCreature* creature = GetCreatureById(vm_pop_object());
	uint32_t result = OBJECT_INVALID;
	if (creature)
	{
		CExoLinkedListNode* player_list_node = server_internal->srv_client_list_1->header->first;
		while (player_list_node)
		{
			CNWSPlayer* player = (CNWSPlayer*)player_list_node->data;
			if (player)
			{
				if (player->pl_pc_oid == creature->obj.obj_id || player->pl_oid == creature->obj.obj_id)
				{
					result = player->pl_pc_oid;
					break;
				}
			}
			player_list_node = player_list_node->next;
		}
	}
	vm_push_object(result);
}
VM_FUNC_NEW(GetPlayerControlledCreature, 287)
{
	CNWSCreature* creature = GetCreatureById(vm_pop_object());
	uint32_t result = OBJECT_INVALID;
	if (creature)
	{
		CExoLinkedListNode* player_list_node = server_internal->srv_client_list_1->header->first;
		while (player_list_node)
		{
			CNWSPlayer* player = (CNWSPlayer*)player_list_node->data;
			if (player)
			{
				if (player->pl_pc_oid == creature->obj.obj_id || player->pl_oid == creature->obj.obj_id)
				{
					result = player->pl_oid;
					break;
				}
			}
			player_list_node = player_list_node->next;
		}
	}
	vm_push_object(result);
}

VM_FUNC_NEW(IsPlayerCharacter, 144)
{
	CGameObject* o = GetGameObject(vm_pop_object());
	vm_push_int(o && o->id > OBJECT_INVALID);
}

int (*GetPVPLike)(CNWSCreature*, uint32_t) = (int (*)(CNWSCreature*, uint32_t))0x081138cc;
VM_FUNC_NEW(GetPCLike, 237)
{
	CNWSCreature* source = GetCreatureById(vm_pop_object());
	uint32_t target_id = vm_pop_object();
	vm_push_int(source ? GetPVPLike(source, target_id) : -1);
}

}
