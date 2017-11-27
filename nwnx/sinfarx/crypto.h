#include <string>
#include <cryptopp/crc.h>
#include <cryptopp/sha.h>
#include <cryptopp/hex.h>

template<class T> std::string apply_hash(T hash, const std::string& data)
{
	byte digest[hash.DigestSize()];
	hash.CalculateDigest(digest, (const byte*)data.c_str(), data.length());
	CryptoPP::HexEncoder encoder;
	std::string result;
	encoder.Attach(new CryptoPP::StringSink(result));
	encoder.Put(digest, sizeof(digest));
	encoder.MessageEnd();
	return result;
}

template <typename I> std::string n2hexstr(I w, size_t hex_len = sizeof(I)<<1) {
    static const char* digits = "0123456789ABCDEF";
    std::string rc(hex_len,'0');
    for (size_t i=0, j=(hex_len-1)*4 ; i<hex_len; ++i,j-=4)
        rc[i] = digits[(w>>j) & 0x0f];
    return rc;
}
