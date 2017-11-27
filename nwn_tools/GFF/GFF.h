#pragma once

#include <cstdio>
#include <cstring>
#include <string>
#include <stdlib.h>
#include "GFFStruct.h"
#include "GFFList.h"
#include "GFFExoLocString.h"

#include <stdint.h>

class CFileBuffer
{
private:
	char* buffer;
	uint32_t size;
	uint32_t pos;
	uint32_t eof_pos;
public:
	CFileBuffer(uint32_t starting_size);
	CFileBuffer();
	void seek(size_t new_pos);
	void write(const void* data, size_t data_len);
	char* get_data();
	char* steal_data();
	uint32_t get_data_size();
	~CFileBuffer();
};

struct GFF_HEADER
{
	char FileType[4] = "";
	char FileVersion[4] = "";
	uint32_t StructOffset = 0;
	uint32_t StructCount = 0;
	uint32_t FieldOffset = 0;
	uint32_t FieldCount = 0;
	uint32_t LabelOffset = 0;
	uint32_t LabelCount = 0;
	uint32_t FieldDataOffset = 0;
	uint32_t FieldDataCount = 0;
	uint32_t FieldIndicesOffset = 0;
	uint32_t FieldIndicesCount = 0;
	uint32_t ListIndicesOffset = 0;
	uint32_t ListIndicesCount = 0;
};

struct GFF_STRUCT
{
	uint32_t Type = 0;
	uint32_t DataOrDataOffset = 0;
	uint32_t FieldCount = 0;
};

struct GFF_FIELD
{
	uint32_t Type = 0;
	uint32_t LabelIndex = 0;
	uint32_t DataOrDataOffset = 0;
};

struct GFF_LABEL
{
	char Name[16] = "";
};

struct GFF_RESREF
{
	uint8_t size = 0;
	char ResRef[16] = "";
};

class CGFF
{
public:
	CGFF(const char* fileType, const char* fileVersion);
	CGFF(const char* fileType, const char* fileVersion, CGFFStruct* mainStruct);
	CGFF(const char* filename, bool bIgnoreMinorErros=false);
	CGFF(const char* file_data, uint32_t file_size, bool bIgnoreMinorErros);
	CGFF(CGFF* copyme);
	~CGFF(void);

	char* GetLastErrorMessage();

	CGFFStruct* GetMainStruct();
	void SetMainStruct(CGFFStruct* main_struct);
	std::string GetFileType();
	std::string GetFileVersion();

	CGFFStruct* mainStruct;
	void ToFileBuffer(CFileBuffer& file_buffer);
	void SaveToFile(const char* filename);
	char fileType[4];
	char fileVersion[4];
private:
	char last_error[255];

	void init(const char* file_data, uint32_t file_size, bool bIgnoreMinorErros);
	
	bool GetIsGFFHeaderValid(GFF_HEADER* lp_gff_header, uint32_t file_size);
	
	double* ReadDouble(GFF_HEADER* lp_gff_header, uint32_t nOffset);		
	char* ReadExoString(GFF_HEADER* lp_gff_header, uint32_t nIndex);
	char* ReadResRef(GFF_HEADER* lp_gff_header, uint32_t nOffset);
	char* ReadLabel(GFF_HEADER* lp_gff_header, uint32_t nIndex);
	CGFFExoLocString* ReadExoLocString(GFF_HEADER* lp_gff_header, uint32_t nOffset);	
	CGFFList* ReadList(GFF_HEADER* lp_gff_header, uint32_t nOffset, uint32_t* lpStructCount);
	CGFFField* ReadField(GFF_HEADER* lp_gff_header, uint32_t nIndex, uint32_t* lpStructCount);
	CGFFStruct* ReadStruct(GFF_HEADER* lp_gff_header, uint32_t nIndex, uint32_t* lpStructCount);
	
	uint32_t GetCExoLocStringTotalSize(CGFFExoLocString* lpGFFExoLocString);
	void GetListDataCount(GFF_HEADER* lp_gff_header, CGFFList* lpGFFList, std::map<const char*, uint32_t, map_strcmp>* lp_labels_map);
	void GetStructDataCount(GFF_HEADER* lp_gff_header, CGFFStruct* lpGFFStruct, std::map<const char*, uint32_t, map_strcmp>* lp_labels_map);
	
	uint32_t WriteDouble(CFileBuffer& file_buffer, GFF_HEADER* lp_gff_header, double* lpDouble);
	uint32_t WriteExoLocString(CFileBuffer& file_buffer, GFF_HEADER* lp_gff_header, CGFFExoLocString* lpGFFExoLocString);
	uint32_t WriteResRef(CFileBuffer& file_buffer, GFF_HEADER* lp_gff_header, char* resref);
	uint32_t WriteExoString(CFileBuffer& file_buffer, GFF_HEADER* lp_gff_header, char* string);
	uint32_t WriteList(CFileBuffer& file_buffer, GFF_HEADER* lp_gff_header, CGFFList* lpGFFList, std::map<const char*, uint32_t, map_strcmp>* lp_labels_map);
	uint32_t WriteLabel(CFileBuffer& file_buffer, GFF_HEADER* lp_gff_header, char* label, std::map<const char*, uint32_t, map_strcmp>* lp_labels_map);
	uint32_t WriteField(CFileBuffer& file_buffer, GFF_HEADER* lp_gff_header, CGFFField* lpGFFField, std::map<const char*, uint32_t, map_strcmp>* lp_labels_map);
	uint32_t WriteStruct(CFileBuffer& file_buffer, GFF_HEADER* lp_gff_header, CGFFStruct* lpGFFStruct, std::map<const char*, uint32_t, map_strcmp>* lp_labels_map);
};
