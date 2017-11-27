#pragma once

#include "core.h"
#include <unordered_set>

namespace nwnx { namespace ai {

extern std::unordered_set<CNWSArea*> active_areas;

void add_pending_x_event(std::function<void()> pending_x_event);

}
}
