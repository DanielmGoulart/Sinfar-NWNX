#pragma once

#include "core.h"

namespace nwnx { namespace erf {

    struct RESERVED_PREFIX_DATA
    {
    	std::string prefix;
    	int erf_id;
    	int access_on_servers;
    	int8_t in_core;
    	int8_t is_public;
    };
	RESERVED_PREFIX_DATA* get_prefix_data(const std::string& res_prefix);
    RESERVED_PREFIX_DATA* get_res_prefix_data(const std::string& resref);
    std::string get_res_prefix(const std::string& resref);
    int get_current_script_servers();
    int get_current_script_in_core();
    int get_current_script_erf_id();
    int get_current_script_public();

}
}
