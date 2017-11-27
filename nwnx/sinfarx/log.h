#pragma once

#include "core.h"

namespace nwnx { namespace log {

enum level
{
	verbose = 0,
	debug = 1,
	info = 2,
	warning = 3,
	error = 4,
	wtf = 5
};

void logf(level level, const char* fmt, ...);
void log(const std::string& message, level level, const std::string& script, int erf_id=0);
	
}
}