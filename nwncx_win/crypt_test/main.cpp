#include <Windows.h>
#include <string>
#include <cstring>
#include <cstdio>

char my_decrypt_buffer[10000];
inline const char* my_decrypt(const BYTE to_decrypt[], size_t to_decrypt_len, const BYTE crypt_key[], size_t crypt_key_len)
{
	size_t crypt_key_index = 0;
	for (size_t to_decrypt_index=0; to_decrypt_index<to_decrypt_len; to_decrypt_index++)
	{
		my_decrypt_buffer[to_decrypt_index] = to_decrypt[to_decrypt_index] ^ crypt_key[crypt_key_index];
		crypt_key_index++; if (crypt_key_index >= crypt_key_len) crypt_key_index = 0;
	}
	my_decrypt_buffer[to_decrypt_len] = 0;
	return my_decrypt_buffer;
}
#define MY_DECRYPT(arg1, arg2) my_decrypt(arg1, sizeof(arg1), arg2, sizeof(arg2))

int main() 
{
	BYTE crypt_key[] = {0x98,0x23,0xB7};
	char* to_crypt = "nwn.silm.pw";

	size_t crypt_key_len = sizeof(crypt_key);
	size_t crypt_key_index = 0;
	BYTE* iter_to_crypt = (BYTE*)to_crypt;
	std::string cpp_crypted;
	while (*iter_to_crypt)
	{
		if (!cpp_crypted.empty()) cpp_crypted += ",";
		cpp_crypted += "0x";
		char crypted_byte_hex[3];
		sprintf_s(crypted_byte_hex, 3, "%02X", *iter_to_crypt ^ crypt_key[crypt_key_index]);
		cpp_crypted += crypted_byte_hex;
		crypt_key_index++; if (crypt_key_index >= crypt_key_len) crypt_key_index = 0;
		iter_to_crypt++;
	}
	cpp_crypted = "{" + cpp_crypted + "}";
	puts(cpp_crypted.c_str());

	BYTE to_decrypt[] = { 0xEB, 0x4A, 0xDB, 0xF5, 0x0D, 0xC7, 0xEF };
	puts(MY_DECRYPT(to_decrypt, crypt_key));

	system("pause");
}