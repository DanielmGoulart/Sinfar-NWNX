#include "nwscript_funcs.h"
#include "../crypto.h"
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>

namespace
{

VM_FUNC_NEW(HexToInt, 558)
{
	std::string hex = vm_pop_string();
	const char* c_hex = hex.c_str();
	if (strncmp(c_hex, "0x", 2) == 0) c_hex += 2;
	int result = strtoul(c_hex, 0, 16); 
	vm_push_int(result);
}
VM_FUNC_NEW(ByteToHex, 559)
{
	int int_val = vm_pop_int();
	vm_push_string(n2hexstr(int_val, 2));
}
VM_FUNC_NEW(WordToHex, 560)
{
	int int_val = vm_pop_int();	
	vm_push_string(n2hexstr(int_val, 4));
}
VM_FUNC_NEW(IntToHex, 561)
{
	int int_val = vm_pop_int();	
	vm_push_string(n2hexstr(int_val, 8));	
}

}