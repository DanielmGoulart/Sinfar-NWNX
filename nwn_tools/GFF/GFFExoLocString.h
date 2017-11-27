#pragma once

#include <map>
#include <stdint.h>

class CGFFField;
class CGFFStruct;
class CGFFList;
class CGFFExoLocString;

const uint32_t GFF_FIELD_TYPE_BYTE = 0;
const uint32_t GFF_FIELD_TYPE_CHAR = 1;
const uint32_t GFF_FIELD_TYPE_WORD = 2;
const uint32_t GFF_FIELD_TYPE_SHORT = 3;
const uint32_t GFF_FIELD_TYPE_DWORD = 4;
const uint32_t GFF_FIELD_TYPE_INT = 5;
const uint32_t GFF_FIELD_TYPE_DWORD64 = 6;
const uint32_t GFF_FIELD_TYPE_INT64 = 7;
const uint32_t GFF_FIELD_TYPE_FLOAT = 8;
const uint32_t GFF_FIELD_TYPE_DOUBLE = 9;
const uint32_t GFF_FIELD_TYPE_CEXOSTRING = 10;
const uint32_t GFF_FIELD_TYPE_CRESREF = 11;
const uint32_t GFF_FIELD_TYPE_CEXOLOCSTRING = 12;
const uint32_t GFF_FIELD_TYPE_VOID = 13;
const uint32_t GFF_FIELD_TYPE_STRUCT = 14;
const uint32_t GFF_FIELD_TYPE_LIST = 15;

class CGFFExoLocString
{
public:
	CGFFExoLocString(void);
	~CGFFExoLocString(void);
	CGFFExoLocString* Clone(CGFFExoLocString* lpGFFExoLocString);

	uint32_t nStrRef;
	std::map<int, char*> data;
	
	typedef std::map<int, char*>::iterator STRINGS_ITERATOR;
	STRINGS_ITERATOR GetLangsIterator();
	int GetLangAtPos(STRINGS_ITERATOR& iterator);
	int GetNextLang(STRINGS_ITERATOR& iterator);

	void RemoveAllString();
	const char* GetString(int nLangId);
	void SetString(int nLangId, const char* string);
	int GetStrRef();
	void SetStrRef(int strref);
};

