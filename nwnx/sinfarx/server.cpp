#include "server.h"
#include "script_event.h"
#include "player.h"
#include "mysql.h"
#include "nwscript.h"
#include "erf.h"

#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

using namespace nwnx::core;
using namespace nwnx::mysql;
using namespace nwnx::nwscript;

namespace nwnx { namespace server {

int current_server_port = -1;

int get_current_server_vault_id()
{
	return current_server_id&sinfar_vault_id?sinfar_vault_id:current_server_id;
}
const char* get_nwn_folder()
{
	if (current_server_id & sinfar_vault_id)
	{
		return "/sinfar/nwn";
	}
	else
	{
		return ".";
	}
}

std::vector<NWSERVER_INSTANCE> nwserver_instances;
int current_server_index_in_array = 0;

int send_udp_message(const std::string& message, uint16_t port)
{
	if (server_internal->srv_network)
	{
		struct sockaddr_in to_addr;
		to_addr.sin_family = AF_INET;
		to_addr.sin_port = htons(port);
		to_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
		return sendto(server_internal->srv_network->net_internal->exo_net->exo_net_internal->socket, message.c_str(), message.length()+1, 0, (struct sockaddr *)&to_addr, sizeof(to_addr));
	}
	else
	{
		return -1;
	}
}
char last_fetchmsg_ip[16];
void (*GetPlayerAddressData)(CNetLayer*, uint32_t, uint32_t*, uint32_t**, uint32_t**, uint32_t*) = (void (*)(CNetLayer*, uint32_t, uint32_t*, uint32_t**, uint32_t**, uint32_t*))0x082ad54c;
unsigned char d_ret_code_fetchmsg[0x20];
int (*FetchIncomingMessage_Org)(CExoNetInternal*, uint32_t*, char**, uint32_t*) = (int (*)(CExoNetInternal*, uint32_t*, char**, uint32_t*))&d_ret_code_fetchmsg;
int FetchIncomingMessage_HookProc(CExoNetInternal* net_internal, uint32_t* p_con_id, char** p_message, uint32_t* p_msg_len)
{
	*p_con_id = -1;
	int msg_fetched = FetchIncomingMessage_Org(net_internal, p_con_id, p_message, p_msg_len);
	if (!msg_fetched) return false;
	if (*p_con_id >= 20000)
	{
		fprintf(stderr, "FetchIncomingMessage_HookProc: Too many connection:%u by:%s\n", *p_con_id, last_fetchmsg_ip);		
		return false;
	}
	else
	{
		uint32_t player_unknown_data; //always 1 ?
		uint32_t player_ip_data[4];
		uint32_t* p_player_ip_data = &player_ip_data[0];
		uint32_t data_3[4];
		uint32_t* p_data_3 = &data_3[0];
		uint32_t player_port = 0;
		GetPlayerAddressData(server_internal->srv_network, *p_con_id, &player_unknown_data, &p_player_ip_data, &p_data_3, &player_port);
		sprintf(last_fetchmsg_ip, "%d.%d.%d.%d", *(((uint8_t*)p_player_ip_data)), *(((uint8_t*)p_player_ip_data)+1), *(((uint8_t*)p_player_ip_data)+2), *(((uint8_t*)p_player_ip_data)+3));
		//printf("message received msg_len:%d con_id:%u port:%d ip:%s\n", *p_msg_len, *p_con_id, player_port, last_fetchmsg_ip);

		/*printf("fetch:");
		for (int i=0; i<*p_msg_len; i++)
		{
			printf("\\%d", (*p_message)[i]);
		}
		printf("\n");//*/

		if (*p_msg_len > 5 && strncmp(*p_message, "SFM ", 4)==0 && module_loaded)
		{
			if (player_ip_data[0] == 0x100007f)
			{
				char* message = new char[*p_msg_len+1];
				strncpy(message, *p_message, *p_msg_len);
				message[*p_msg_len] = 0;
				if (*p_msg_len > 7 && strncmp(*p_message, "SFM WC ", 7)==0)
				{
					uint32_t sf_player_id;
					if (sscanf(message, "SFM WC %d", &sf_player_id) == 1)
					{
						/*for (clients_players_id_iter=clients_players_id.begin(); clients_players_id_iter!=clients_players_id.end(); clients_players_id_iter++)
						{
							if (clients_players_id_iter->second[1] == sf_player_id) break;
						}
						if (clients_players_id_iter == clients_players_id.end())
						{
							fprintf(stderr, "trying to get chat from and invalid player id:%d\n", sf_player_id);
							send_udp_message("PLAYER_NOT_CONNECTED", player_port);
						}
						else*/
						{
							//this should be done in the nwnx::player module
							player::wc_get_chat_waiting[sf_player_id] = player_port;
						}
					}
				}
				else
				{
					char script_name[17];
					uint32_t caller_id;
					char* params = new char[*p_msg_len];
					if (sscanf(message, "SFM %s %x %[\x01-\xFE\xFF]", script_name, &caller_id, params)>=2)
					{
						nwscript::CScriptArray event_params(new nwscript::CScriptArrayData);
						event_params->push_back(player_port);
						char* tokens = params;
						char* token = tokens;
						while (*tokens)
						{
							if (strncmp(tokens, "\xf0\xf5\xaa", 3)==0)
							{
								*tokens = 0;
								event_params->push_back(token);
								tokens += 3;
								token = tokens;
							}
							else
							{
								tokens++;
							}
						}
						event_params->push_back(token);
						script_event::run(script_name, caller_id, event_params);
					}
					delete[] params;
				}
				delete[] message;
			}
		}
	}
	return msg_fetched;
}

unsigned char d_ret_code_ghbn[0x20];
struct hostent* (*gethostbyname_org)(const char*) = (struct hostent* (*)(const char*))&d_ret_code_ghbn;
char* he_aliases[] = { NULL };
char* he_gamespy_addrs[] = { (char*)"\x0\x0\x0\x0", NULL };
char* he_bioware_addrs[] = { (char*)/*"\x34\x2A\x1B\x43"*/"\xc7\xc1\x98\x1b", NULL };
struct hostent* (*ghbn)(const char*) = NULL;
struct hostent he_gamespy = {
	(char*)"nwn.master.gamespy.com",
	he_aliases,
	2,
	4,
	he_gamespy_addrs
};
struct hostent he_bioware = {
	(char*)"nwmaster.bioware.com",
	he_aliases,
	2,
	4,
	he_bioware_addrs
};
struct hostent* gethostbyname_hook(const char *host)
{
	if (strcmp(host, "nwn.master.gamespy.com") == 0)
	{
		return &he_gamespy;
	}
	else if (strcmp(host, "nwmaster.bioware.com") == 0)
	{
		return &he_bioware;
	}
	return gethostbyname_org(host);
}

uint32_t dos_error_count = 0;
void log_dos_error(const char* fmt, ...)
{
	dos_error_count++;
	if (dos_error_count < 10)
	{
		va_list va_list;
		va_start(va_list, fmt);
		vfprintf(stderr, fmt, va_list);
		va_end(va_list);
	}
}
unsigned char d_ret_code_recvfrom[0x20];
ssize_t (*recvfrom_org)(int, void*, size_t, int, struct sockaddr*, socklen_t*) = (ssize_t (*)(int, void*, size_t, int, struct sockaddr*, socklen_t*))&d_ret_code_recvfrom;
ssize_t recvfrom_hook(int sockfd, void* buf, size_t len, int flags, struct sockaddr* src_addr, socklen_t* addrlen)
{
	ssize_t result_len = recvfrom_org(sockfd, buf, len, flags, src_addr, addrlen);
	int src_ip = ((sockaddr_in*)src_addr)->sin_addr.s_addr;
	if (result_len != -1 && src_ip != 0x100007f)
	{
		if (result_len < 4)
		{
			//log_dos_error("result_len too small:%d\n", result_len);
			result_len = -1;
		}
		else if (memcmp(buf, "M", 1) == 0)
		{
			//TODO: check if there's a player with this IP
		}
		else if (memcmp(buf, "BN", 2) == 0/* || memcmp(buf, "BM", 2) == 0*/)
		{
			if (result_len > 200)
			{
				log_dos_error("BNXX message too big:%d\n", result_len);
				result_len = -1;
			}
		}
		/*else if (memcmp(buf, "BMSR", 4) == 0)
		{
			if (src_ip != 0x1b98c1c7 || result_len != 8)
			{
				log_dos_error("unexpected BMSR len:%d from:%x\n", result_len, src_ip);
				result_len = -1;
			}
		}*/
		else
		{
			//log_dos_error("unknown message received:%s len:%d from:%x\n", (char*)buf, result_len, src_ip);
			result_len = -1;
		}
	}
	return result_len;
}

const int MAX_MESSAGE_LENGTH = 30000;
uint8_t OnMessageArrived_recvfrom_buffer[MAX_MESSAGE_LENGTH];
size_t OnMessageArrived_recvfrom(int sockfd, void *buf, size_t len, int flags, struct sockaddr *src_addr, socklen_t *addrlen)
{
	return recvfrom_hook(sockfd, OnMessageArrived_recvfrom_buffer, (((struct sockaddr_in*)src_addr)->sin_addr.s_addr == 0x100007f ? MAX_MESSAGE_LENGTH : len), flags, src_addr, addrlen);
}
int (*StoreMessage)(CExoNetInternal*, uint8_t*, uint32_t, uint32_t) = (int (*)(CExoNetInternal*, uint8_t*, uint32_t, uint32_t))0x82c823c;
int OnMessageArrived_StoreMessage(CExoNetInternal* net_internal, uint8_t* message, uint32_t message_len, uint32_t p4)
{
	return StoreMessage(net_internal, OnMessageArrived_recvfrom_buffer, message_len, p4);
}

NWSERVER_INSTANCE* current_nwserver_instance = NULL;

void init()
{
	if (mysql_admin->query("SELECT port,using_id,name,prefix,share_player_list,default_hakset,public FROM servers WHERE active=1"))
	{
		CMySQLRes result = mysql_admin->store_result();
		MYSQL_ROW row;
		int server_index_in_array = 0;
		while ((row = result.fetch_row()) != NULL)
		{
			NWSERVER_INSTANCE nwserver_instance;
			nwserver_instance.port = atoi(row[0]);
			nwserver_instance.using_id = atoi(row[1]);
			nwserver_instance.name = row[2];
			nwserver_instance.prefix = row[3];
			nwserver_instance.share_player_list = atoi(row[4]);
			nwserver_instance.default_hakset = atoi(row[5]);
			nwserver_instance.show_in_list = atoi(row[6]);
			nwserver_instances.push_back(nwserver_instance);
			if (nwserver_instance.port == current_server_port)
			{
				current_server_index_in_array = server_index_in_array;
				current_nwserver_instance = &nwserver_instances.at(server_index_in_array);
			}
			server_index_in_array++;
		}
	}

	//to send external message to the server
	hook_function(0x82c831c, (long)FetchIncomingMessage_HookProc, d_ret_code_fetchmsg, 9);

	//increase the udp messages size
	enable_write(0x082c7822);
	*(uint32_t*)(0x082c7822) = ((uint32_t)OnMessageArrived_recvfrom-(uint32_t)0x082c7826);
	enable_write(0x082c7888);
	*(uint32_t*)(0x082c7888) = ((uint32_t)OnMessageArrived_StoreMessage-(uint32_t)0x082c788c);
	//from 0x308 to MAX_MESSAGE_LENGTH
	enable_write(0x082c5474);
	*(uint32_t*)0x082c5474 = MAX_MESSAGE_LENGTH;
	enable_write(0x82c5600);
	*(uint32_t*)0x82c5600 = MAX_MESSAGE_LENGTH;
	enable_write(0x082c59ed);
	*(uint32_t*)0x082c59ed = MAX_MESSAGE_LENGTH;
	//from internal MAX_MESSAGE_LENGTH to my MAX_MESSAGE_LENGTH
	enable_write(0x0829f32b);
	*(uint32_t*)0x0829f32b = (uint32_t)&MAX_MESSAGE_LENGTH;
	enable_write(0x082c5df0);
	*(uint32_t*)0x082c5df0 = (uint32_t)&MAX_MESSAGE_LENGTH;
	enable_write(0x82c5f90);
	*(uint32_t*)0x82c5f90 = (uint32_t)&MAX_MESSAGE_LENGTH;
	enable_write(0x082c5fc3);
	*(uint32_t*)0x082c5fc3 = (uint32_t)&MAX_MESSAGE_LENGTH;
	enable_write(0x082c6044);
	*(uint32_t*)0x082c6044 = (uint32_t)&MAX_MESSAGE_LENGTH;
	enable_write(0x82c7809);
	*(uint32_t*)0x82c7809 = (uint32_t)&MAX_MESSAGE_LENGTH;
	//not working
	/*enable_write(0x8323c44);
	*(uint32_t*)0x8323c44 = MAX_MESSAGE_LENGTH;//*/
	
	//increase the maximum number of connections
	enable_write(0x082c72b3);
	*(uint32_t*)0x082c72b3 = 20000;
	
	//bypass bad packets
	hook_function(0x0804a984, (long)recvfrom_hook, d_ret_code_recvfrom, 11);

	//redirect the bioware and gamespy servers
	hook_function(0x0804af14, (long)gethostbyname_hook, d_ret_code_ghbn, 11);
}
REGISTER_INIT(init);

VM_FUNC_NEW(GetServerPort, 156)
{
	vm_push_int(current_server_port);
}
VM_FUNC_NEW(GetNWServerInstanceCount, 159)
{
	vm_push_int(nwserver_instances.size());
}
VM_FUNC_NEW(GetNWServerInstancePort, 160)
{
	uint32_t server_index = vm_pop_int();
	if (server_index == 0xFFFFFFFF) server_index = current_server_index_in_array;
	int result = 0;
	if (server_index < nwserver_instances.size())
	{
		result = nwserver_instances.at(server_index).port;
	}
	vm_push_int(result);
}
VM_FUNC_NEW(GetNWServerInstanceName, 161)
{
	uint32_t server_index = vm_pop_int();
	if (server_index == 0xFFFFFFFF) server_index = current_server_index_in_array;
	CExoString result;
	if (server_index < nwserver_instances.size())
	{
		result = nwserver_instances.at(server_index).name.c_str();
	}
	vm_push_string(&result);
}
VM_FUNC_NEW(GetNWServerInstanceSharePlayerList, 162)
{
	uint32_t server_index = vm_pop_int();
	if (server_index == 0xFFFFFFFF) server_index = current_server_index_in_array;
	int result = 0;
	if (server_index < nwserver_instances.size())
	{
		result = nwserver_instances.at(server_index).share_player_list;
	}
	vm_push_int(result);
}
VM_FUNC_NEW(GetNWServerInstanceDefaultHakset, 163)
{
	uint32_t server_index = vm_pop_int();
	if (server_index == 0xFFFFFFFF) server_index = current_server_index_in_array;
	int result = 0;
	if (server_index < nwserver_instances.size())
	{
		result = nwserver_instances.at(server_index).default_hakset;
	}
	vm_push_int(result);
}

VM_FUNC_NEW(GetCurrentServerId, 164)
{
	vm_push_int(current_server_id);
}
VM_FUNC_NEW(GetServervaultId, 165)
{
	vm_push_int(get_current_server_vault_id());
}
VM_FUNC_NEW(GetServerNameById, 166)
{
	int server_id = vm_pop_int();
	CExoString result;
	for (uint32_t server_index=0; server_index<nwserver_instances.size(); server_index++)
	{
		auto server_instance = nwserver_instances.at(server_index);
		if (server_instance.using_id == server_id)
		{
			result = CExoString(server_instance.name);
		}
	}
	vm_push_string(&result);
}
VM_FUNC_NEW(GetPortFromServerId, 167)
{
	int server_id = vm_pop_int();
	vm_push_int(5121+__builtin_ctz(server_id));
}
VM_FUNC_NEW(GetServerIdFromPort, 168)
{
	int server_port = vm_pop_int();
	vm_push_int(1<<(server_port-5121));
}

VM_FUNC_NEW(SetRemotePlayerCount, 380)
{
	if (vm_pop_int()) return;
}
VM_FUNC_NEW(SendUDPMessage, 44)
{
	int sendto_port = vm_pop_int();
	CExoString sendto_message = vm_pop_string();
	if (sendto_message.text == NULL) return;
	int sendto_server_index = sendto_port-5121;
	if (sendto_server_index >= 0 && sendto_server_index < 32)
	{
		int sendto_server_id = (1 << sendto_server_index);
		if (erf::get_current_script_servers() & sendto_server_id)
		{
			send_udp_message(sendto_message, sendto_port);
		}
		else
		{
			fprintf(stderr, "SendUDPMessage: called from a script:%s that can't access the server:%d\n", get_last_script_ran().c_str(), sendto_server_id);
		}
	}
	else
	{
		fprintf(stderr, "SendUDPMessage: invalid server index:%d\n", sendto_server_index);
	}

}


}
}
