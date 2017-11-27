#pragma once

#include "core.h"

#include <map>

namespace nwnx { namespace server {

extern char last_fetchmsg_ip[16];

int send_udp_message(const std::string& message, uint16_t port);
void send_message_to_master_server(const char* msg, uint32_t len);

extern int current_server_port;
int get_current_server_vault_id();
const char* get_nwn_folder();

struct NWSERVER_INSTANCE
{
	int port;
	int using_id;
	std::string name;
	std::string prefix;
	int share_player_list;
	int default_hakset;
	int show_in_list;
	NWSERVER_INSTANCE() : port(0), using_id(0), share_player_list(0), default_hakset(-1), show_in_list(1) {}
};

extern NWSERVER_INSTANCE* current_nwserver_instance;

}
}
