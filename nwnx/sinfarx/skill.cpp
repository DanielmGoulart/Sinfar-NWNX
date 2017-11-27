#include "core.h"
#include "creature.h"
#include "nwscript.h"
#include "cached2da.h"

using namespace nwnx::core;
using namespace nwnx::creature;
using namespace nwnx::nwscript;
using namespace nwnx::cached2da;

namespace nwnx { namespace skill {

void init()
{
	//increase the healkit use animation duration
	enable_write(0x0810b75e);
	*(float*)(0x0810b75e) = 4.5;
}
REGISTER_INIT(init);
	
}
}