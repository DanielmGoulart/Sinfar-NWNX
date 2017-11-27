#include "nwscript_funcs.h"
#include "../crypto.h"

namespace
{

template<class T> void vm_func_hash(T hash)
{
	std::string value = vm_pop_string();
	vm_push_string(apply_hash(hash, value));	
}

VM_FUNC_NEW(CRC32, 555)
{
	CryptoPP::CRC32 hash;
	std::string value = vm_pop_string();
	uint32_t crc32_value;
	hash.CalculateDigest((byte*)&crc32_value, (const byte*)value.c_str(), value.length());
	vm_push_int(crc32_value);	
}

VM_FUNC_NEW(SHA256, 556)
{
	vm_func_hash(CryptoPP::SHA256());
}
	
}