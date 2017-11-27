#pragma once

#include <stdint.h>
#include <string>
#include <cstring>
#include <vector>
#include <map>
#include <unordered_set>
#include <memory>
#include <cstdio>
#include "../../nwnx/common/include/structs/CResRef.h"

#ifdef WIN32
#include <boost/thread/lock_guard.hpp>
#include <boost/thread/recursive_mutex.hpp>
#elif __linux__
#include <mutex>
#endif

#ifdef __linux__
#define _stricmp strcasecmp
#define _strnicmp strncasecmp
#elif WIN32
#include <Windows.h>
#endif

namespace aurora {

extern void (*log_printf)(const char* format, ...);
extern void(*print_stacktrace)(const std::string& info);

#ifdef WIN32
typedef boost::recursive_mutex mutex;
typedef boost::lock_guard<mutex> lock_guard;
#elif __linux__
typedef std::recursive_mutex mutex;
typedef std::lock_guard<mutex> lock_guard;
#endif

char* strtolower(char* str);
void normalize_resref(char* resref);

class CFile
{
private:
	CFile() : file(NULL) {}
public:
	CFile(FILE* file) : file(file) {}
	~CFile() {if (file) fclose(file);}
	operator FILE*() {return file;}
	FILE* file;
};

enum NwnResType
{
	NwnResType_RES		= 0x0000,
	NwnResType_BMP		= 0x0001,
	NwnResType_MVE		= 0x0002,
	NwnResType_TGA		= 0x0003,
	NwnResType_WAV		= 0x0004,
	NwnResType_PLT		= 0x0006,
	NwnResType_INI		= 0x0007,
	NwnResType_BMU		= 0x0008,
	NwnResType_MPG		= 0x0009,
	NwnResType_TXT		= 0x000A,
	NwnResType_PLH		= 0x07D0,
	NwnResType_TEX		= 0x07D1,
	NwnResType_MDL		= 0x07D2,
	NwnResType_THG		= 0x07D3,
	NwnResType_FNT		= 0x07D5,
	NwnResType_LUA		= 0x07D7,
	NwnResType_SLT		= 0x07D8,
	NwnResType_NSS		= 0x07D9,
	NwnResType_NCS		= 0x07DA,
	NwnResType_MOD		= 0x07DB,
	NwnResType_ARE		= 0x07DC,
	NwnResType_SET		= 0x07DD,
	NwnResType_IFO		= 0x07DE,
	NwnResType_BIC		= 0x07DF,
	NwnResType_WOK		= 0x07E0,
	NwnResType_2DA		= 0x07E1,
	NwnResType_TLK		= 0x07E2,
	NwnResType_TXI		= 0x07E6,
	NwnResType_GIT		= 0x07E7,
	NwnResType_BTI		= 0x07E8,
	NwnResType_UTI		= 0x07E9,
	NwnResType_BTC		= 0x07EA,
	NwnResType_UTC		= 0x07EB,
	NwnResType_DLG		= 0x07ED,
	NwnResType_ITP		= 0x07EE,
	NwnResType_BTT		= 0x07EF,
	NwnResType_UTT		= 0x07F0,
	NwnResType_DDS		= 0x07F1,
	NwnResType_UTS		= 0x07F3,
	NwnResType_LTR		= 0x07F4,
	NwnResType_GFF		= 0x07F5,
	NwnResType_FAC		= 0x07F6,
	NwnResType_BTE		= 0x07F7,
	NwnResType_UTE		= 0x07F8,
	NwnResType_BTD		= 0x07F9,
	NwnResType_UTD		= 0x07FA,
	NwnResType_BTP		= 0x07FB,
	NwnResType_UTP		= 0x07FC,
	NwnResType_DTF		= 0x07FD,
	NwnResType_GIC		= 0x07FE,
	NwnResType_GUI		= 0x07FF,
	NwnResType_CSS		= 0x0800,
	NwnResType_CCS		= 0x0801,
	NwnResType_BTM		= 0x0802,
	NwnResType_UTM		= 0x0803,
	NwnResType_DWK		= 0x0804,
	NwnResType_PWK		= 0x0805,
	NwnResType_BTG		= 0x0806,
	NwnResType_UTG		= 0x0807,
	NwnResType_JRL		= 0x0808,
	NwnResType_SAV		= 0x0809,
	NwnResType_UTW		= 0x080A,
	NwnResType_4PC		= 0x080B,
	NwnResType_SSF		= 0x080C,
	NwnResType_HAK		= 0x080D,
	NwnResType_NWM		= 0x080E,
	NwnResType_BIK		= 0x080F,
	NwnResType_PTM		= 0x0811,
	NwnResType_PTT		= 0x0812,
	NwnResType_ERF		= 0x270D,
	NwnResType_BIF		= 0x270E,
	NwnResType_KEY		= 0x270F,
	NwnResType_Unknown	= 0xFFFFFFFF,
};
const char* NwnGetResTypeExtension(NwnResType nType);
NwnResType NwnGetResTypeFromExtension(const char* extension);

class CResource {
public:
	CResource() : type(NwnResType_Unknown), resref("") {};
	CResource(const std::string filename)
	{
		if (filename.length() >= 5)
		{
			int dot_pos = filename.length()-3;
			type = NwnGetResTypeFromExtension(filename.c_str()+dot_pos);
			dot_pos--;
			strncpy(resref.value, filename.c_str(), dot_pos);
			resref.value[dot_pos] = 0;
		}
		else
		{
			type = NwnResType_Unknown;
			resref.value[0] = 0;
		}
	}
	CResource(NwnResType type, char resref[16]) : type(type), resref(resref) {};
	CResource(NwnResType type, CResRef resref) : type(type), resref(resref) {};
	CResource(const CResource& copy) : type(copy.type), resref(copy.resref) {};
	bool operator<(const CResource& cmp_to) const
	{
		if (type != cmp_to.type) return (type < cmp_to.type);
		return _strnicmp(resref.value, cmp_to.resref.value, 16) < 0;
	}
	std::string GetFileName() const;
	NwnResType type;
	CResRef resref;
};

class CBIF
{
friend class CBIFResource;
public:
	CBIF(const std::string& filepath);
	std::string file_path;
private:
	#pragma pack(push, 1)
	struct BIF_HEADER
	{
		char FileType[4];
		char FileVersion[4];
		uint32_t VariableResourceCount;
		uint32_t FixedResourceCount;
		uint32_t VariableTableOffset;
	};
	struct BIF_VARIABLE_RES_ENTRY
	{
		uint32_t ResID;
		uint32_t Offset;
		uint32_t FileSize;
		uint32_t ResourceType;
	};
	struct BIF_FIXED_RES_ENTRY
	{
		uint32_t ResID;
		uint32_t Offset;
		uint32_t PartCount;
		uint32_t FileSize;
		uint32_t ResourceType;
	};
	#pragma pack(pop)
	BIF_HEADER header;
	std::vector<BIF_VARIABLE_RES_ENTRY> res_entries;
};

class CResourceData
{
public:
	void SaveToFile(const std::string& filepath);
	virtual uint32_t GetDataSize() { print_stacktrace("CResourceData::GetDataSize"); return 0; }
	virtual void ReadData(char* buffer) { print_stacktrace("CResourceData::ReadData"); }
	virtual ~CResourceData() {}
};

class CResourcesContainer
{
public:
	std::map<CResource, std::unique_ptr<CResourceData>> resources;
	virtual ~CResourcesContainer() {}
};

class CFileResource : public CResourceData
{
public:
	CFileResource(const std::string& file_path, uint32_t file_size) : file_path(file_path), file_size(file_size) {}
	uint32_t GetDataSize() { return file_size; }
	void ReadData(char* buffer);
	std::string file_path;
	uint32_t file_size;
};

class CResourcesDirectory : public CResourcesContainer
{
public:
#ifdef WIN32
	static void start_watch_thread();
#endif
	CResourcesDirectory(const std::string& dir_path, void (*on_res_changed)(const std::string&));
	~CResourcesDirectory();
	void RefreshResources();
	std::string dir_path;
private:
#ifdef WIN32
	static HANDLE watch_thread_handle;
	static DWORD WINAPI watch_thread(void* param);
	HANDLE watch_directory_handle;
#endif
	static std::unordered_set<CResourcesDirectory*>* watched_resdirectories;
	void (*on_res_changed)(const std::string&);
	static mutex watch_thread_mutex;

