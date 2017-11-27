#pragma once

#include "GFFField.h"
#include <map>
#include <cstring>

struct map_strcmp
{
	bool operator()( const char* s1, const char* s2 ) const
	{
		return strcmp( s1, s2 ) < 0;
	}
};

class CGFFStruct
{
public:
	CGFFStruct();
	CGFFStruct(uint32_t nStructType);
	~CGFFStruct();
	CGFFStruct* Clone(CGFFStruct* lpGFFStruct);

	uint32_t structType;
	std::map<const char*, CGFFField*, map_strcmp> data;
	typedef std::map<const char*, CGFFField*, map_strcmp>::iterator FIELDS_ITERATOR;

	void SetStructType(uint32_t nStructType);
	uint32_t GetStructType();
	
	FIELDS_ITERATOR GetFieldsIterator();
	FIELDS_ITERATOR GetFieldsEndIterator();
	CGFFField* GetFieldAtPos(FIELDS_ITERATOR& iterator);
	CGFFField* GetNextField(FIELDS_ITERATOR& iterator);

	uint32_t GetFieldCount();
	void SetField(CGFFField* lpGFFField);
	CGFFField* GetField(const char* label);
	void RemoveField(const char* label);

	void SetStruct(const char* label, CGFFStruct* value);
	void SetList(const char* label, CGFFList* value);
	void SetResRef(const char* label, const char* value);
	void SetString(const char* label, const char* value);
	void SetLocString(const char* label, CGFFExoLocString* value);
	void SetDWORD(const char* label, uint32_t value);
	void SetWORD(const char* label, uint16_t value);	
	void SetBYTE(const char* label, uint8_t value);
	void SetChar(const char* label, char value);
	void SetInt(const char* label, int value);
	void SetShort(const char* label, short value);
	void SetFloat(const char* label, float value);
	void SetDouble(const char* label, double value);
	void SetDWORD64(const char* label, uint64_t value);
	void SetINT64(const char* label, int64_t value);

	CGFFStruct* GetStruct(const char* label);
	CGFFList* GetList(const char* label);
	const char* GetResRef(const char* label);
	const char* GetString(const char* label);
	const char* GetLocStringStr(const char* label, int nLangId);
	void SetLocStringStr(const char* label, int nLangId, const char* value);
	CGFFExoLocString* GetLocString(const char* label);
	int GetInt(const char* label);
	short GetShort(const char* label);
	uint8_t GetBYTE(const char* label);
	char GetChar(const char* label);
	uint16_t GetWORD(const char* label);
	uint32_t GetDWORD(const char* label);
	float GetFloat(const char* label);
	double GetDouble(const char* label);
	uint64_t GetDWORD64(const char* label);
	int64_t GetINT64(const char* label);
};
