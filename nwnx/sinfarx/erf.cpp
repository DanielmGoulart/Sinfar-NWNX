#include "erf.h"
#include "mysql.h"
#include "nwscript.h"
#include "server.h"

#include <boost/regex.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/format.hpp>
#include <boost/algorithm/string.hpp>

using namespace nwnx::mysql;
using namespace nwnx::core;
using namespace nwnx::nwscript;
using namespace nwnx::server;

namespace nwnx { namespace erf {

std::unordered_map<std::string, RESERVED_PREFIX_DATA> reserved_prefixes;
std::unordered_map<std::string, RESERVED_PREFIX_DATA>::iterator reserved_prefixes_iter;
RESERVED_PREFIX_DATA* get_prefix_data(const std::string& res_prefix)
{
	reserved_prefixes_iter = reserved_prefixes.find(res_prefix);
	if (reserved_prefixes_iter != reserved_prefixes.end())
	{
		return &reserved_prefixes_iter->second;
	}
	return NULL;
}
std::string get_res_prefix(const std::string& resref)
{
	uint32_t resref_len = resref.length();
	if (resref_len < 2 || resref_len > 20) return "";
	char res_prefix[30];
	char* temp_res_prefix = res_prefix;
	const char* temp_resref = resref.c_str();
	while (*temp_resref)
	{
		if (*temp_resref == '_')
		{
			*temp_res_prefix = 0;
			return res_prefix;
		}
		else
		{
			*temp_res_prefix = tolower(*temp_resref);
			temp_res_prefix++;
			temp_resref++;
		}
	}
	return "";	
}
RESERVED_PREFIX_DATA* get_res_prefix_data(const std::string& resref)
{
	return get_prefix_data(get_res_prefix(resref));
}
RESERVED_PREFIX_DATA* get_current_script_prefix_data()
{
	return get_res_prefix_data(get_last_script_ran());
}

int get_current_script_servers()
{
	RESERVED_PREFIX_DATA* rdp = get_current_script_prefix_data();
	return (rdp ? rdp->access_on_servers : 0);
}
int get_current_script_in_core()
{
	RESERVED_PREFIX_DATA* rdp = get_current_script_prefix_data();
	return (rdp ? rdp->in_core : 0);
}
int get_current_script_erf_id()
{
	RESERVED_PREFIX_DATA* rdp = get_current_script_prefix_data();
	return (rdp ? rdp->erf_id : 0);
}
int get_current_script_public()
{
	RESERVED_PREFIX_DATA* rdp = get_current_script_prefix_data();
	return (rdp ? rdp->is_public : 0);
}
void load_reserved_prefixes()
{
	char sql[1000];
	sprintf(sql, "SELECT reserved_prefix.prefix,erf.prefix,erf.access_on_servers,public,in_core,erf.id FROM reserved_prefix LEFT JOIN erf ON reserved_prefix.erf_id=erf.id WHERE erf.access_on_servers&%d", current_server_id);
	if (mysql_admin->query(sql))
	{
		reserved_prefixes.clear();
		CMySQLRes result = mysql_admin->store_result();
		MYSQL_ROW row;
		while ((row = result.fetch_row()) != NULL)
		{
			RESERVED_PREFIX_DATA& reserved_prefix_data = reserved_prefixes[row[0]];
			reserved_prefix_data.prefix = row[1];
			reserved_prefix_data.access_on_servers = atoi(row[2]);
			reserved_prefix_data.is_public = atoi(row[3]);
			reserved_prefix_data.in_core = atoi(row[4]);
			reserved_prefix_data.erf_id = atoi(row[5]);
		}
	}
}
void init()
{
	load_reserved_prefixes();
}
REGISTER_INIT(init);

VM_FUNC_NEW(ReloadERFs, 524)
{
	load_reserved_prefixes();
}

VM_FUNC_NEW(GetCurrentScriptInCore, 551)
{
	vm_push_int(get_current_script_in_core());
}
VM_FUNC_NEW(GetCurrentScriptPublic, 552)
{
	vm_push_int(get_current_script_public());
}
VM_FUNC_NEW(GetCurrentScriptErfId, 553)
{
	vm_push_int(get_current_script_erf_id());
}
VM_FUNC_NEW(GetCurrentScriptServers, 554)
{
	vm_push_int(get_current_script_servers());
}

}
}