	void AddFile(const std::string& file_name, uint32_t file_size);
};

class CBIFResource : public CResourceData
{
public:
	CBIFResource(){}
	CBIFResource(std::shared_ptr<CBIF> bif, uint32_t index_in_bif) : bif(bif), index_in_bif(index_in_bif) {};
	uint32_t GetDataSize() { return bif->res_entries.at(index_in_bif).FileSize; }
	void ReadData(char* buffer);
	std::shared_ptr<CBIF> bif;
	uint32_t index_in_bif;
};

class CKey : public CResourcesContainer
{
public:
	CKey(const std::string& filepath, const std::string& nwn_dir);
private:
	#pragma pack(push, 1)
	struct KEY_HEADER
	{
		char FileType[4];
		char FileVersion[4];
		uint32_t BIFCount;
		uint32_t KeyCount;
		uint32_t OffsetToFileTable;
		uint32_t OffsetToKeyTable;
		uint32_t BuildYear;
		uint32_t BuildDay;
		char Reserved[32];
	};
	struct KEY_BIF_ENTRY
	{
		uint32_t FileSize;
		uint32_t FilenameOffset;
		uint16_t FilenameSize;
		uint16_t Drives;
	};
	struct KEY_RES_ENTRY
	{
		CResRef ResRef;
		uint16_t ResourceType;
		uint32_t ResID;
	};
	#pragma pack(pop)
	char file_type[4];
	char file_version[4];
};

}
