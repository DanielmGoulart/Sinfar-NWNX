#include "setting.h"
#include "mysql.h"
#include "nwscript.h"
#include "setting.h"
#include "erf.h"

#include <boost/regex.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/format.hpp>
#include <boost/algorithm/string.hpp>

using namespace nwnx::mysql;
using namespace nwnx::core;
using namespace nwnx::nwscript;
using namespace nwnx::setting;
using namespace nwnx::erf;

namespace nwnx { namespace setting {
	
void init()
{
	register_hook(hook::module_loading, []{
		mysql_admin->query("UPDATE servers_settings SET active=0 WHERE server="+boost::lexical_cast<std::string>(current_server_id));
	});
}
REGISTER_INIT(init);

void (*SetMaxPlayer)(CNetLayer*, uint32_t) = (void (*)(CNetLayer*, uint32_t))0x082ad608;
const int SERVSETTING_MAX_PLAYERS = 1001;
VM_FUNC_NEW(SetServerSetting, 170)
{
	uint32_t setting_index = vm_pop_int();
	int value = vm_pop_int();
	if (setting_index >= 57 && setting_index <= 73)
	{
		*(((int*)(server_internal->server_settings))+setting_index) = value;
	}
	else if (setting_index == 1001)
	{
		SetMaxPlayer(server_internal->srv_network, value);
	}
}
const int SERVSETTING_STR_DESCRIPTION = 1;
const int SERVSETTING_STR_PLAYER_PASSWORD = 2;
int (*SetPlayerPassword)(CNetLayer*, CExoString) = (int (*)(CNetLayer*, CExoString))0x082ad1fc;
VM_FUNC_NEW(SetServerSettingString, 171)
{
	uint32_t setting_index = vm_pop_int();
	CExoString value = vm_pop_string();
	switch (setting_index)
	{
		case SERVSETTING_STR_DESCRIPTION:
			server_internal->server_settings->description = value;
			break;
		case SERVSETTING_STR_PLAYER_PASSWORD:
			SetPlayerPassword(server_internal->srv_network, value);
			break;
	}
}
VM_FUNC_NEW(SetServerSettingFloat, 194)
{
	uint32_t setting_index = vm_pop_int();
	float value = vm_pop_float();
    if (setting_index && value) return;
}

bool regex_test(const std::string& value, const std::string& regex_str)
{
    boost::regex regex = boost::regex(regex_str, boost::regex_constants::extended);
	return boost::regex_match(value, regex, boost::regex_constants::match_any);
}
template<typename T>T load_server_option(const std::string& input_name, const T& default_value, setting::type type, int* min, int* max, const std::string& description)
{
	std::string value;
	std::string name = boost::to_upper_copy(input_name);
	if (name.length()<=64 && type>=1 && type<=6)
	{
		int erf_id = get_current_script_erf_id();
		if (erf_id == 0) erf_id = 1;
		std::string escaped_name = mysql_escape_string(name);
		value = mysql_admin->query_string(boost::str(boost::format("SELECT setting_value FROM servers_settings WHERE server=%1% AND setting_name='%2%' AND setting_value!=setting_default_value")
			% current_server_id
			% escaped_name));
		if (!value.empty())
		{
			std::string value_regex;
			switch (type)
			{
				case type_float:
				{
					float float_value = atof(value.c_str());
					if ((!min || float_value >= *min) && (!max || float_value <= *max))
					{
						value_regex = "^[0-9]+\\.?[0-9]*$";
					}
					else
					{
						value = "";
					}
					break;
				}
				case type_integer:
				{
					int int_value = atoi(value.c_str());
					if ((!min || int_value >= *min) && (!max || int_value <= *max))
					{
						value_regex = "^[0-9]+$";
					}
					else
					{
						value = "";
					}
					break;
				}
				case type_boolean:
					value = atoi(value.c_str()) ? "1" : "0";
					break;
				case type_text:
					break;
				case type_tag:
					value_regex = "^[A-Z0-9_]{1,32}$";
					break;
				case type_resref:
					value_regex = "^[a-z0-9_]{1,16}$";
					break;
				default: return default_value;
			}
			if (!value_regex.empty() && !regex_test(value, value_regex))
			{
				value = "";
			}
		}
		T final_value = value.empty() ? default_value : boost::lexical_cast<T>(value);
		mysql_admin->query(boost::str(boost::format("REPLACE INTO servers_settings(server,setting_name,erf_id,setting_value,setting_default_value,setting_type,setting_min,setting_max,description,active) VALUES(%1%,'%2%',%3%,'%4%','%5%',%6%,%7%,%8%,'%9%',%10%)")
			% current_server_id
			% escaped_name
			% erf_id
			% mysql_escape_string(boost::lexical_cast<std::string>(final_value))
			% mysql_escape_string(boost::lexical_cast<std::string>(default_value))
			% type
			% (min ? boost::lexical_cast<std::string>(*min) : "null")
			% (max ? boost::lexical_cast<std::string>(*max) : "null")
			% mysql_escape_string(description)
			% 1));
		return final_value;
	}
	return default_value;
}
std::unordered_map<std::string, int> server_vars_int;
std::unordered_map<std::string, float> server_vars_float;
std::unordered_map<std::string, std::string> server_vars_string;
int load_server_int(const std::string& name, int default_value, setting::type type, int* min, int* max, const std::string& description)
{
    return load_server_option(name, default_value, type, min, max, description);
}
VM_FUNC_NEW(LoadServerInt, 333)
{
	std::string name = vm_pop_string();
	int default_value = vm_pop_int();
	int min = vm_pop_int();
	int max = vm_pop_int();
	std::string description = vm_pop_string();
	int result = load_server_int(name, default_value, type_integer, &min, &max, description);
	server_vars_int[name] = result;
	vm_push_int(result);
}
int load_server_bool(const std::string& name, int default_value, const std::string& description)
{
    return load_server_option(name, default_value, type_boolean, NULL, NULL, description);
}
VM_FUNC_NEW(LoadServerBoolean, 332)
{
	std::string name = vm_pop_string();
	int default_value = vm_pop_int();
	std::string description = vm_pop_string();
	int result = load_server_bool(name, default_value, description);
	server_vars_int[name] = result;
	vm_push_int(result);
}
float load_server_float(const std::string& name, float default_value, int* min, int* max, const std::string& description)
{
    return load_server_option(name, default_value, type_float, min, max, description);
}
VM_FUNC_NEW(LoadServerFloat, 334)
{
	std::string name = vm_pop_string();
	float default_value = vm_pop_float();
	int min = vm_pop_int();
	int max = vm_pop_int();
	std::string description = vm_pop_string();
	float result = load_server_float(name, default_value, &min, &max, description);
	server_vars_float[name] = result;
	vm_push_float(result);
}
std::string load_server_string(const std::string& name, const std::string& default_value, setting::type type, const std::string& description)
{
    return load_server_option(name, default_value, type, NULL, NULL, description);
}
VM_FUNC_NEW(LoadServerString, 335)
{
	std::string name = vm_pop_string();
	std::string default_value = vm_pop_string();
	std::string description = vm_pop_string();
	setting::type type = static_cast<setting::type>(vm_pop_int());
	std::string result = load_server_string(name, default_value, type, description);
	server_vars_string[name] = result;
	vm_push_string(result);
}
VM_FUNC_NEW(GetServerInt, 336)
{
	std::string name = vm_pop_string();
	vm_push_int(server_vars_int[name]);
}
VM_FUNC_NEW(GetServerFloat, 337)
{
	std::string name = vm_pop_string();
	vm_push_float(server_vars_float[name]);
}
VM_FUNC_NEW(GetServerString, 338)
{
	std::string name = vm_pop_string();
	vm_push_string(server_vars_string[name]);
}
VM_FUNC_NEW(SetServerInt, 339)
{
	std::string name = vm_pop_string();
	server_vars_int[name] = vm_pop_int();
}
VM_FUNC_NEW(SetServerFloat, 340)
{
	std::string name = vm_pop_string();
	server_vars_float[name] = vm_pop_float();
}
VM_FUNC_NEW(SetServerString, 341)
{
	std::string name = vm_pop_string();
	server_vars_string[name] = vm_pop_string();
}
VM_FUNC_NEW(ReloadServerSettings, 329)
{
	if (mysql_admin->query(boost::str(boost::format("SELECT setting_type,setting_name,setting_value FROM servers_settings WHERE server=%1% AND active=1") % current_server_id)))
	{
		CMySQLRes result = mysql_admin->store_result();
		MYSQL_ROW row;
		while ((row = result.fetch_row()) != NULL)
		{
			switch (atoi(row[0]))
			{
				case type_float:
					server_vars_float[row[1]] = atof(row[2]);
					break;
				case type_integer:
				case type_boolean:
					server_vars_int[row[1]] = atoi(row[2]);
					break;
				case type_text:
				case type_tag:
				case type_resref:
					server_vars_string[row[1]] = row[2];
					break;
			}
		}
	}
}


}
}
