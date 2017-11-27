#pragma once

#include "core.h"

namespace nwnx { namespace setting {

	enum type
	{
		type_float = 1,
		type_integer = 2,
		type_text = 3,
		type_tag = 4,
		type_resref = 5,
		type_boolean = 6
	};

    int load_server_int(const std::string& name, int default_value, setting::type type, int* min, int* max, const std::string& description);
    int load_server_bool(const std::string& name, int default_value, const std::string& description);
    float load_server_float(const std::string& name, float default_value, int* min, int* max, const std::string& description);
    std::string load_server_string(const std::string& name, const std::string& default_value, setting::type type, const std::string& description);


}
}
