#pragma once

#include "core.h"
#include "../../nwn_tools/CKey/CKey.h"

namespace nwnx { namespace resman {

std::string get_resource_path(aurora::NwnResType type, CResRef resref);

}
}
