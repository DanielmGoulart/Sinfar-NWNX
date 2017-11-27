#pragma once

#include "../../nwn_tools/C2da/C2da.h"
#include "../../nwn_tools/CKey/CKey.h"

namespace nwnx { namespace cached2da {

C2da* get_cached_2da(std::string table_name);
void remove_2da_from_cache(std::string table_name);
void clear_cached_2da();

}
}
