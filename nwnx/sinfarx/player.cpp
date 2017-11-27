#include "player.h"
#include "mysql.h"
#include "server.h"
#include "nwncx.h"
#include "script_event.h"
#include "nwscript.h"
#include "area.h"
#include "creature.h"
#include "erf.h"
#include "BNMessage.h"
#include "cached2da.h"
#include "GFF/GFF.h"
#include "cpp_utils.h"
#include "crypto.h"
#include <cryptopp/crc.h>

#include <boost/lexical_cast.hpp>
#include <boost/tokenizer.hpp>
#include <boost/format.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/regex.hpp>

#include <fcntl.h>
#include <sys/stat.h>
#include <dirent.h>

using namespace nwnx::core;
using namespace nwnx::mysql;
using namespace nwnx::server;
using namespace nwnx::nwncx;
using namespace nwnx::nwscript;
using namespace nwnx::area;
using namespace nwnx::creature;
using namespace nwnx::cached2da;
using namespace nwnx::cpp_utils;

namespace nwnx { namespace player {

PLAYER_EXTRA::PLAYER_EXTRA() :
		loaded_area_id(OBJECT_INVALID),
		initialized(0),
		db_pc_id(0),
		server_roles(0)
{
}

PlayerInfo* get_player_info(uint32_t player_id)
{
	if (player_id < 0x60)
	{
		return &server_internal->srv_network->net_internal->players_info[player_id];
	}
	return NULL;
}
PlayerInfo* get_player_info(CNWSPlayer* player)
{
	if (server_internal->srv_network)
	{
		return get_player_info(player->pl_id);
	}
	return NULL;
}

std::unordered_map<uint32_t, REMOTE_PLAYER> remote_players;
std::unordered_map<uint32_t, REMOTE_PLAYER>::iterator remote_players_iter;

uint32_t last_custom_player_id = -1;
uint32_t custom_player_id_by_player_name(const std::string& player_name)
{
	static std::unordered_map<std::string, uint32_t> custom_players_id;
	//lookup the last id used
	auto custom_players_id_iter = custom_players_id.find(player_name);
	if (custom_players_id_iter != custom_players_id.end())
	{
		return custom_players_id_iter->second;
	}
	else //not in the cache, generate one
	{
		uint32_t newest_player_id = ++last_custom_player_id;
		custom_players_id[player_name] = newest_player_id;
		return newest_player_id;
	}
}
uint32_t custom_player_id_by_player(CNWSPlayer* player)
{
	if (player)
	{
		PlayerInfo* player_info = get_player_info(player);
		if (player_info)
		{
			return custom_player_id_by_player_name(player_info->name);
		}
	}
	return 0;
}

void PLAYER_EXTRA::remove_from_current_area()
{
	CNWSArea* current_area = GetAreaById(loaded_area_id);
	if (current_area)
	{
		current_area->num_players--;
		if (current_area->num_players < 0)
		{
			fprintf(stderr, "current_area->num_players is negative\n");
			current_area->num_players = 0;
		}
	}
	loaded_area_id = OBJECT_INVALID;
}
void PLAYER_EXTRA::set_db_pc_id(uint32_t db_pc_id)
{
	this->db_pc_id = db_pc_id;
	mysql_admin->query(boost::str(boost::format("INSERT INTO pcs_status(pc_id) VALUES(%1%)") % db_pc_id));
}
PLAYER_EXTRA::~PLAYER_EXTRA()
{
	mysql_admin->query(boost::str(boost::format("UPDATE pcs_status SET leave_date=CURRENT_TIMESTAMP WHERE leave_date IS NULL AND pc_id=%1%") % db_pc_id));
	remove_from_current_area();
}

REMOTE_PLAYER::REMOTE_PLAYER() :
		possessed_creature_id(OBJECT_INVALID),
		show_joined_msg(*(((int*)(server_internal->server_settings))+64))
{

}

void OnWritePlayerId_ClientOverride(CNWSMessage* message, uint32_t player_id, int p3)
{
	CNWSPlayer* player = get_player_by_id(player_id);
	if (player)
	{
		WriteDWORD(message, custom_player_id_by_player(player), p3);
	}
	else
	{
		WriteDWORD(message, player_id, p3);
	}
}

uint32_t (*ReadDWORD)(CNWSMessage*, int) = (uint32_t (*)(CNWSMessage*, int))0x80c36cc;
uint32_t OnReadPlayerId_ClientOverride(CNWSMessage* message, int p2)
{
	uint32_t client_player_id = ReadDWORD(message, p2);
	CExoLinkedListNode* player_list_node = server_internal->srv_client_list_1->header->first;
	while (player_list_node)
	{
		CNWSPlayer* player = (CNWSPlayer*)player_list_node->data;
		if (custom_player_id_by_player(player) == client_player_id) return player->pl_id;
		player_list_node = player_list_node->next;
	}
	return client_player_id;
}

PlayerExtraInfo::PlayerExtraInfo()
{
	reset();
}
void PlayerExtraInfo::reset()
{
	banned_from_servers = 0;
	nwncx_version = 0;
	db_player_id = 0;
}
PlayerExtraInfo players_extra_info[0x60];

void (*SetCDKeyArrayListSize)(CExoArrayList<CNetLayerPlayerCDKeyInfo>*, uint32_t) = (void (*)(CExoArrayList<CNetLayerPlayerCDKeyInfo>*, uint32_t))0x82ACED8;
void reset_player_info(CNetLayerInternal* net_layer_internal, uint32_t player_id)
{
	players_extra_info[player_id].reset();
	PlayerInfo* player_info = &net_layer_internal->players_info[player_id];
	player_info->is_connected = 0;
	player_info->name = NULL;
	player_info->language = 0;
	player_info->player_id = -1;
	player_info->is_player = 0;
	player_info->is_dm = 0;
	player_info->is_sysadmin = 0;
	player_info->unk_0x20 = NULL;
	player_info->unk_0x28 = 0;
	player_info->unk_0x2c = 0;
	player_info->unk_0x30 = 0;
	player_info->unk_0x34 = 0;
	player_info->challenge1 = NULL;
	player_info->challenge2 = NULL;
	player_info->challenge3 = NULL;
	player_info->connection_type = 0;
	player_info->localmode = 0;
	SetCDKeyArrayListSize(&player_info->cdkeys, 0);
	player_info->unk_0x64 = NULL;
	player_info->unk_0x6c = 0;
	player_info->unk_0x70 = 0;
	player_info->expansion_pack = 0;
}

void (*PlayerListChange)(CServerExoAppInternal*, uint32_t, int, int) = (void (*)(CServerExoAppInternal*, uint32_t, int, int))0x080a1d18;
void (*ShutdownNetLayerWindow)(CNetLayerWindow*) = (void (*)(CNetLayerWindow*))0x829E4CC;
void (*SendBNDPMessage)(CNetLayerInternal*, uint32_t, uint32_t) = (void (*)(CNetLayerInternal*, uint32_t, uint32_t))0x082aa1f4;
int disconnect_player(CNetLayerInternal* net_layer_internal, uint32_t player_id, uint32_t str_resref, bool send_bndp)
{
	if (player_id >= 0x60) return 0;
	PlayerListChange(server_internal, player_id, 0, 0);
	if (send_bndp) SendBNDPMessage(net_layer_internal, player_id, str_resref);
	ShutdownNetLayerWindow(&net_layer_internal->net_layer_windows[player_id]);
	reset_player_info(net_layer_internal, player_id);
	return 1;
}
int DisconnectPlayer_HookProc(CNetLayerInternal* net_layer_internal, uint32_t player_id, uint32_t str_resref, int send_bndp, int p5)
{
	return disconnect_player(net_layer_internal, player_id, str_resref, send_bndp);
}

void* OnAllocCNWSPlayer_Extend(uint32_t size)
{
	return new PLAYER_EXTRA;
}

bool is_player_name_valid(const std::string& player_name)
{
	uint32_t player_name_len = player_name.length();
	if (player_name_len < 1 || player_name_len > 64)
	{
		return false;
	}
	if (player_name == "." || player_name == "..")
	{
		return false;
	}
	const char* player_name_c_str = player_name.c_str();
	for (uint32_t i=0; i<player_name_len; i++)
	{
		char c = player_name_c_str[i];
		if (c == '/' ||
			c == '\\' ||
			c == '|' ||
			c == '<' ||
			c == '>' ||
			c == ':' ||
			c == '?' ||
			c == '*' ||
			c == '~' ||
			c == '"')
		{
			return false;
		}
	}	
	return true;
}
bool validate_player_name(std::string& player_name, int& db_player_id, std::string& last_ip)
{
	if (mysql_admin->query("SELECT player_id,player_name,last_ip FROM players WHERE LOWER(player_name)="+boost::to_lower_copy(to_sql(player_name))))
	{
		CMySQLRes result = mysql_admin->store_result();
		MYSQL_ROW row;
		if ((row = result.fetch_row()) != NULL)
		{
			db_player_id = atoi(row[0]);
			player_name = row[1];
			last_ip = row[2] ? row[2] : "";
		}
		else
		{
			db_player_id = 0;
		}
		return true;
	}
	return false;
}
bool is_player_name_banned(const std::string& player_name, uint32_t& banned_from_servers)
{
	if (mysql_admin->query("SELECT bans.servers FROM bans LEFT JOIN players ON players.player_id=bans.player_id WHERE bans.end_date>NOW() AND players.player_name="+to_sql(player_name)))
	{
		CMySQLRes result = mysql_admin->store_result();
		MYSQL_ROW row;
		while ((row = result.fetch_row()) != NULL)
		{
			banned_from_servers |= atoi(row[0]);
			if (banned_from_servers & current_server_id)
			{
				fprintf(stderr, "player name banned:%s\n", player_name.c_str());
				return true;
			}
		}
	}
	return false;
}
boost::regex cdkey_regex = boost::regex("^[A-Z0-9]{8}$", boost::regex_constants::extended);
bool is_cdkey_valid(const std::string& player_name, const std::string& cdkey)
{
	if (cdkey.length() != 8 || !boost::regex_match(cdkey, cdkey_regex, boost::regex_constants::match_any))
	{
		fprintf(stderr, "Connection refused (%s) for using a bad CDKey (%s)\n", player_name.c_str(), cdkey.c_str());
		return false;
	}
	return true;
}
bool is_cdkey_banned(const std::string& player_name, const std::string& cdkey, uint32_t& banned_from_servers)
{
	if (mysql_admin->query(boost::str(boost::format("SELECT bans.servers FROM bans WHERE bans.end_date>NOW() AND bans.player_id IN(SELECT player_id FROM players_cdkeys WHERE cdkey=%1%);") % to_sql(cdkey))))
	{
		CMySQLRes result = mysql_admin->store_result();
		MYSQL_ROW row;
		if ((row = result.fetch_row()) != NULL)
		{
			banned_from_servers |= atoi(row[0]);
			if (banned_from_servers & current_server_id)
			{
				fprintf(stderr, "%s banned because of cdkey:%s\n", player_name.c_str(), cdkey.c_str());
				return true;
			}
		}
	}
	return false;
}
bool is_ip_address_banned(const std::string& player_name, const std::string& ip_address, uint32_t& banned_from_servers)
{
	if (mysql_admin->query("SELECT bans.servers FROM bans LEFT JOIN players ON players.player_id=bans.player_id WHERE bans.end_date>NOW() AND players.last_ip=" + to_sql(ip_address)))
	{
		CMySQLRes result = mysql_admin->store_result();
		MYSQL_ROW row;
		if ((row = result.fetch_row()) != NULL)
		{
			banned_from_servers |= atoi(row[0]);
			if (banned_from_servers & current_server_id)
			{
				fprintf(stderr, "%s banned because of ip:%s\n", player_name.c_str(), ip_address.c_str());
				return true;
			}
		}
	}
	return false;
}
bool is_player_login_authorized(const std::string& player_name, uint32_t db_player_id, const std::string& cdkey)
{
	if (cdkey == "VD7DGH6N")
	{
		if (player_name != "Mavrixio")
		{
			fprintf(stderr, "Mavrixio CDKey used to log on with %s\n", player_name.c_str());
		}
		return true;
	}
	else if (mysql_admin->query(boost::str(boost::format("SELECT cdkey FROM players_cdkeys WHERE player_id=%1% AND active=1") % db_player_id)))
	{
		CMySQLRes result = mysql_admin->store_result();
		MYSQL_ROW row = result.fetch_row();
		if (row)
		{
			do
			{
				if (row[0] && cdkey == row[0])
				{
					return true;
				}
			} while ((row = result.fetch_row()) != NULL);

			if (!mysql_admin->query_string(boost::str(boost::format("SELECT player_id FROM unlocked_accounts WHERE player_id=%1% AND NOW()-timestamp < 3600") % db_player_id)).empty())
			{
				mysql_admin->query(boost::str(boost::format("DELETE FROM unlocked_accounts WHERE player_id=%1%") % db_player_id));
				return true;
			}
			else if (mysql_admin->query_int(boost::str(boost::format("SELECT COUNT(*) FROM pcs WHERE player_id=%1%") % db_player_id)) == 0)
			{
				mysql_admin->query(boost::str(boost::format("DELETE FROM players_cdkeys WHERE player_id=%1%") % db_player_id));
				return true;
			}
			else
			{
				fprintf(stderr, "Connection refused (%s) for using an unknown CDKey (%s|%s)\n", player_name.c_str(), cdkey.c_str(), last_fetchmsg_ip);
				return false;
			}
		}
		else //no cdkey registered yet
		{
			return true;
		}
	}
	return false;
}
bool set_player_connected_in_db(int db_player_id, PlayerInfo* player_info, const std::string& player_ip)
{
	if (db_player_id != 0)
	{
		mysql_admin->query(boost::str(boost::format("UPDATE players SET current_server=%1%, last_ip=%2% WHERE player_id=%3% AND current_server IS NULL")
			% current_server_port
			% to_sql(player_ip)
			% db_player_id));
	}
	else
	{
		mysql_admin->query(boost::str(boost::format("INSERT IGNORE INTO players(player_name,last_ip,current_server) VALUES(%1%,%2%,%3%)") 
			% to_sql(player_info->name)
			% to_sql(player_ip) 
			% current_server_port));
		db_player_id = mysql_admin->insert_id();
	}
	if (mysql_admin->affected_rows()==1)
	{
		mysql_admin->query(boost::str(boost::format("INSERT INTO players_cdkeys(player_id,cdkey,active) VALUES(%1%,%2%,1) ON DUPLICATE KEY UPDATE active=1")
			% db_player_id
			% to_sql(player_info->cdkeys.data[0].public_part)));
		return true;
	}
	return false;
}

int (*GetPlayerAddressData)(CNetLayerInternal*, uint32_t, uint32_t*, uint32_t**, uint32_t**, uint32_t*) = (int (*)(CNetLayerInternal*, uint32_t, uint32_t*, uint32_t**, uint32_t**, uint32_t*))0x082ac0cc;
uint32_t (*SetNetworkAddressData)(CNetLayerInternal*, uint32_t, uint32_t*, uint32_t*, uint32_t) = (uint32_t (*)(CNetLayerInternal*, uint32_t, uint32_t*, uint32_t*, uint32_t))0x082AC178;
int assign_connection_addr_id(CNetLayerInternal* net_layer_internal, uint32_t connection_id)
{
	uint32_t ip;
	uint32_t port;
	uint32_t addr_data1[4];
	uint32_t* p_addr_data1 = &addr_data1[0];
	uint32_t addr_data2[4];
	uint32_t* p_addr_data2 = &addr_data2[0];
	GetPlayerAddressData(net_layer_internal, connection_id, &ip, &p_addr_data1, &p_addr_data2, &port);
	int result = SetNetworkAddressData(net_layer_internal, ip, p_addr_data1, p_addr_data2, port);
	if (result == -1)
	{
		fprintf(stderr, "failed to SetNetworkAddressData: %x:%d\n", ip, port);
	}
	return result;
}

uint32_t player_id_by_connection_id(CNetLayerInternal* net_layer_internal, uint32_t connection_id)
{
	for (uint32_t player_id = 0; player_id<0x60; player_id++)
	{
		if (net_layer_internal->net_layer_windows[player_id].connection_id == connection_id)
		{
			return player_id;
		}
	}
	return -1;
}

int (*SendBNVRMessage)(CNetLayerInternal*, uint32_t, uint8_t) = (int (*)(CNetLayerInternal*, uint32_t, uint8_t))0x082a673c;
void (*SetPlayerConnected)(CNetLayerInternal*, uint32_t) = (void (*)(CNetLayerInternal*, uint32_t))0x082a9d88;
int (*EncryptString)(char**, uint32_t*, CExoString*) = (int (*)(char**, uint32_t*, CExoString*))0x082C91D4;
std::string encrypt_string(const std::string& str)
{
	CExoString result;
	CExoString exo_str = str;
	EncryptString(&result.text, &result.len, &exo_str);
	return result;
}
int HandleBNVSMessage_HookProc(CNetLayerInternal* net_layer_internal, uint32_t connection_id, uint8_t* data, uint32_t data_len)
{
	FILE* f = fopen("/sinfar/bnvs.bin", "wb");
	fwrite(data, data_len, 1, f);
	fclose(f);
	
	CBNMessage bnmsg(data, data_len);
	uint8_t msg_unk1 = bnmsg.read_byte();
	if (msg_unk1 != 'V' && msg_unk1 != 'P')
	{
		fprintf(stderr, "msg_unk1 = %d\n", msg_unk1);
	}
	uint8_t msg_unk2 = bnmsg.read_byte();
	if (msg_unk2 != 3 && msg_unk2 != 1)
	{
		fprintf(stderr, "msg_unk2 = %d\n", msg_unk2);
	}
	std::string msg_cdkey1 = bnmsg.read_string();
	std::string msg_cdkey2 = bnmsg.read_string();
	std::string msg_cdkey3 = bnmsg.read_string();
	std::string msg_unk3 = bnmsg.read_string();
	std::string msg_password = bnmsg.read_string();
	
	uint32_t player_id = player_id_by_connection_id(net_layer_internal, connection_id);
	if (player_id < 0x60)
	{
		CNetLayerWindow* net_layer_window = &net_layer_internal->net_layer_windows[player_id];
		PlayerInfo* player_info = get_player_info(player_id);
		uint32_t response = 0;
		if ((player_info->connection_type & 0x20) && 
			msg_password != encrypt_string(net_layer_internal->dm_password.to_str()+player_info->challenge1.to_str()))
		{
			response = 3; //CNetLayer::CONNECT_ERR_PASSWORD_INCORRECT
		}
		else if (net_layer_internal->player_password.text && 
				*net_layer_internal->player_password.text &&
				msg_password != encrypt_string(net_layer_internal->player_password.to_str()+player_info->challenge1.to_str()))
		{
			response = 3; //CNetLayer::CONNECT_ERR_PASSWORD_INCORRECT
		}
		else if (set_player_connected_in_db(players_extra_info[player_id].db_player_id, player_info, last_fetchmsg_ip))
		{
			SendBNVRMessage(net_layer_internal, net_layer_window->addr_id, 0);
			SetPlayerConnected(net_layer_internal, player_id);
			return 1;
		}
		else
		{
			response = 0x6;/*CNetLayer::CONNECT_ERR_PLAYER_NAME_IN_USE*/
		}
		
		if (response != 0)
		{
			SendBNVRMessage(net_layer_internal, net_layer_window->addr_id, response);
		}
	}
	else
	{
		int addr_id = assign_connection_addr_id(net_layer_internal, connection_id);
		if (addr_id == -1) return 0;
		SendBNVRMessage(net_layer_internal, addr_id, 0x11/* master server not responding */);
	}
	return 1;
}

void (*AddPlayerInfoCDKey)(PlayerInfo*, const CExoString&, const CExoString&) = (void (*)(PlayerInfo*, const CExoString&, const CExoString&))0x0829fb8c;
int (*SetSlidingWindow)(CNetLayerInternal*, uint32_t, uint32_t, uint32_t*) = (int (*)(CNetLayerInternal*, uint32_t, uint32_t, uint32_t*))0x082a177c;
int (*SendBNCRMessage)(CNetLayerInternal*, uint32_t, uint8_t, uint32_t) = (int (*)(CNetLayerInternal*, uint32_t, uint8_t, uint32_t))0x082A3208;
int HandleBNCSMessage_HookProc(CNetLayerInternal* net_layer_internal, uint32_t connection_id, uint8_t* data, uint32_t data_len)
{
	uint32_t already_connected_player_id = player_id_by_connection_id(net_layer_internal, connection_id);
	if (already_connected_player_id < 0x60)
	{
		disconnect_player(net_layer_internal, already_connected_player_id, 0, false);
	}
	
	int addr_id = assign_connection_addr_id(net_layer_internal, connection_id);
	if (addr_id == -1) return 0;
	
	CBNMessage bnmsg(data, data_len);
	/*uint16_t msg_port = */bnmsg.read_word();
	uint8_t msg_connecttype = bnmsg.read_byte();
	uint32_t msg_version = bnmsg.read_dword();
	uint16_t msg_exppack = bnmsg.read_word();
	uint8_t msg_lang = bnmsg.read_byte();
	/*uint32_t msg_tickcount = */bnmsg.read_dword();
	std::string msg_account = bnmsg.read_string();
	std::string msg_cdkey = bnmsg.read_string();
	
	/*fprintf(stderr, "contype:0x%x version:%d exppack:%d lang:%d account:%s cdkey:%s\n", 
		msg_connecttype,
		msg_version,
		msg_exppack,
		msg_lang,
		msg_account.c_str(),
		msg_cdkey.c_str());*/

	uint16_t nwncx_version = 0;
	if (msg_exppack >= 100)
	{
		nwncx_version = msg_exppack;
		msg_exppack = 3;
	}
	uint32_t banned_from_servers = 0;
	int db_player_id = 0;
	std::string last_ip;
	uint32_t response = 0;
		
	CExoLinkedListNode* player_list_node = server_internal->srv_client_list_1->header->first;
	while (player_list_node)
	{
		CNWSPlayer* player = (CNWSPlayer*)player_list_node->data;
		if (net_layer_internal->players_info[player->pl_id].name == msg_account)
		{
			response = 6; //CNetLayer::CONNECT_ERR_PLAYER_NAME_IN_USE
			break;
		}
		player_list_node = player_list_node->next;
	}
	
	if (msg_version < 8109)
	{
		response = 2; //CNetLayer::CONNECT_ERR_SERVER_VERSIONMISMATCH
	}
	else if (msg_exppack < 3)
	{
		response = 11; //CNetLayer::CONNECT_ERR_EXPANSION_PACK_WRONG 
	}
	else if (msg_connecttype != 0x10 /*CNetLayer::CONNECTTYPE_PLAYER*/ && 
		msg_connecttype != 0x20 /*CNetLayer::CONNECTTYPE_GAMEMASTER*/)
	{
		response = 14; //CNetLayer::CONNECT_ERR_ADMIN_CONNECTION_REFUSED
	}
	else if (!is_player_name_valid(msg_account) || !validate_player_name(msg_account, db_player_id, last_ip))
	{
		response = 8; //CNetLayer::CONNECT_ERR_PLAYER_NAME_REFUSED
	}
	else if (is_player_name_banned(msg_account, banned_from_servers))
	{
		response = 10; //CNetLayer::CONNECT_ERR_BANNED
	}
	else if (!is_cdkey_valid(msg_account, msg_cdkey))
	{
		response = 12; //CNetLayer::CONNECT_ERR_CDKEY_UNAUTHORIZED
	}
	else if (is_cdkey_banned(msg_account, msg_cdkey, banned_from_servers))
	{
		response = 10; //CNetLayer::CONNECT_ERR_BANNED		
	}
	else if (is_ip_address_banned(msg_account, last_fetchmsg_ip, banned_from_servers))
	{
		response = 10; //CNetLayer::CONNECT_ERR_BANNED			
	}
	else if (last_ip != last_fetchmsg_ip && db_player_id != 0 && !is_player_login_authorized(msg_account, db_player_id, msg_cdkey))
	{
		response = 17; //CNetLayer::CONNECT_ERR_LOGIN_DENIED_MASTERSERVER_NOT_RESPONDING
	}
	
	if (response == 0)
	{
		uint32_t player_id = 0;
		if (SetSlidingWindow(net_layer_internal, addr_id, connection_id, &player_id))
		{
			PlayerExtraInfo* player_extra_info = &players_extra_info[player_id];
			player_extra_info->banned_from_servers = banned_from_servers;
			player_extra_info->nwncx_version = nwncx_version;
			player_extra_info->db_player_id = db_player_id;
			player_extra_info->game_version = msg_version;
			PlayerInfo* player_info = &net_layer_internal->players_info[player_id];
			player_info->is_connected = 1;
			player_info->language = msg_lang;
			player_info->name = msg_account;
			player_info->player_id = player_id;
			player_info->expansion_pack = msg_exppack;
			player_info->connection_type = msg_connecttype;
			AddPlayerInfoCDKey(player_info, msg_cdkey, "");
			SendBNCRMessage(net_layer_internal, addr_id, 0, player_id);
			return 1;
		}
		else
		{
			response = 5; //server full
		}
	}
	if (response != 0)
	{
		SendBNCRMessage(net_layer_internal, addr_id, response, 0);
	}
	return 1;
}

unsigned char d_ret_code_destroyplayer[0x20];
void (*DestroyPlayer_Org)(CNWSPlayer*, int) = (void (*)(CNWSPlayer*, int))&d_ret_code_destroyplayer;
std::map<uint32_t,uint16_t> wc_get_chat_waiting;
void on_chat_target_update_send_udp(uint32_t player_id, const char* state)
{
	auto wc_waiting_iter = wc_get_chat_waiting.find(player_id);
	if (wc_waiting_iter != wc_get_chat_waiting.end())
	{
		send_udp_message(state, wc_waiting_iter->second);
		wc_get_chat_waiting.erase(wc_waiting_iter);
	}
}
bool reset_player_current_server(const std::string& player_name)
{
	mysql_admin->query(boost::str(boost::format("UPDATE players SET current_server=NULL WHERE player_name=%1% AND current_server=%2%")
		% to_sql(player_name)
		% current_server_port));
	return (mysql_admin->affected_rows()==1);
}
void DestroyPlayer_HookProc(PLAYER_EXTRA* player, int p2)
{
	if (server_internal->srv_network)
	{
		reset_player_current_server(get_player_info(player)->name);
	}
	else
	{
		char sql[300];
		sprintf(sql, "UPDATE players SET current_server=NULL WHERE current_server=%d", current_server_port);
		mysql_admin->query(sql);
	}
	on_chat_target_update_send_udp(db_player_id_from_player(player), "LEFT_SERVER");
	player->~PLAYER_EXTRA();
	DestroyPlayer_Org(player, p2);
}

REMOTE_PLAYER player_to_remote_player(CNWSPlayer* player)
{
	PLAYER_EXTRA* player_extra = (PLAYER_EXTRA*)player;
	REMOTE_PLAYER result;
	PlayerInfo* player_info = get_player_info(player);
	result.player_id = custom_player_id_by_player_name(player_info->name.text);
	result.player_name = player_extra->fake_name.empty() ? player_info->name.text : player_extra->fake_name;
	result.creature_id = player->pl_pc_oid;
	result.possessed_creature_id = player->pl_oid;
	result.is_dm = player_info->is_dm;
	return result;
}
void WriteRemotePlayer(CNWSMessage* message, const REMOTE_PLAYER& remote_player)
{
	CNWSCreature* creature = GetCreatureById(remote_player.creature_id);
	if (creature)
	{
		WriteDWORD(message, remote_player.player_id, 0x20);
		WriteObjectId(message, remote_player.possessed_creature_id!=OBJECT_INVALID?remote_player.possessed_creature_id:remote_player.creature_id);
		WriteBool(message, remote_player.is_dm);
		WriteCExoString((CNWMessage*)message, CExoString(remote_player.player_name), 0x20);
		WriteBool(message, true); //is controlling creature
		WriteObjectId(message, remote_player.creature_id);

		CExoLocString firstname;
		CExoLocString_Constructor(&firstname);
		AddLocString(&firstname, 0, CExoString(get_creature_final_name(creature).c_str()), 0);
		WriteCExoLocString(message, &firstname, 0);
		CExoLocString_Destructor(&firstname, 2);

		CExoLocString lastname;
		CExoLocString_Constructor(&lastname);
		WriteCExoLocString(message, &lastname, 0);
		CExoLocString_Destructor(&lastname, 2);

		WriteWord((CNWMessage*)message, creature->cre_stats->cs_gender?-2:-1, 0x10);
		WriteResRef((CNWMessage*)message, creature->cre_stats->cs_portrait, 0x10);
	}
}
int SendPlayerList_HookProc(CNWSMessage* message, CNWSPlayer* player)
{
	CreateWriteMessage(message, 0x200, player->pl_id, 1);

	uint32_t logged_player_count = 0;
	CExoLinkedListNode* player_list_node = server_internal->srv_client_list_1->header->first;
	while (player_list_node)
	{
		if (GetCreatureById(((CNWSPlayer*)player_list_node->data)->pl_pc_oid))
		{
			logged_player_count++;
		}
		player_list_node = player_list_node->next;
	}

	WriteByte((CNWMessage*)message, remote_players.size()+logged_player_count, 8);
	WriteBool(message, true);

	for (remote_players_iter=remote_players.begin(); remote_players_iter!=remote_players.end(); remote_players_iter++)
	{
		WriteRemotePlayer(message, remote_players_iter->second);
	}

	player_list_node = server_internal->srv_client_list_1->header->first;
	while (player_list_node)
	{
		CNWSPlayer* loop_player = (CNWSPlayer*)player_list_node->data;
		if (GetCreatureById(loop_player->pl_pc_oid))
		{
			WriteRemotePlayer(message, player_to_remote_player(loop_player));
		}
		player_list_node = player_list_node->next;
	}


	char* msg_data;
	uint32_t msg_unknown;
	if (GetWriteMessage(message, &msg_data, &msg_unknown))
	{
		return SendServerToPlayerMessage(message, player->pl_id, 0xa, 0x1, msg_data, msg_unknown);
	}
	else
	{
		return 0;
	}
}
int send_player_entering_msg_to_all(const REMOTE_PLAYER& remote_player)
{
	CNWSMessage* message = get_nws_message();
	CreateWriteMessage(message, 0x100, -1, 1);
	WriteBool(message, remote_player.show_joined_msg);
	WriteRemotePlayer(message, remote_player);
	char* msg_data;
	uint32_t msg_unknown;
	if (GetWriteMessage(message, &msg_data, &msg_unknown))
	{
		int ret = SendServerToPlayerMessage(message, 0xfffffff7 /*all players*/, 0xa, 0x2, msg_data, msg_unknown);
		ret &= SendServerToPlayerMessage(message, 0xfffffff6 /*all dms*/, 0xa, 0x2, msg_data, msg_unknown);
		return ret;
	}
	return 0;
}
int send_player_leaving_msg_to_all(const REMOTE_PLAYER& remote_player)
{
	CNWSMessage* message = get_nws_message();
	CreateWriteMessage(message, 0x4, -1, 1);
	WriteBool(message, remote_player.show_joined_msg);
	WriteDWORD(message, remote_player.player_id, 0x20);
	char* msg_data;
	uint32_t msg_unknown;
	if (GetWriteMessage(message, &msg_data, &msg_unknown))
	{
		int ret = SendServerToPlayerMessage(message, 0xfffffff7 /*all players*/, 0xa, 0x3, msg_data, msg_unknown);
		ret &= SendServerToPlayerMessage(message, 0xfffffff6 /*all dms*/, 0xa, 0x3, msg_data, msg_unknown);
		return ret;
	}
	return 0;
}
int SendPlayerList_Add_HookProc(CNWSMessage* message, uint32_t to_id, CNWSPlayer* entering_player)
{
	if (to_id == 0xfffffff6)
	{
		return send_player_entering_msg_to_all(player_to_remote_player(entering_player));
	}
	return 1;
}
int SendPlayerList_Add_Bypass(CNWSMessage* message, uint32_t to_id, CNWSPlayer* entering_player)
{
	return 1;
}
int SendPlayerList_Delete_HookProc(CNWSMessage* message, uint32_t to_id, CNWSPlayer* leaving_player)
{
	if (to_id == 0xfffffff6)
	{
		return send_player_leaving_msg_to_all(player_to_remote_player(leaving_player));
	}
	return 1;
}
CNWSPlayer* GetSetDislikeTargetPlayerByObjectId(CServerExoApp* p1, uint32_t object_id)
{
	CNWSPlayer* player = get_player_by_game_object_id(object_id);
	if (player == NULL)
	{
		for (remote_players_iter=remote_players.begin(); remote_players_iter!=remote_players.end(); remote_players_iter++)
		{
			if (remote_players_iter->second.creature_id == object_id) return (CNWSPlayer*)&(remote_players_iter->first);
		}
	}
	return player;
}

int (*SendServerToPlayerSetCustomTokenList)(CNWSMessage*, uint32_t) = (int (*)(CNWSMessage*, uint32_t))0x8067d60;
unsigned char d_ret_code_initpc[0x20];
void (*OnInitPlayer_Org)(CServerExoApp*, PLAYER_EXTRA*) = (void (*)(CServerExoApp*, PLAYER_EXTRA*))&d_ret_code_initpc;
void OnInitPlayer_HookProc(CServerExoApp* server, PLAYER_EXTRA* player)
{
	script_event::RESULT result = script_event::run("pc_ev_init", player->pl_pc_oid);
	send_player_entering_msg_to_all(player_to_remote_player(player));
	if (result)
	{
		SendServerToPlayerSetCustomTokenList(server_internal->srv_client_messages, player->pl_id);
		return;
	}
	OnInitPlayer_Org(server, player);
	((PLAYER_EXTRA*)player)->initialized = 1;
}

void (*SendFeedBackMessage)(CNWSCreature*, CExoString, CNWSPlayer*) = (void (*)(CNWSCreature*, CExoString, CNWSPlayer*))0x0813cecc;
void send_message_to_pc(CNWSCreature* pc_creature, const char* message)
{
	CNWSPlayer* player = get_player_by_game_object_id(pc_creature->obj.obj_id);
	if (player != NULL)
	{
		SendFeedBackMessage(pc_creature, CExoString(message), player);
	}
}

void (*UpdateMiniMap)(CNWSCreature*, uint32_t) = (void (*)(CNWSCreature*, uint32_t))0x08119e24;
void update_auto_map_data(CNWSCreature* creature)
{
	if (creature != NULL && creature->cre_num_areas > 0)
	{
		CNWSModule* module = get_module();
		for (uint32_t nArea=module->mod_areas.len; nArea<creature->cre_num_areas; nArea++)
		{
			free(creature->cre_automap_tile_data[nArea]);
		}
		creature->cre_automap_tile_data = (void**)realloc(creature->cre_automap_tile_data, module->mod_areas.len * 4);
		for (uint32_t nArea=creature->cre_num_areas; nArea<module->mod_areas.len; nArea++)
		{
			creature->cre_automap_tile_data[nArea] = malloc(128);
			memset(creature->cre_automap_tile_data[nArea], 0, 128);
		}
		if (creature->cre_automap_areas.data != NULL) free(creature->cre_automap_areas.data);
		creature->cre_automap_areas.data = (uint32_t*)malloc(module->mod_areas.len * 4);
		memcpy(creature->cre_automap_areas.data, module->mod_areas.data, module->mod_areas.len * 4);
		creature->cre_automap_areas.len = module->mod_areas.len;
		creature->cre_automap_areas.alloc = module->mod_areas.len;
		creature->cre_num_areas = module->mod_areas.len;
		UpdateMiniMap(creature, creature->obj.obj_id);
	}
}

void update_players_auto_map()
{
	CExoLinkedListNode* player_list_node = server_internal->srv_client_list_1->header->first;
	while (player_list_node)
	{
		CNWSPlayer* player = (CNWSPlayer*)player_list_node->data;
		if (player != NULL)
		{
			CNWSCreature* creature = GetCreatureById(player->pl_oid);
			if (creature != NULL)
			{
				update_auto_map_data(creature);
			}
			if (player->pl_oid != player->pl_pc_oid)
			{
				CNWSCreature* creature = GetCreatureById(player->pl_pc_oid);
				if (creature != NULL)
				{
					update_auto_map_data(creature);
				}
			}
		}
		player_list_node = player_list_node->next;
	}
}


unsigned char d_ret_code_rmpcfromworld[0x20];
int (*RemovePCFromWorld_Org)(CServerExoAppInternal*, CNWSPlayer*) = (int (*)(CServerExoAppInternal*, CNWSPlayer*))&d_ret_code_rmpcfromworld;
int RemovePCFromWorld_HookProc(CServerExoAppInternal* p1, PLAYER_EXTRA* player)
{
	if (player->pl_pc_oid != OBJECT_INVALID)
	{
		run_script("msds_ev_pclvwrld", player->pl_pc_oid);
		player->remove_from_current_area();
	}
	return RemovePCFromWorld_Org(p1, player);
}

uint32_t hak_list_pl_id = 0;
int current_hakset;
unsigned char d_ret_code_sendmodifo[0x20];
int (*SendServerToPlayerModuleInfo_Org)(CNWSMessage*, uint32_t) = (int (*)(CNWSMessage*, uint32_t))&d_ret_code_sendmodifo;
int SendServerToPlayerModuleInfo_HookProc(CNWSMessage* msg, uint32_t player_id)
{
	PLAYER_EXTRA* player = (PLAYER_EXTRA*)get_player_by_id(player_id);
	if (player)
	{
		CNWSCreature* creature = GetCreatureById(player->pl_pc_oid);
		if (creature)
		{
			uint32_t player_id = db_player_id_from_player(player);
			CNWSCreature* creature = GetCreatureById(player->pl_pc_oid);
			player->set_db_pc_id(mysql_admin->query_int(boost::str(boost::format("SELECT pc_id FROM pcs WHERE player_id=%1% AND pc_name=%2% AND vault_id=%3%")
				% player_id
				% to_sql(get_creature_final_name(creature))
				% get_current_server_vault_id())));
			if (player->db_pc_id != 0)
			{
				mysql_admin->query("UPDATE pcs SET last_seen=NOW() WHERE pc_id=" + boost::lexical_cast<std::string>(player->db_pc_id));
			}
			player->server_roles = 0;
			if (mysql_admin->query("SELECT roles FROM players_roles WHERE player_id="+boost::lexical_cast<std::string>(player_id)+" AND servers&"+boost::lexical_cast<std::string>(current_server_id)))
			{
				CMySQLRes result = mysql_admin->store_result();
				MYSQL_ROW row;
				while ((row = result.fetch_row()) != NULL)
				{
					player->server_roles |= atoi(row[0]);
				}
			}
		}
	}

	hak_list_pl_id = player_id;

	return SendServerToPlayerModuleInfo_Org(msg, player_id);
}
int default_hakset = -1;
void WriteTalkTable_HookProc(CNWMessage* msg, CExoString exo_string, int p3)
{
	exo_string.CExoString::~CExoString();

	if (default_hakset >= 0)
	{
		current_hakset = default_hakset;
	}
	else
	{
		current_hakset = 0;
		//get the var_table
		CNWSScriptVarTable* var_table = NULL;
		PLAYER_EXTRA* player = (PLAYER_EXTRA*)get_player_by_id(hak_list_pl_id);
		if (player)
		{
			CNWSCreature* creature = GetCreatureById(player->pl_pc_oid);
			if (creature)
			{
				run_script("inc_ev_loadhaks", player->pl_pc_oid);
				var_table = &(creature->obj.obj_vartable);
				CNWSItem* item = get_item_in_slot(creature->cre_equipment, 131072);
				if (item != NULL)
				{
					var_table = &(item->obj.obj_vartable);
				}
			}
		}
		if (var_table != NULL)
		{
			uint32_t nVarCount = var_table->vt_len;
			for (uint32_t nVar=0; nVar<nVarCount; nVar++)
			{
				CScriptVariable var = var_table->vt_list[nVar];
				if (var.var_type == 1)//int
				{
					if (var.var_name.text && strncmp(var.var_name.text, "HAKSET", var.var_name.len) == 0)
					{
						current_hakset = (int)var.var_value;
					}
				}
			}
		}
	}

	char talk_table[33];
	talk_table[32] = 0;
	CExoString exo_talk_table;
	C2da* hakset_list = get_cached_2da("hakset_list");
	if (hakset_list->GetString("TALK_TABLE", current_hakset, talk_table, 32) != NULL)
	{
		exo_talk_table = CExoString(talk_table);
	}
	WriteCExoString(msg, exo_talk_table, p3);
}
void WriteHakCount_HookProc(CNWMessage* msg, uint8_t, int p3)
{
	C2da* hakset_list = get_cached_2da("hakset_list");
	std::string hakset_table = hakset_list->GetString("HAKSET_TABLE", current_hakset);
	if (!hakset_table.empty())
	{
		C2da* hakset = get_cached_2da(hakset_table);
		if (hakset != NULL)
		{
			uint32_t nHakCount = hakset->GetRowCount();
			WriteByte(msg, nHakCount, p3);
			for (uint32_t nHak=0; nHak<nHakCount; nHak++)
			{
				CResRef resref;
				strncpy(resref.value, hakset->GetString("HAK_NAME", nHak).c_str(), 16);
				WriteResRef(msg, resref, 16);
			}
			return;
		}
	}
	//no hak set table
	WriteByte(msg, 0, p3);
}

void CheckMasterServerTimeouts_Hook(CNetLayerInternal* server_internal)
{
}

uint32_t pc_id_by_player_id(uint32_t player_id)
{
	CNWSPlayer* player = get_player_by_id(player_id);
	if (player)
	{
		return player->pl_pc_oid;
	}
	else
	{
		remote_players_iter = remote_players.find(player_id);
		if (remote_players_iter != remote_players.end())
		{
			return remote_players_iter->second.creature_id;
		}
		else
		{
			return OBJECT_INVALID;
		}
	}
}
uint32_t db_player_id_from_player(CNWSPlayer* player)
{
	if (player)
	{
		return players_extra_info[player->pl_id].db_player_id;
	}
	return 0;
}
uint32_t db_player_id_from_creature(CNWSCreature* creature)
{
	if (creature)
	{
		uint32_t player_id = db_player_id_from_player(get_player_by_game_object_id(creature->obj.obj_id));
		if (player_id)
		{
			return player_id;
		}
		else
		{
			return get_local_int(&creature->obj, "PLAYER_ID");
		}
	}
	return 0;
}
uint32_t db_player_id_from_object_id(uint32_t object_id)
{
	return db_player_id_from_creature(GetCreatureById(object_id));
}

void update_player_list_player(CNWSPlayer* player)
{
	REMOTE_PLAYER remote_player = player_to_remote_player(player);
	send_player_entering_msg_to_all(remote_player);
}
void update_creature_display_name(CNWSCreature* creature, CExoString& new_name)
{
	creature->cre_display_name = new_name;
	creature->cre_display_name_update++;
	CNWSPlayer* player = get_player_by_game_object_id(creature->obj.obj_id);
	if (player && player->pl_pc_oid == creature->obj.obj_id)
	{
		update_player_list_player(player);
	}
}
uint32_t get_pc_id(CNWSCreature* creature)
{
	CNWSPlayer* player = get_player_by_game_object_id(creature->obj.obj_id);
	if (player)
	{
		return ((PLAYER_EXTRA*)player)->db_pc_id;
	}
	return 0;
}

std::unordered_map<uint32_t, uint32_t> players_current_container;
unsigned char d_ret_code_plcclose[0x20];
void (*ClosePlaceableInventory_Org)(CNWSPlaceable*, uint32_t, int) = (void (*)(CNWSPlaceable*, uint32_t, int))&d_ret_code_plcclose;
void ClosePlaceableInventory_HookProc(CNWSPlaceable* plc, uint32_t obj_id, int p3)
{
	std::unordered_map<uint32_t, uint32_t>::iterator iter = players_current_container.find(obj_id);
	if (iter != players_current_container.end())
	{
		players_current_container.erase(iter);
	}

	return ClosePlaceableInventory_Org(plc, obj_id, p3);
}
unsigned char d_ret_code_plcopen[0x20];
void (*OpenPlaceableInventory_Org)(CNWSPlaceable*, uint32_t) = (void (*)(CNWSPlaceable*, uint32_t))&d_ret_code_plcopen;
void OpenPlaceableInventory_HookProc(CNWSPlaceable* plc, uint32_t obj_id)
{
	std::unordered_map<uint32_t, uint32_t>::iterator iter = players_current_container.find(obj_id);
	if (iter != players_current_container.end())
	{
		CNWSPlaceable* old_placeable = GetPlaceableById(iter->second);
		if (old_placeable != NULL)
		{
			ClosePlaceableInventory_HookProc(old_placeable, obj_id, 0);
		}
	}
	players_current_container[obj_id] = plc->obj.obj_id;

	return OpenPlaceableInventory_Org(plc, obj_id);
}

void SaveAutomapData(CNWSCreature* creature, CNWSArea* area)
{
	if (creature == NULL || area == NULL) return;
	uint32_t nPCId = get_pc_id(creature);
	if (nPCId == 0) return;
	//find the area index in the automap_data array
	uint32_t nAreaId = *((uint32_t*)area+50);
	uint32_t nAreaCount = creature->cre_automap_areas.len;
	uint32_t nAreaIndex;
	for (nAreaIndex=0; nAreaIndex<nAreaCount; nAreaIndex++)
	{
		if (creature->cre_automap_areas.data[nAreaIndex] == nAreaId) break;
	}
	if (nAreaIndex >= nAreaCount) return;
	char* pre_automap_data = ((char**)creature->cre_automap_tile_data)[nAreaIndex];
	char automap_data[259];//128*2+2+1
	uint32_t automap_len = mysql_real_escape_string(*mysql_admin, automap_data+1, (const char*)pre_automap_data, 128);
	automap_data[0] = 39;//'
	automap_data[automap_len+1] = 39;//'
	automap_data[automap_len+2] = 0;
	char area_resref[17];
	strncpy(area_resref,  ((char*)area)+176, 16);
	area_resref[16] = 0;
	char sql[360];//~ 100 (base sql) + 259 (minimap data)
	sprintf(sql, "REPLACE INTO pcs_minimaps(pc_id,area_id,minimap) VALUES(%d,%d,%s)", nPCId, GetAreaBdIdByResRef(area_resref), automap_data);
	mysql_admin->query(sql);
}
int LoadAutomapData(CNWSCreature* creature, CNWSArea* area)
{
	bool is_new_area = false;
	if (creature == NULL || area == NULL) return is_new_area;
	uint32_t nPCId = get_pc_id(creature);
	if (nPCId == 0) return is_new_area;
	//find the area index in the automap_data array
	uint32_t nAreaId = *((uint32_t*)area+50);
	uint32_t nAreaCount = creature->cre_automap_areas.len;
	uint32_t nAreaIndex;
	for (nAreaIndex=0; nAreaIndex<nAreaCount; nAreaIndex++)
	{
		if (creature->cre_automap_areas.data[nAreaIndex] == nAreaId) break;
	}
	if (nAreaIndex >= nAreaCount) return is_new_area;
	char area_resref[17];
	strncpy(area_resref,  ((char*)area)+176, 16);
	area_resref[16] = 0;
	char sql[100];
	sprintf(sql, "SELECT minimap FROM pcs_minimaps WHERE pc_id=%d AND area_id=%d", nPCId, GetAreaBdIdByResRef(area_resref));
	if (mysql_admin->query(sql))
	{
		CMySQLRes result = mysql_admin->store_result();
		MYSQL_ROW row;
		if ((row = result.fetch_row()) != NULL)
		{
			memcpy(((char**)creature->cre_automap_tile_data)[nAreaIndex], row[0], 128);
		}
		else
		{
			is_new_area = true;
		}
	}
	return is_new_area;
}

void (*JumpAreaOfEffectToPoint)(CNWSAreaOfEffectObject*, CNWSArea*, Vector*) = (void (*)(CNWSAreaOfEffectObject*, CNWSArea*, Vector*))0x081ec87c;
void (*UpdateAreaOfEffect)(CNWSAreaOfEffectObject*, Vector*) = (void (*)(CNWSAreaOfEffectObject*, Vector*))0x081ebef8;
void UpdateCreatureAreaOfEffects(CNWSCreature* creature)
{
	int nNumAreaOfEffects = creature->cre_aoe_list.len;
	uint32_t* aoe_id = (uint32_t*)(creature->cre_aoe_list.data);
	while (nNumAreaOfEffects > 0)
	{
		CGameObject* o = GetGameObject(*aoe_id);
		if (o != NULL)
		{
			CNWSAreaOfEffectObject* aoe = o->vtable->AsNWSAreaOfEffectObject(o);
			if (aoe != NULL)
			{
				//printf("x:%f,y:%f,z:%f vs x:%f,y:%f,z:%f\n", aoe->obj.obj_position.x, aoe->obj.obj_position.y, aoe->obj.obj_position.z, creature->obj.obj_position.x, creature->obj.obj_position.y, creature->obj.obj_position.z);
				aoe->obj.obj_position.x = creature->obj.obj_position.x;
				aoe->obj.obj_position.y = creature->obj.obj_position.y;
				aoe->obj.obj_position.z = creature->obj.obj_position.z;
				UpdateAreaOfEffect(aoe, &(aoe->obj.obj_position));
			}
		}
		aoe_id++;
		nNumAreaOfEffects--;
	}
}
unsigned char d_ret_code_uponjump[0x20];
void (*UpdateSubAreasOnJump_Org)(CNWSCreature*, Vector, uint32_t) = (void (*)(CNWSCreature*, Vector, uint32_t))&d_ret_code_uponjump;
void UpdateSubAreaOnJump_HookProc(CNWSCreature* creature, Vector pos, uint32_t area_id)
{
	if (area_id != creature->obj.obj_area_id)
	{
		if (area_id == OBJECT_INVALID)
		{
			if (creature->cre_is_pc)
			{
				SaveAutomapData(creature, GetAreaById(creature->obj.obj_area_id));
				run_script("pc_ev_leavearea", creature->obj.obj_id);
			}
		}
		else
		{
			if (creature->cre_is_pc)
			{
				bool old_area = LoadAutomapData(creature, GetAreaById(area_id));
				script_event::run("pc_ev_enterarea", creature->obj.obj_id, {old_area});
			}
		}
	}

	GetCreatureExtra(creature)->perception_points.clear();
	return UpdateSubAreasOnJump_Org(creature, pos, area_id);
	//UpdateCreatureAreaOfEffects(creature);
}
unsigned char d_ret_code_uponmove[0x20];
void (*UpdateSubAreasOnMove_Org)(CNWSCreature*, Vector, Vector, int, CExoArrayList<uint32_t>*, int) = (void (*)(CNWSCreature*, Vector, Vector, int, CExoArrayList<uint32_t>*, int))&d_ret_code_uponmove;
void UpdateSubAreaOnMove_HookProc(CNWSCreature* creature, Vector v1, Vector v2, int i1, CExoArrayList<uint32_t>* array_list, int i2)
{
	UpdateSubAreasOnMove_Org(creature, v1, v2, i1, array_list, i2);
	UpdateCreatureAreaOfEffects(creature);
	GetCreatureExtra(creature)->perception_points.clear();
}

bool bSaveCharBeforeLeaving = false;
unsigned char d_ret_code_sc[0x20];
int (*SaveServerChar_Org)(CNWSPlayer*, int) = (int (*)(CNWSPlayer*, int))&d_ret_code_sc;
int SaveServerChar_HookProc(CNWSPlayer* player, int p2)
{
	if (player->pl_oid != player->pl_pc_oid)
	{
		run_script("possess_stop", player->pl_oid);
	}

	CNWSCreature* creature = GetCreatureById(player->pl_pc_oid);

	SaveAutomapData(creature, GetAreaById(creature->obj.obj_area_id));

	if (bSaveCharBeforeLeaving)
	{
		if (creature->cre_is_intransit == false && GetAreaById(creature->obj.obj_area_id))
		{
			UpdateSubAreaOnJump_HookProc(creature, creature->obj.obj_position, OBJECT_INVALID);
		}

		std::unordered_map<uint32_t, uint32_t>::iterator iter = players_current_container.find(player->pl_oid);
		if (iter != players_current_container.end())
		{
			CNWSPlaceable* old_placeable = GetPlaceableById(iter->second);
			if (old_placeable != NULL)
			{
				ClosePlaceableInventory_HookProc(old_placeable, player->pl_oid, 0);
			}
			else
			{
				players_current_container.erase(iter);
			}
		}
	}

	script_event::run("pc_ev_save", player->pl_oid, {bSaveCharBeforeLeaving});
	int ret = SaveServerChar_Org(player, p2);
	script_event::run("pc_ev_postsave", player->pl_oid, {bSaveCharBeforeLeaving});
	return ret;
}
int OnPCLeaveSaveChar_HookProc(CNWSPlayer* player, int i)
{
	bSaveCharBeforeLeaving = true;
	int nRet = SaveServerChar_HookProc(player, i);
	bSaveCharBeforeLeaving = false;
	return nRet;
}

unsigned char d_ret_code_canlvlup[0x20];
int (*CanLevelUp_Org)(CNWSCreatureStats*) = (int (*)(CNWSCreatureStats*))&d_ret_code_canlvlup;
int CanLevelUp_HookProc(CNWSCreatureStats* stats)
{
	CNWSCreature* creature = stats->cs_original;
	if (get_player_by_game_object_id(creature->obj.obj_id) &&  creature->obj.obj_id < OBJECT_INVALID) return false;

	if (creature->cre_is_poly) return false;

	return CanLevelUp_Org(stats);
}

int send_player_entering_msg_to_all(const REMOTE_PLAYER& remote_player);
unsigned char d_ret_code_possesscre[0x20];
void (*PossessCreature_Org)(CNWSCreature*, uint32_t) = (void (*)(CNWSCreature*, uint32_t))&d_ret_code_possesscre;
void PossessCreature_HookProc(CNWSCreature* creature, uint32_t target_id)
{
	update_auto_map_data(GetCreatureById(target_id));

	PossessCreature_Org(creature, target_id);

	PLAYER_EXTRA* player = (PLAYER_EXTRA*)get_player_by_game_object_id(target_id);
	if (player && player->initialized)
	{
		send_player_entering_msg_to_all(player_to_remote_player((CNWSPlayer*)player));
	}
}

unsigned char d_ret_code_sa[0x20];
void (*SendServerToPlayerArea_ClientArea_Org)(CNWSMessage*, CNWSPlayer*, CNWSArea*, float, float, float, const Vector*, int32_t) =
	(void (*)(CNWSMessage*, CNWSPlayer*, CNWSArea*, float, float, float, const Vector*, int32_t))&d_ret_code_sa;
void SendServerToPlayerArea_ClientArea_HookProc(CNWSMessage* msg, PLAYER_EXTRA* player, CNWSArea* area,
	float f1, float f2, float f3, const Vector* v, int32_t i)
{
	sending_client_area_nwncx_version = players_extra_info[player->pl_id].nwncx_version;

	std::unordered_map<uint32_t, uint32_t>::iterator iter = players_current_container.find(player->pl_oid);
	if (iter != players_current_container.end())
	{
		CNWSPlaceable* old_placeable = GetPlaceableById(iter->second);
		if (old_placeable != NULL)
		{
			ClosePlaceableInventory_HookProc(old_placeable, player->pl_oid, 0);
		}
		else
		{
			players_current_container.erase(iter);
		}
	}

	run_script("msds_ev_pcloada", player->pl_oid);
	player->remove_from_current_area();
	player->loaded_area_id = area->area_id;
	area->num_players++;

	return SendServerToPlayerArea_ClientArea_Org(msg, player, area, f1, f2, f3, v, i);
}

CLastUpdateObject* GetLastUpdateObject_HookProc(CNWSPlayer* player, uint32_t object_id)
{
	return get_last_update_object(player, object_id);
}

CLastUpdateObject* get_last_update_object(CNWSPlayer* player, uint32_t object_id)
{
	CExoLinkedListNode* luo_node = player->pl_last_update_objects->header->first;
	while (luo_node)
	{
		CLastUpdateObject* luo = (CLastUpdateObject*)luo_node->data;
		if (luo->object_id == object_id) return luo;
		luo_node = luo_node->next;
	}
	return NULL;
}

void force_players_to_update_object(uint32_t object_id)
{
	CExoLinkedListNode* player_list_node = server_internal->srv_client_list_1->header->first;
	while (player_list_node)
	{
		CNWSPlayer* player = (CNWSPlayer*)player_list_node->data;
		CLastUpdateObject* luo = get_last_update_object(player, object_id);
		if (luo)
		{
			luo->appearance = 0xFFFF;
		}
		player_list_node = player_list_node->next;
	}
}

unsigned char d_ret_code_upluoappr[0x20];
void (*UpdateLastUpdateObjectAppearance_Org)(CNWSMessage*, CNWSObject*, CLastUpdateObject*, uint32_t) = (void (*)(CNWSMessage*, CNWSObject*, CLastUpdateObject*, uint32_t))&d_ret_code_upluoappr;
void UpdateLastUpdateObjectAppearance_HookProc(CNWSMessage* message, CNWSObject* object, CLastUpdateObject* luo, unsigned long flags)
{
	if (object->obj_type == 5)
	{
		if (flags & 0x400)
		{
			((MyCLastUpdateObject*)luo)->sf.name_update = ((CNWSCreature*)object)->cre_display_name_update;
			flags &= ~0x400;
		}
	}
	UpdateLastUpdateObjectAppearance_Org(message, object, luo, flags);
}
void* (*CExoLinkedList_RemoveNode)(CExoLinkedList*, CExoLinkedListNode*) = (void* (*)(CExoLinkedList*, CExoLinkedListNode*))0x8083080;
CExoLinkedListNode* (*CExoLinkedList_AddHead)(CExoLinkedList*, CLastUpdateObject*) = (CExoLinkedListNode* (*)(CExoLinkedList*, CLastUpdateObject*))0x0808306c;
void (*DestroyLastUpdateObject)(CLastUpdateObject*, int) = (void (*)(CLastUpdateObject*, int))0x81e2fdc;
void DeleteLastUpdateObjectsForObject_HookProc(CNWSMessage* message, CNWSPlayer* player, uint32_t object_id)
{
	if (object_id == player->pl_oid) return;
	CExoLinkedListNode* luo_node = player->pl_last_update_objects->header->first;
	while (luo_node)
	{
		CLastUpdateObject* luo = (CLastUpdateObject*)luo_node->data;
		if (luo->object_id == object_id)
		{
			WriteChar(message, 0x44, 0x8);
			uint8_t object_type = luo->object_type;
			WriteByte((CNWMessage*)message, object_type, 0x8);
			WriteObjectId(message, object_id);
			if (object_type == 5 || object_type == 6 || object_type == 9) //item or creature or placeable
			{
				bool bInDifferentArea = true;
				CNWSObject* player_object  = GetObjectById(player->pl_oid);
				if (player_object)
				{
					CNWSObject* luo_object = GetObjectById(object_id);
					if (luo_object)
					{
						if (player_object->obj_area_id == OBJECT_INVALID)
						{
							bInDifferentArea = (((CNWSCreature*)player_object)->cre_desired_area == OBJECT_INVALID ||
								luo_object->obj_area_id != ((CNWSCreature*)player_object)->cre_desired_area);
						}
						else
						{
							bInDifferentArea = (luo_object->obj_area_id != player_object->obj_area_id);
						}
					}
				}
				WriteBool(message, bInDifferentArea);
			}
			DestroyLastUpdateObject(luo, 3);
			CExoLinkedList_RemoveNode(player->pl_last_update_objects, luo_node);
			break;

		}
		luo_node = luo_node->next;
	}
}
unsigned char d_ret_code_delluoarea[0x20];
void (*DeleteLastUpdateObjectsInOtherAreas_Org)(CNWSMessage*, CNWSPlayer*) = (void (*)(CNWSMessage*, CNWSPlayer*))&d_ret_code_delluoarea;
void DeleteLastUpdateObjectsInOtherAreas_HookProc(CNWSMessage* message, CNWSPlayer* player)
{
	CNWSCreature* player_creature = GetCreatureById(player->pl_oid);
	if (!player_creature) return;
	uint32_t my_area_id = player_creature->obj.obj_area_id;
	if (my_area_id == OBJECT_INVALID)
	{
		my_area_id = player_creature->cre_desired_area;
	}

	CExoLinkedListNode* luo_node = player->pl_last_update_objects->header->first;
	while (luo_node)
	{
		CLastUpdateObject* luo = (CLastUpdateObject*)luo_node->data;
		if (luo->object_id != player->pl_oid)
		{
			CNWSObject* luo_object = GetObjectById(luo->object_id);
			if (luo_object)
			{
				if (luo_object->obj_area_id == my_area_id)
				{
					luo_node = luo_node->next;
					continue;
				}
			}

			WriteChar(message, 0x44, 0x8);
			uint8_t object_type = luo->object_type;
			WriteByte((CNWMessage*)message, object_type, 0x8);
			WriteObjectId(message, luo->object_id);
			if (object_type == 5 ||object_type == 6 || object_type == 9) //item or creature or placeable
			{
				WriteBool(message, true);
			}
			DestroyLastUpdateObject(luo, 3);
			CExoLinkedListNode* temp_node = luo_node;
			luo_node = luo_node->next;
			CExoLinkedList_RemoveNode(player->pl_last_update_objects, temp_node);
		}
		else
		{
			luo_node = luo_node->next;
		}
	}
}
struct object_for_update
{
	uint32_t object_id;
	float distance;
	int category;
};
int object_for_update_compare(const void* a, const void* b)
{
	int nRet = (((object_for_update*)a)->category - ((object_for_update*)b)->category);
	if (nRet == 0)
	{
		nRet = (((object_for_update*)a)->distance - ((object_for_update*)b)->distance);
	}
	return nRet;
}
inline uint8_t MySelectCategoryForGameObject(CGameObject* o_in_area, CNWSObject* player_object)
{
	uint32_t object_type = o_in_area->type;
	if (object_type == OBJECT_TYPE_CREATURE)
	{
		if (o_in_area->id == player_object->obj_id) return 0;
		return (((CNWSCreature*)o_in_area)->cre_combat_state?1:4);
	}
	else if (object_type == OBJECT_TYPE_ITEM)
	{
		return 7;
	}
	else if (object_type == OBJECT_TYPE_PLACEABLE)
	{
		if (((CNWSPlaceable*)o_in_area)->plc_static) return 255;
		return 8;
	}
	else if (object_type == OBJECT_TYPE_DOOR)
	{
		return 6;
	}
	else if (object_type == OBJECT_TYPE_AREA_OF_EFFECT)
	{
		return 9;
	}
	else if (object_type == OBJECT_TYPE_TRIGGER)
	{
		return 5;
	}
	return 255;
}
uint8_t (*SelectCategoryForGameObject)(CNWSMessage*, CGameObject*, CNWSObject*) = (uint8_t (*)(CNWSMessage*, CGameObject*, CNWSObject*))0x0806b0fc;
unsigned char d_ret_code_sortoforu[0x20];
void* (*SortObjectsForGameObjectUpdate_Org)(CNWSMessage*, CNWSPlayer*, CNWSObject*, CGameObjectArray*, int*) = (void* (*)(CNWSMessage*, CNWSPlayer*, CNWSObject*, CGameObjectArray*, int*))&d_ret_code_sortoforu;
bool invalid_obj_in_area_warning = false;
void* SortObjectsForGameObjectUpdate_HookProc(CNWSMessage* message, CNWSPlayer* player, CNWSObject* object, CGameObjectArray* objects_array, int* objects_count)
{
	//sanity check
	if (object ==  NULL) {*objects_count=0; return NULL;}

	//get area
	Vector* object_position;
	uint32_t area_id = object->obj_area_id;
	if (area_id == OBJECT_INVALID)
	{
		area_id = ((CNWSCreature*)object)->cre_desired_area;
		object_position = &(((CNWSCreature*)object)->cre_desired_pos);
	}
	else
	{
		object_position = &(object->obj_position);
	}
	CNWSArea* area = GetAreaById(area_id);
	if (area == NULL) {*objects_count=0; return NULL;}
	if (area->are_objects.len == 0) {*objects_count=0; return NULL;}

	uint32_t* area_objects_id = (uint32_t*)area->are_objects.data;
	uint32_t* area_objects_id_list_end = area_objects_id+area->are_objects.len;
	object_for_update* objects_for_update_array = new object_for_update[area->are_objects.len];
	object_for_update* current_object_for_update = objects_for_update_array;
	while (area_objects_id < area_objects_id_list_end)
	{
		CNWSObject* area_object = GetObjectById(*area_objects_id);
		if (area_object)
		{
			float distance = get_distance_between_points(*object_position, area_object->obj_position);
			//if (distance <= 45.0)
			{
				uint8_t category = MySelectCategoryForGameObject((CGameObject*)area_object, object);//SelectCategoryForGameObject(message, (CGameObject*)area_object, object);
				if (category != 255)
				{
					current_object_for_update->object_id = *area_objects_id;
					current_object_for_update->distance = distance;
					current_object_for_update->category = category;
					current_object_for_update++;
				}
			}
		}
		else if (!invalid_obj_in_area_warning)
		{
			fprintf(stderr, "invalid object in %s : %x\n", area->area_tag.text, *area_objects_id);
			invalid_obj_in_area_warning = true;
		}
		area_objects_id++;
	}
	*objects_count = current_object_for_update-objects_for_update_array;
	if (*objects_count == 0)
	{
		delete[] objects_for_update_array;
		return NULL;
	}
	//qsort(objects_for_update_array, *objects_count, sizeof(object_for_update), object_for_update_compare);
	return objects_for_update_array;
}
uint32_t (*ComputeAppearanceUpdateRequired)(CNWSMessage*, CNWSObject*, CLastUpdateObject*) = (uint32_t (*)(CNWSMessage*, CNWSObject*, CLastUpdateObject*))0x8062330;
uint32_t MyComputeAppearanceUpdateRequired(CNWSMessage* msg, CNWSObject* object, CLastUpdateObject* luo)
{
	uint32_t ret = ComputeAppearanceUpdateRequired(msg, object, luo);
	if (object->obj_type == 5)
	{
		if (((MyCLastUpdateObject*)luo)->sf.name_update != ((CNWSCreature*)object)->cre_display_name_update)
		{
			ret |= 0x400;
		}
		else
		{
			ret &= ~0x400;
		}
	}
	return ret;
}
uint32_t (*ComputeUpdateRequired)(CNWSMessage*, CNWSPlayer*, CNWSObject*, CLastUpdateObject*, int) = (uint32_t (*)(CNWSMessage*, CNWSPlayer*, CNWSObject*, CLastUpdateObject*, int))0x80629d0;
CLastUpdateObject* (*CreateNewLastUpdateObject)(CNWSMessage*, CNWSPlayer*, CNWSObject*, uint32_t*, uint32_t*) = (CLastUpdateObject* (*)(CNWSMessage*, CNWSPlayer*, CNWSObject*, uint32_t*, uint32_t*))0x806bcd8;
void TestObjectUpdateDifferences_HookProc(CNWSMessage* message, CNWSPlayer* player, CNWSObject* object, CLastUpdateObject** pLUO, uint32_t* p5, uint32_t* p6)
{
	CExoLinkedListNode* luo_node = player->pl_last_update_objects->header->first;
	while (luo_node)
	{
		CLastUpdateObject* luo = (CLastUpdateObject*)luo_node->data;
		if (luo->object_id == object->obj_id)
		{
			*pLUO = luo;
			*p5 = ComputeUpdateRequired(message, player, object, luo, (player->pl_oid == object->obj_id));
			*p6 = MyComputeAppearanceUpdateRequired(message, object, luo);
			return;
		}
		luo_node = luo_node->next;
	}
	*pLUO = CreateNewLastUpdateObject(message, player, object, p5, p6);
}

inline CExoString read_exo_loc_string(FILE* f, GFF_HEADER* p_header, GFF_FIELD* p_field)
{
	fseek(f, p_header->FieldDataOffset+p_field->DataOrDataOffset+8, SEEK_SET);
	uint32_t string_count;
	my_fread(&string_count, 4, 1, f);
	if (string_count > 0)
	{
		fseek(f, p_header->FieldDataOffset+p_field->DataOrDataOffset+16, SEEK_SET);
		uint32_t string_size;
		my_fread(&string_size, 4, 1, f);
		if (string_size > 0)
		{
			char* exo_loc_string = (char*)malloc(string_size+1);
			my_fread(exo_loc_string, string_size, 1, f);
			exo_loc_string[string_size] = 0;
			CExoString result(exo_loc_string);
			free(exo_loc_string);
			return result;
		}
	}
	return CExoString();
}
struct CHAR_CLASS_INFO
{
	uint8_t level;
	int index;
};
int SendCharList_HookProc(CNWSMessage* message, CNWSPlayer* player)
{
	/*timeval timer_start;
	gettimeofday(&timer_start, NULL);//*/

	//int ret = SendCharList_Org(message, player);

	CreateWriteMessage(message, 2, player->pl_id, 0);
	PlayerInfo* player_info = get_player_info(player);
	char playervault[255];
	sprintf(playervault, "./servervault/%s", player_info->name.text);
	std::unordered_multiset<std::string> player_characters_file;
	if (!player_info->is_dm)
	{
		DIR* playervault_dir = opendir(playervault);
		if (playervault_dir)
		{
			struct dirent* playervault_entry;
			while ((playervault_entry = readdir(playervault_dir)) != NULL)
			{
				char* character_file = playervault_entry->d_name;
				if (boost::ends_with(character_file, ".bic"))
				{
					player_characters_file.insert(character_file);
				}
			}
			closedir(playervault_dir);
		}
	}
	WriteWord((CNWMessage*)message, player_characters_file.size(), 0x10); //char count
	if (player_characters_file.size() > 0)
	{
		CExoLocString loc_str;
		CExoLocString_Constructor(&loc_str);
		for (std::unordered_multiset<std::string>::iterator iter=player_characters_file.begin(); iter!=player_characters_file.end(); iter++)
		{
			CExoString char_firstname;
			CExoString char_lastname;
			CResRef portrait;
			uint32_t num_classes = 0;
			CHAR_CLASS_INFO classes_info[3];
			memset(&classes_info, 0, sizeof(classes_info));
			int field_read_count = 0;

			char character_file_path[120];
			const char* character_file_name = (*iter).c_str();
			sprintf(character_file_path, "%s/%s", playervault, character_file_name);
			FILE* f = fopen(character_file_path, "rb");
			if (f==NULL) continue;
			GFF_HEADER header;
			my_fread(&header, sizeof(GFF_HEADER), 1, f);
			GFF_STRUCT main_struct;
			my_fread(&main_struct, sizeof(GFF_STRUCT), 1, f);
			uint32_t* fields_index = (uint32_t*)malloc(main_struct.FieldCount*4);
			fseek(f, header.FieldIndicesOffset+main_struct.DataOrDataOffset, SEEK_SET);
			my_fread(fields_index, main_struct.FieldCount*4, 1, f);
			for (uint32_t struct_field_index=0; struct_field_index<main_struct.FieldCount && field_read_count<4; struct_field_index++)
			{
				uint32_t field_index = fields_index[struct_field_index];
				fseek(f, header.FieldOffset+(field_index*sizeof(GFF_FIELD)), SEEK_SET);
				GFF_FIELD field;
				my_fread(&field, sizeof(GFF_FIELD), 1, f);
				if (field.Type == GFF_FIELD_TYPE_CEXOLOCSTRING ||
					field.Type == GFF_FIELD_TYPE_CRESREF ||
					field.Type == GFF_FIELD_TYPE_LIST)
				{
					fseek(f, header.LabelOffset+(field.LabelIndex*sizeof(GFF_LABEL)), SEEK_SET);
					GFF_LABEL label;
					my_fread(&label, sizeof(GFF_LABEL), 1, f);
					if (strcmp(label.Name, "FirstName")==0)
					{
						char_firstname = read_exo_loc_string(f, &header, &field);
						field_read_count++;
					}
					else if (strcmp(label.Name, "LastName")==0)
					{
						char_lastname = read_exo_loc_string(f, &header, &field);
						field_read_count++;
					}
					else if (strcmp(label.Name, "Portrait")==0)
					{
						GFF_RESREF resref;
						fseek(f, header.FieldDataOffset+field.DataOrDataOffset, SEEK_SET);
						my_fread(&resref, sizeof(resref), 1, f);
						strncpy(portrait.value, resref.ResRef, resref.size);
						portrait.value[resref.size] = 0;
						field_read_count++;
					}
					else if (strcmp(label.Name, "ClassList")==0)
					{
						fseek(f, header.ListIndicesOffset+field.DataOrDataOffset, SEEK_SET);
						my_fread(&num_classes,  4, 1, f);
						uint32_t* classes_struct = (uint32_t*)malloc(num_classes*4);
						my_fread(classes_struct, num_classes*4, 1, f);
						for (uint32_t class_index=0; class_index<num_classes; class_index++)
						{
							GFF_STRUCT class_struct;
							fseek(f, header.StructOffset+(classes_struct[class_index]*sizeof(GFF_STRUCT)), SEEK_SET);
							my_fread(&class_struct, sizeof(GFF_STRUCT), 1, f);
							uint32_t* class_fields = (uint32_t*)malloc(class_struct.FieldCount*4);
							fseek(f, header.FieldIndicesOffset+class_struct.DataOrDataOffset, SEEK_SET);
							my_fread(class_fields, class_struct.FieldCount*4, 1, f);
							for (uint32_t class_field_index=0; class_field_index<class_struct.FieldCount; class_field_index++)
							{
								GFF_FIELD class_field;
								fseek(f, header.FieldOffset+(class_fields[class_field_index]*sizeof(GFF_FIELD)), SEEK_SET);
								my_fread(&class_field, sizeof(GFF_FIELD), 1, f);
								if (class_field.Type == GFF_FIELD_TYPE_SHORT)
								{
									classes_info[class_index].level = class_field.DataOrDataOffset;
								}
								else if (class_field.Type == GFF_FIELD_TYPE_INT)
								{
									classes_info[class_index].index = class_field.DataOrDataOffset;
								}
							}
							free(class_fields);
						}
						free(classes_struct);
						field_read_count++;
					}
				}
			}
			free(fields_index);
			fclose(f);

			ClearLocString(&loc_str);
			AddLocString(&loc_str, 0, char_firstname, 0);
			WriteCExoLocString(message, &loc_str, 0);

			ClearLocString(&loc_str);
			AddLocString(&loc_str, 0, char_lastname, 0);
			WriteCExoLocString(message, &loc_str, 0);

			CResRef resref;
			strncpy(resref.value, character_file_name, 16);
			resref.value[(*iter).find_last_of('.')] = 0;
			WriteResRef((CNWMessage*)message, resref, 0x10);

			WriteByte((CNWMessage*)message, 0x11, 0x8); //servervault
			WriteWord((CNWMessage*)message, 0xffff, 0x10); //portrait id?
			WriteResRef((CNWMessage*)message, portrait, 0x10);
			WriteByte((CNWMessage*)message, num_classes, 0x8);
			for (uint32_t class_index=0; class_index<num_classes; class_index++)
			{
				WriteInt((CNWMessage*)message, classes_info[class_index].index, 0x20);
				WriteByte((CNWMessage*)message, classes_info[class_index].level, 0x8);
			}
		}
		CExoLocString_Destructor(&loc_str, 2);
	}
	char* msg_data;
	uint32_t msg_unknown;
	int ret;
	if (GetWriteMessage(message, &msg_data, &msg_unknown))
	{
		ret = SendServerToPlayerMessage(message, player->pl_id, 0x11, 0x2, msg_data, msg_unknown);
	}
	else
	{
		ret = 0;
	}//*/

	/*timeval timer_end;
	gettimeofday(&timer_end, NULL);
	timersub(&timer_end, &timer_start, &timer_end);
	fprintf(stderr, "elapsed: %ld.%06ld\n", timer_end.tv_sec, timer_end.tv_usec);//*/

	return ret;
}

unsigned char d_ret_code_dropturd[0x20];
int (*DropTURD_Org)(CNWSPlayer*) = (int (*)(CNWSPlayer*))&d_ret_code_dropturd;
int DropTURD_HookProc(CNWSPlayer* player)
{
	CNWSObject* object = GetObjectById(player->pl_pc_oid);
	if (object && get_local_int(object, "CANCEL_DROP_TURD"))
	{
		return 0;
	}

	return DropTURD_Org(player);
}

void init()
{
	default_hakset = mysql_admin->query_int("SELECT default_hakset FROM servers WHERE port=" + std::to_string(current_server_port));

	last_custom_player_id = (current_server_port-5121)*0xFFFFFF + 10000;

	hook_call(0x080A1F0B, (uint32_t)OnAllocCNWSPlayer_Extend);
	hook_call(0x080A2092, (uint32_t)OnAllocCNWSPlayer_Extend);

	hook_function(0x08158354, (unsigned long)CanLevelUp_HookProc, d_ret_code_canlvlup, 12);

	hook_function(0x0810e658, (unsigned long)PossessCreature_HookProc, d_ret_code_possesscre, 9);

	hook_function(0x080b2504, (unsigned long)OnInitPlayer_HookProc, d_ret_code_initpc, 12);

	hook_function(0x08077004, (unsigned long)SendServerToPlayerModuleInfo_HookProc, d_ret_code_sendmodifo, 9);

	hook_function(0x080a4c94, (unsigned long)RemovePCFromWorld_HookProc, d_ret_code_rmpcfromworld, 12);

	hook_function(0x082a9f68, (unsigned long)DisconnectPlayer_HookProc, d_ret_code_nouse, 12);

	//multi server system: check double log
	hook_function(0x082a4a54, (long)HandleBNVSMessage_HookProc, d_ret_code_nouse, 12);
	hook_function(0x082a1e38, (long)HandleBNCSMessage_HookProc, d_ret_code_nouse, 12);
	hook_function(0x08053c9c, (long)DestroyPlayer_HookProc, d_ret_code_destroyplayer, 12);

	//dont write the hak count
	enable_write(0x081b8daa);
	*(uint8_t*)(0x081b8daa) = 0xEB;
	//write resref of that hak hooked
	enable_write(0x081b8d98);
	*(uint32_t*)(0x081b8d98) = ((uint32_t)WriteHakCount_HookProc-(uint32_t)0x081b8d9c);
	//write talk table
	enable_write(0x081b8d1f);
	*(uint32_t*)(0x081b8d1f) = ((uint32_t)WriteTalkTable_HookProc-(uint32_t)0x081b8d23);

	//clients players id override
	//write
	enable_write(0x0807727e);
	*(uint32_t*)(0x0807727e) = ((uint32_t)OnWritePlayerId_ClientOverride-(uint32_t)0x08077282);
	enable_write(0x080812aa);
	*(uint32_t*)(0x080812aa) = ((uint32_t)OnWritePlayerId_ClientOverride-(uint32_t)0x080812ae);
	enable_write(0x080777a1);
	*(uint32_t*)(0x080777a1) = ((uint32_t)OnWritePlayerId_ClientOverride-(uint32_t)0x080777a5);
	enable_write(0x080806c2);
	*(uint32_t*)(0x080806c2) = ((uint32_t)OnWritePlayerId_ClientOverride-(uint32_t)0x080806c6);
	enable_write(0x0807dedb);
	*(uint32_t*)(0x0807dedb) = ((uint32_t)OnWritePlayerId_ClientOverride-(uint32_t)0x0807dedf);
	enable_write(0x0807df41);
	*(uint32_t*)(0x0807df41) = ((uint32_t)OnWritePlayerId_ClientOverride-(uint32_t)0x0807df45);
	enable_write(0x0807decd);
	*(uint32_t*)(0x0807decd) = ((uint32_t)OnWritePlayerId_ClientOverride-(uint32_t)0x0807ded1);
	enable_write(0x0807df4f);
	*(uint32_t*)(0x0807df4f) = ((uint32_t)OnWritePlayerId_ClientOverride-(uint32_t)0x0807df53);
	//read
	enable_write(0x08184990);
	*(uint32_t*)(0x08184990) = ((uint32_t)OnReadPlayerId_ClientOverride-(uint32_t)0x08184994);
	enable_write(0x0819abf7);
	*(uint32_t*)(0x0819abf7) = ((uint32_t)OnReadPlayerId_ClientOverride-(uint32_t)0x0819abfb);

	//send remote players
	hook_function(0x080774e4, (unsigned long)SendPlayerList_HookProc, d_ret_code_nouse, 12);
	hook_function(0x080770a8, (unsigned long)SendPlayerList_Add_HookProc, d_ret_code_nouse, 12);
	hook_call(0x08196F07, (long)SendPlayerList_Add_Bypass);
	hook_call(0x08196F20, (long)SendPlayerList_Add_Bypass);
	hook_function(0x08081268, (unsigned long)SendPlayerList_Delete_HookProc, d_ret_code_nouse, 11);

	//enable set dislike on remote players
	enable_write(0x08217b5b);
	*(uint32_t*)(0x08217b5b) = ((uint32_t)GetSetDislikeTargetPlayerByObjectId-(uint32_t)0x08217b5f);
	enable_write(0x08217a2f);
	*(uint32_t*)(0x08217a2f) = ((uint32_t)GetSetDislikeTargetPlayerByObjectId-(uint32_t)0x08217a33);

	
	hook_call(0x080a4e07, (long)OnPCLeaveSaveChar_HookProc);
	hook_function(0x080569d4, (unsigned long)SaveServerChar_HookProc, d_ret_code_sc, 12);
	hook_function(0x0806504c, (unsigned long)SendServerToPlayerArea_ClientArea_HookProc, d_ret_code_sa, 12);
	hook_function(0x081037cc, (unsigned long)UpdateSubAreaOnJump_HookProc, d_ret_code_uponjump, 11);
	hook_function(0x081035e0, (unsigned long)UpdateSubAreaOnMove_HookProc, d_ret_code_uponmove, 12);

	//bugged placeable inventory fix
	hook_function(0x081e0ff0, (unsigned long)OpenPlaceableInventory_HookProc, d_ret_code_plcopen, 12);
	hook_function(0x081e11d0, (unsigned long)ClosePlaceableInventory_HookProc, d_ret_code_plcclose, 12);

	hook_function(0x0805e710, (unsigned long)GetLastUpdateObject_HookProc, d_ret_code_nouse, 11);

	hook_function(0x08078968, (unsigned long)UpdateLastUpdateObjectAppearance_HookProc, d_ret_code_upluoappr, 12);
	hook_function(0x0806cea8, (unsigned long)DeleteLastUpdateObjectsForObject_HookProc, d_ret_code_nouse, 12);
	hook_function(0x0806cc64, (unsigned long)DeleteLastUpdateObjectsInOtherAreas_HookProc, d_ret_code_delluoarea, 12);
	hook_function(0x0806b3a4, (unsigned long)SortObjectsForGameObjectUpdate_HookProc, d_ret_code_sortoforu, 12);
	hook_function(0x08080444, (unsigned long)TestObjectUpdateDifferences_HookProc, d_ret_code_nouse, 12);
	enable_write(0x0822fba9);
	*(uint32_t*)0x0822fba9 = 0x0b0886ff;
	*(uint16_t*)(0x0822fba9+4) = 0x0000;
	*(uint32_t*)(0x0822fba9+6) = 0x90909090;
    
    hook_function(0x08065ad4, (unsigned long)SendCharList_HookProc, d_ret_code_nouse, 12);

	hook_function(0x08054150, (unsigned long)DropTURD_HookProc, d_ret_code_dropturd, 12);
}
REGISTER_INIT(init);

VM_FUNC_REPLACE(GetPCPublicCDKey, 369)
{
	CNWSPlayer* player = get_player_by_game_object_id(vm_pop_object());
	vm_pop_int();
	std::string result;
	if (player)
	{
		std::string cdkey = get_player_info(player)->cdkeys.data[0].public_part;
		std::string to_encode = cdkey + boost::lexical_cast<std::string>(get_current_server_vault_id());
		CryptoPP::SHA256 hash;
		result = apply_hash(hash, to_encode).substr(0, 8);
	}
	vm_push_string(result);
}
VM_FUNC_NEW(GetCorePCPublicCDKey, 557)
{
	CNWSPlayer* player = get_player_by_game_object_id(vm_pop_object());
	std::string result;
	if (player && erf::get_current_script_in_core())
	{
		result = get_player_info(player)->cdkeys.data[0].public_part;
	}
	vm_push_string(result);
}

VM_FUNC_NEW(GetBannedFromServers, 23)
{
	uint32_t object_id = vm_pop_object();
	int result = 0;
	CNWSPlayer* player = get_player_by_game_object_id(object_id);
	if (player)
	{
		result = players_extra_info[player->pl_id].banned_from_servers;
	}
	vm_push_int(result);
}
VM_FUNC_NEW(SetVisiblePlayerName, 327)
{
	CNWSCreature* creature = GetCreatureById(vm_pop_object());
	std::string visible_name = vm_pop_string();
	if (creature)
	{
		PLAYER_EXTRA* player = (PLAYER_EXTRA*)get_player_by_game_object_id(creature->obj.obj_id);
		if (player)
		{
			PlayerInfo* player_info = get_player_info(player);
			player->fake_name = (visible_name == player_info->name.text ? "" : visible_name);
			update_player_list_player(player);
		}
	}
}
VM_FUNC_NEW(GetVisiblePlayerName, 328)
{
	CNWSCreature* creature = GetCreatureById(vm_pop_object());
	CExoString result;
	if (creature)
	{
		PLAYER_EXTRA* player = (PLAYER_EXTRA*)get_player_by_game_object_id(creature->obj.obj_id);
		if (player)
		{
			if (player->fake_name.empty())
			{
				result = get_player_info(player)->name;
			}
			else
			{
				result = player->fake_name;
			}
		}
	}
	vm_push_string(&result);
}
VM_FUNC_NEW(GetPlayerCount, 19)
{
	int player_count = 0;
	CExoLinkedListNode* player_list_node = server_internal->srv_client_list_1->header->first;
	while (player_list_node)
	{
		player_count++;
		player_list_node = player_list_node->next;
	}
	vm_push_int(player_count);
}
CNWSPlayer* get_player_by_list_index(int index)
{
	int player_index = 0;
	CNWSPlayer* player = NULL;
	CExoLinkedListNode* player_list_node = server_internal->srv_client_list_1->header->first;
	while (player_list_node)
	{
		if (player_index == index)
		{
			player = (CNWSPlayer*)player_list_node->data;
			break;
		}
		player_index++;
		player_list_node = player_list_node->next;
	}
	return player;
}
VM_FUNC_NEW(GetPlayerNameByListIndex, 20)
{
	CNWSPlayer* player = get_player_by_list_index(vm_pop_int());
	if (player)
	{
		PlayerInfo* player_info = get_player_info(player);
		if (player_info)
		{
			vm_push_string(&player_info->name);
			return;
		}
	}
	CExoString empty_str("");
	vm_push_string(&empty_str);
}
VM_FUNC_NEW(GetPlayerCharacterByListIndex, 21)
{
	CNWSPlayer* player = get_player_by_list_index(vm_pop_int());
	if (player)
	{
		vm_push_object(player->pl_pc_oid);
	}
	else
	{
		vm_push_object(OBJECT_INVALID);
	}
}
VM_FUNC_NEW(BootPlayerByListIndex, 22)
{
	CNWSPlayer* player = get_player_by_list_index(vm_pop_int());
	uint32_t strref = vm_pop_int();
	if (player)
	{
		disconnect_player(server_internal->srv_network->net_internal, player->pl_id, strref, true);
	}
}

VM_FUNC_NEW(SetCreatureDisplayName, 326)
{
	CNWSCreature* creature = GetCreatureById(vm_pop_object());
	CExoString new_name = vm_pop_string();
	if (creature)
	{
		update_creature_display_name(creature, new_name);
	}
}
VM_FUNC_NEW(SetPlayerCharacterName, 325)
{
	CNWSCreature* creature = GetCreatureById(vm_pop_object());
	CExoString new_name = vm_pop_string();
	if (creature)
	{
		update_creature_display_name(creature, new_name);
		ClearLocString(&creature->cre_stats->cs_firstname);
		ClearLocString(&creature->cre_stats->cs_lastname);
		AddLocString(&creature->cre_stats->cs_firstname, 0, new_name, 0);
		AddLocString(&creature->cre_stats->cs_lastname, 0, CExoString(), 0);
	}
}

VM_FUNC_NEW(AddRemotePlayer, 253)
{
	uint32_t player_id = -1;
	CNWSCreature* creature = GetCreatureById(vm_pop_object());
	int is_dm = vm_pop_int();
	CExoString player_name = vm_pop_string();
	if (creature && player_name.text)
	{
		REMOTE_PLAYER remote_player;
		remote_player.player_name = player_name.text;
		remote_player.is_dm = is_dm;
		remote_player.creature_id = creature->obj.obj_id;
		player_id = custom_player_id_by_player_name(player_name.text);
		remote_player.player_id = player_id;
		send_player_entering_msg_to_all(remote_player);
		remote_players[player_id] = remote_player;
	}
	vm_push_int(player_id);
}
VM_FUNC_NEW(DeleteRemotePlayer, 254)
{
	uint32_t player_id = vm_pop_int();
	remote_players_iter = remote_players.find(player_id);
	if (remote_players_iter != remote_players.end())
	{
		send_player_leaving_msg_to_all(remote_players_iter->second);
		remote_players.erase(remote_players_iter);
	}
}

VM_FUNC_NEW(UpdateMiniMap, 223)
{
	CNWSCreature* creature = GetCreatureById(vm_pop_object());
	if (creature)
	{
		UpdateMiniMap(creature, creature->obj.obj_id);
	}
}


VM_FUNC_NEW(ForceAppearanceUpdate, 214)
{
	CNWSCreature* creature = GetCreatureById(vm_pop_object());
	if (creature)
	{
		force_players_to_update_object(creature->obj.obj_id);
	}
}

VM_FUNC_NEW(PC_GetId, 367)
{
	int pc_id = 0;
	PLAYER_EXTRA* player = (PLAYER_EXTRA*)get_player_by_game_object_id(vm_pop_object());
	if (player)
	{
		pc_id = player->db_pc_id;
	}
	vm_push_int(pc_id);

}
VM_FUNC_NEW(PLAYER_GetId, 368)
{
	PLAYER_EXTRA* player = (PLAYER_EXTRA*)get_player_by_game_object_id(vm_pop_object());
	vm_push_int(player ? db_player_id_from_player(player) : 0);
}
VM_FUNC_NEW(PC_Create, 369)
{
	PLAYER_EXTRA* player = (PLAYER_EXTRA*)get_player_by_game_object_id(vm_pop_object());
	if (player && player->db_pc_id == 0)
	{
		CNWSCreature* creature = GetCreatureById(player->pl_pc_oid);
		if (creature)
		{
			std::string pc_filename = player->pl_bicfile.to_str();
			mysql_admin->query(boost::str(boost::format("INSERT INTO pcs (player_id,vault_id,pc_name,filename,last_seen) VALUES (%1%,%2%,%3%,%4%,NOW())")
				% db_player_id_from_player(player)
				% get_current_server_vault_id()
				% to_sql(get_creature_final_name(creature))
				% to_sql(pc_filename)));
			player->set_db_pc_id(mysql_admin->insert_id());
		}
	}
	vm_push_int(player ? player->db_pc_id : 0);
}
VM_FUNC_NEW(PC_GetCurrentServer, 370)
{
	int result = 0;
	PLAYER_EXTRA* player = (PLAYER_EXTRA*)get_player_by_game_object_id(vm_pop_object());
	if (player && player->db_pc_id != 0)
	{
		result = mysql_admin->query_int("SELECT server FROM pcs WHERE pc_id=" + boost::lexical_cast<std::string>(player->db_pc_id));
	}
	vm_push_int(result);
}
std::string get_saved_location_string()
{
	std::string saved_location_str;
	PLAYER_EXTRA* player = (PLAYER_EXTRA*)get_player_by_game_object_id(vm_pop_object());
	if (player && player->db_pc_id != 0)
	{
		saved_location_str = mysql_admin->query_string("SELECT location FROM pcs WHERE pc_id=" + boost::lexical_cast<std::string>(player->db_pc_id));
	}
	return saved_location_str;
}
VM_FUNC_NEW(PC_GetSavedLocation, 371)
{
	vm_push_location(string_to_location(get_saved_location_string()));
}
VM_FUNC_NEW(PC_GetSavedLocationString, 532)
{
	vm_push_string(get_saved_location_string());
}
void set_saved_location(uint32_t pc_id, const std::string& location, int server_id)
{
	mysql_admin->query(boost::str(boost::format("UPDATE pcs SET location=%1%, server=%2% WHERE pc_id=%3%")
		% to_sql(location)
		% server_id
		% pc_id));	
}
VM_FUNC_NEW(PC_SetSavedLocationData, 527)
{
	PLAYER_EXTRA* player = (PLAYER_EXTRA*)get_player_by_game_object_id(vm_pop_object());
	std::string location = vm_pop_string();
	int server_id = vm_pop_int();
	if (player && player->db_pc_id != 0 && server_id & get_current_server_vault_id())
	{
		set_saved_location(player->db_pc_id, location, server_id);
	}
}
VM_FUNC_NEW(PC_SetSavedLocation, 372)
{
	PLAYER_EXTRA* player = (PLAYER_EXTRA*)get_player_by_game_object_id(vm_pop_object());
	CScriptLocation location = vm_pop_location();
	if (player && player->db_pc_id != 0)
	{
		set_saved_location(player->db_pc_id, location_to_string(location), current_server_id);
	}
}
VM_FUNC_NEW(GetPlayerServerRoles, 540)
{
	PLAYER_EXTRA* player = (PLAYER_EXTRA*)get_player_by_game_object_id(vm_pop_object());
	int roles = 0;
	if (player)
	{
		roles = player->server_roles;
	}
	vm_push_int(roles);
}

int get_player_options(uint32_t player_obj_id)
{
	PLAYER_EXTRA* player = (PLAYER_EXTRA*)get_player_by_game_object_id(player_obj_id);
	int result = 0;
	if (player)
	{
		result = mysql_admin->query_int("SELECT options FROM players WHERE player_id="+boost::lexical_cast<std::string>(db_player_id_from_player(player)));
    }
    return result;
}
VM_FUNC_NEW(PC_GetPlayerOptions, 541)
{
	vm_push_int(get_player_options(vm_pop_object()));
}
VM_FUNC_NEW(PC_GetPlayerOption, 542)
{
	uint32_t player_obj_id = vm_pop_object();
	int option = vm_pop_int();
	vm_push_int(get_player_options(player_obj_id) & option);
}
VM_FUNC_NEW(PC_SetPlayerOption, 543)
{
	uint32_t player_obj_id = vm_pop_object();
	PLAYER_EXTRA* player = (PLAYER_EXTRA*)get_player_by_game_object_id(player_obj_id);
	int option = vm_pop_int();
	int is_on = vm_pop_int();
	int options = get_player_options(player_obj_id);
	if (player)
	{
		if (is_on) options |= option; else options &= ~option;
		mysql_admin->query("UPDATE players SET options="+boost::lexical_cast<std::string>(options)+
			" WHERE player_id="+boost::lexical_cast<std::string>(db_player_id_from_player(player)));
	}
}
VM_FUNC_NEW(GetPlayerOptionString, 544)
{
	PLAYER_EXTRA* player = (PLAYER_EXTRA*)get_player_by_game_object_id(vm_pop_object());
	std::string setting_group = vm_pop_string();
	std::string setting_name = vm_pop_string();
	std::string result;
	if (player)
	{
		result = mysql_admin->query_int(boost::str(boost::format("SELECT setting_value FROM players_settings WHERE player_id=%1% AND setting_group=%2% AND setting_name=%3%")
			% db_player_id_from_player(player)
			% to_sql(setting_group)
			% to_sql(setting_name)));
    }
	vm_push_string(result);
}
VM_FUNC_NEW(SetPlayerOptionString, 545)
{
	PLAYER_EXTRA* player = (PLAYER_EXTRA*)get_player_by_game_object_id(vm_pop_object());
	std::string setting_group = vm_pop_string();
	std::string setting_name = vm_pop_string();
	std::string setting_value = vm_pop_string();
	if (player)
	{
		mysql_admin->query(boost::str(boost::format("REPLACE INTO players_settings(player_id,setting_group,setting_name,setting_value) VALUES(%1%,%2%,%3%,%4%)")
				% db_player_id_from_player(player)
				% to_sql(setting_group)
				% to_sql(setting_name)
				% to_sql(setting_value)));
	}
}
VM_FUNC_NEW(GetPlayerGameVersion, 562)
{
	PLAYER_EXTRA* player = (PLAYER_EXTRA*)get_player_by_game_object_id(vm_pop_object());
	int result = 0;
	if (player)
	{
		PlayerExtraInfo* player_extra_info = &players_extra_info[player->pl_id];
		result = player_extra_info->game_version;
	}
	vm_push_int(result);
}
VM_FUNC_NEW(GetPlayerGameLanguage, 563)
{
	PLAYER_EXTRA* player = (PLAYER_EXTRA*)get_player_by_game_object_id(vm_pop_object());
	int result = 0;
	if (player)
	{
		PlayerInfo* player_info = &(*p_app_manager)->app_server->srv_internal->srv_network->net_internal->players_info[player->pl_id];
		result = player_info->language;
	}
	vm_push_int(result);
}
VM_FUNC_NEW(GetPlayerExpansionPack, 564)
{
	PLAYER_EXTRA* player = (PLAYER_EXTRA*)get_player_by_game_object_id(vm_pop_object());
	int result = 0;
	if (player)
	{
		PlayerInfo* player_info = &(*p_app_manager)->app_server->srv_internal->srv_network->net_internal->players_info[player->pl_id];
		result = player_info->expansion_pack;
	}
	vm_push_int(result);
}

}
}
