#include "nwscript_funcs.h"
#include "../player.h"
#include "../mysql.h"

#include <boost/format.hpp>
#include <boost/lexical_cast.hpp>

using namespace nwnx::mysql;

namespace {

VM_FUNC_REPLACE(SetCampaignVariable, 589,590,591,592,593,602)
{
	bool is_obj = (last_vm_command == 602);
    std::string campaign_name = vm_pop_string();
    std::string var_name = vm_pop_string();
	const char* var_value = NULL;
	std::string var_value_str;
	std::unique_ptr<char> var_value_ptr;
	if (is_obj)
	{
		int object_sql_len = 0;
		var_value_ptr = object_to_sql(vm_pop_object(), object_sql_len);
		var_value = var_value_ptr.get();
	}
	else
	{
		switch (last_vm_command)
		{
			case 589: var_value_str = boost::lexical_cast<std::string>(vm_pop_float()); break;
			case 590: var_value_str = boost::lexical_cast<std::string>(vm_pop_int()); break;
			case 591: var_value_str = vector_to_string(vm_pop_vector()); break;
			case 592: var_value_str = location_to_string(vm_pop_location()); break;
			case 593: var_value_str = vm_pop_string(); break;
		}
        var_value_str = to_sql(var_value_str);
		var_value = var_value_str.c_str();
	}
    player::PLAYER_EXTRA* player = (player::PLAYER_EXTRA*)get_player_by_game_object_id(vm_pop_object());
    mysql_restricted->query(boost::str(boost::format("REPLACE INTO variables(campaign_name,var_name,%1%,pc_id) VALUES(%2%,%3%,%4%,%5%)")
		% (is_obj ? "var_obj" : "var_value")
        % to_sql(campaign_name)
        % to_sql(var_name)
        % (var_value ? var_value : "null")
        % (player ? player->db_pc_id : 0)));
}

VM_FUNC_REPLACE(GetCampaignVariable, 595,596,597,598,599,603)
{
    bool is_obj = (last_vm_command == 603);
    std::string campaign_name = vm_pop_string();
    std::string var_name = vm_pop_string();
    CScriptLocation location;
    uint32_t owner_id = OBJECT_INVALID;
    if (is_obj)
    {
        location = vm_pop_location();
        owner_id = vm_pop_object();
    }
    player::PLAYER_EXTRA* player = (player::PLAYER_EXTRA*)get_player_by_game_object_id(vm_pop_object());
    bool query_result = mysql_restricted->query(boost::str(boost::format("SELECT %1% FROM variables WHERE campaign_name=%2% AND var_name=%3% AND pc_id=%4%")
        % (is_obj ? "var_obj" : "var_value")
        % to_sql(campaign_name)
        % to_sql(var_name)
        % (player ? player->db_pc_id : 0)));
    if (is_obj)
    {
        uint32_t result_object = OBJECT_INVALID;
        if (query_result)
        {
			CMySQLRes sql_result = mysql_restricted->store_result();
            sql_result.fetch_row();
            result_object = read_object_from_sql_result(sql_result, 0, location, owner_id);
        }
        vm_push_object(result_object);
    }
    else
    {
        std::string result_string;
        if (query_result)
        {
            CMySQLRes sql_result = mysql_restricted->store_result();
            MYSQL_ROW row;
            if ((row = sql_result.fetch_row()) != NULL)
            {
                result_string = (row[0] ? row[0] : "");
            }
        }
		switch (last_vm_command)
		{
            case 595: vm_push_float(atof(result_string.c_str())); break;
            case 596: vm_push_int(atoi(result_string.c_str())); break;
            case 597: vm_push_vector(string_to_vector(result_string)); break;
            case 598: vm_push_location(string_to_location(result_string)); break;
            case 599: vm_push_string(result_string); break;
        }
    }
}

VM_FUNC_REPLACE(DestroyCampaignDatabase, 594)
{
    std::string campaign_name = vm_pop_string();
    mysql_restricted->query(boost::str(boost::format("DELETE FROM variables WHERE campaign_name=%1%")
        % to_sql(campaign_name)));
}

VM_FUNC_REPLACE(DeleteCampaignVariable, 601)
{
    std::string campaign_name = vm_pop_string();
    std::string var_name = vm_pop_string();
    player::PLAYER_EXTRA* player = (player::PLAYER_EXTRA*)get_player_by_game_object_id(vm_pop_object());
    mysql_restricted->query(boost::str(boost::format("DELETE FROM variables WHERE campaign_name=%1% AND var_name=%2% AND pc_id=%3%")
        % to_sql(campaign_name)
		% to_sql(var_name)
		% (player ? player->db_pc_id : 0)));
}

}