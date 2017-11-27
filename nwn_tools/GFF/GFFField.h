#pragma once

#include "GFFExoLocString.h"
#include "GFFList.h"

class CGFFField
{
private:
	void DeleteData();
public:
	void SetResRef(const char* value);

	CGFFField(void);
	CGFFField(const char* label);
	CGFFField(const char* label, CGFFList* list);
	~CGFFField(void);
	CGFFField* Clone(CGFFField* lpGFFField);
	
	const char* GetLabel();
	uint32_t GetDataType();
	
	CGFFStruct* AsStruct();
	CGFFList* AsList();
	const char* AsResRef();
	const char* AsString();
	CGFFExoLocString* AsLocString();
	int AsInt();
	short AsShort();
	char AsChar();
	uint8_t AsBYTE();
	uint16_t AsWORD();
	uint32_t AsDWORD();
	float AsFloat();
    double AsSignificantFloat();
	double AsDouble();
	uint64_t AsDWORD64();
	int64_t AsINT64();

	char* label;
	char* data;
	uint32_t dataType;
};
