#pragma once

#include "include/NWNXLib.h"

namespace nwnx
{
namespace odbc
{

char *SaveObject(uint32_t object_id, int* size);
uint32_t LoadObject(void* data, int size, const CScriptLocation& location, uint32_t owner_id);

}
}