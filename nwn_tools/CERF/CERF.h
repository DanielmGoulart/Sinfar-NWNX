#pragma once

#include <stdint.h>
#include <string>
#include <cstring>
#include <vector>
#include <map>
#include <memory>
#include <fstream>
#include "../../nwnx/common/include/structs/CResRef.h"
#include "../CKey/CKey.h"

namespace aurora {

	class CERF : public CResourcesContainer
	{
		friend class CERFResource;
	public:
		CERF(const std::string& filepath);
		#pragma pack(push, 1)
		struct ERF_HEADER
		{
			char FileType[4];
			char Version[4];
			uint32_t LanguageCount;
			uint32_t LocalizedStringSize;
			uint32_t EntryCount;
			uint32_t OffsetToLocalizedString;
			uint32_t OffsetToKeyList;
			uint32_t OffsetToResourceList;
			uint32_t BuildYear;
			uint32_t BuildDay;
			uint32_t DescriptionStrRef;
			uint8_t Reserved[116];
		};
		struct ERF_KEY
		{
			char ResRef[16];
			uint32_t ResID;
			uint16_t ResType;
			uint16_t Unused;
		};
		struct ERF_RESOURCE
		{
			uint32_t OffsetToResource;
			uint32_t ResourceSize;
		};
		#pragma pack(pop)
		ERF_HEADER header;
		std::string filepath;
	};

	class CERFResource : public CResourceData
	{
	public:
		CERFResource(CERF& erf, CERF::ERF_RESOURCE res_info) : erf(erf), res_info(res_info) {};
		void ReadData(char* buffer);
		uint32_t GetDataSize() { return res_info.ResourceSize; }
		CERF& erf;
		CERF::ERF_RESOURCE res_info;
	};
}