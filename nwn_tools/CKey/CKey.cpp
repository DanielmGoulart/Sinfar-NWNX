#include "CKey.h"
#include <algorithm>
#include <memory>

#ifdef WIN32
#include <Windows.h>
#elif __linux__
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#endif

using namespace aurora;

namespace {
	void dummy_log_printf(const char* format, ...){};
	void dummy_printstacktrace(const std::string& info) {};
}
void (*aurora::log_printf)(const char* format, ...) = dummy_log_printf;
void(*aurora::print_stacktrace)(const std::string& info) = dummy_printstacktrace;

void aurora::normalize_resref(char* resref)
{
	for (uint32_t i=0; i<16; i++)
	{
		resref[i] = tolower(resref[i]);
	}
}

char* aurora::strtolower(char* string)
{
	char* temp = string;
	if (temp == NULL) return NULL;
	do
	{
		*temp = tolower(*temp);
	}
	while (*++temp);
	return string;
}

const char* aurora::NwnGetResTypeExtension(NwnResType nType)
{
	switch (nType)
	{
		case NwnResType_RES: return "res";
		case NwnResType_BMP: return "bmp";
		case NwnResType_MVE: return "mve";
		case NwnResType_TGA: return "tga";
		case NwnResType_WAV: return "wav";
		case NwnResType_PLT: return "plt";
		case NwnResType_INI: return "ini";
		case NwnResType_BMU: return "bmu";
		case NwnResType_MPG: return "mpg";
		case NwnResType_TXT: return "txt";
		case NwnResType_PLH: return "plh";
		case NwnResType_TEX: return "tex";
		case NwnResType_MDL: return "mdl";
		case NwnResType_THG: return "thg";
		case NwnResType_FNT: return "fnt";
		case NwnResType_LUA: return "lua";
		case NwnResType_SLT: return "slt";
		case NwnResType_NSS: return "nss";
		case NwnResType_NCS: return "ncs";
		case NwnResType_MOD: return "mod";
		case NwnResType_ARE: return "are";
		case NwnResType_SET: return "set";
		case NwnResType_IFO: return "ifo";
		case NwnResType_BIC: return "bic";
		case NwnResType_WOK: return "wok";
		case NwnResType_2DA: return "2da";
		case NwnResType_TLK: return "tlk";
		case NwnResType_TXI: return "txi";
		case NwnResType_GIT: return "git";
		case NwnResType_BTI: return "bti";
		case NwnResType_UTI: return "uti";
		case NwnResType_BTC: return "btc";
		case NwnResType_UTC: return "utc";
		case NwnResType_DLG: return "dlg";
		case NwnResType_ITP: return "itp";
		case NwnResType_BTT: return "btt";
		case NwnResType_UTT: return "utt";
		case NwnResType_DDS: return "dds";
		case NwnResType_UTS: return "uts";
		case NwnResType_LTR: return "ltr";
		case NwnResType_GFF: return "gff";
		case NwnResType_FAC: return "fac";
		case NwnResType_BTE: return "bte";
		case NwnResType_UTE: return "ute";
		case NwnResType_BTD: return "btd";
		case NwnResType_UTD: return "utd";
		case NwnResType_BTP: return "btp";
		case NwnResType_UTP: return "utp";
		case NwnResType_DTF: return "dtf";
		case NwnResType_GIC: return "gic";
		case NwnResType_GUI: return "gui";
		case NwnResType_CSS: return "css";
		case NwnResType_CCS: return "ccs";
		case NwnResType_BTM: return "btm";
		case NwnResType_UTM: return "utm";
		case NwnResType_DWK: return "dwk";
		case NwnResType_PWK: return "pwk";
		case NwnResType_BTG: return "btg";
		case NwnResType_UTG: return "utg";
		case NwnResType_JRL: return "jrl";
		case NwnResType_SAV: return "sav";
		case NwnResType_UTW: return "utw";
		case NwnResType_4PC: return "4pc";
		case NwnResType_SSF: return "ssf";
		case NwnResType_HAK: return "hak";
		case NwnResType_NWM: return "nwm";
		case NwnResType_BIK: return "bik";
		case NwnResType_ERF: return "erf";
		case NwnResType_BIF: return "bif";
		case NwnResType_KEY: return "key";
		case NwnResType_PTM: return "ptm";
		case NwnResType_PTT: return "ptt";
		default: return NULL;
	}
}

NwnResType aurora::NwnGetResTypeFromExtension(const char* extension)
{
	if (*extension == '.')
		extension++;
	char lower_extension[4];
	lower_extension[0] = tolower(extension[0]);
	lower_extension[1] = tolower(extension[1]);
	lower_extension[2] = tolower(extension[2]);
	lower_extension[3] = 0;
	int extension_code = *(int*)lower_extension;
	switch (extension_code)
	{
		case 0x736572: return NwnResType_RES;
		case 0x706d62: return NwnResType_BMP;
		case 0x65766d: return NwnResType_MVE;
		case 0x616774: return NwnResType_TGA;
		case 0x766177: return NwnResType_WAV;
		case 0x746c70: return NwnResType_PLT;
		case 0x696e69: return NwnResType_INI;
		case 0x756d62: return NwnResType_BMU;
		case 0x67706d: return NwnResType_MPG;
		case 0x747874: return NwnResType_TXT;
		case 0x686c70: return NwnResType_PLH;
		case 0x786574: return NwnResType_TEX;
		case 0x6c646d: return NwnResType_MDL;
		case 0x676874: return NwnResType_THG;
		case 0x746e66: return NwnResType_FNT;
		case 0x61756c: return NwnResType_LUA;
		case 0x746c73: return NwnResType_SLT;
		case 0x73736e: return NwnResType_NSS;
		case 0x73636e: return NwnResType_NCS;
		case 0x646f6d: return NwnResType_MOD;
		case 0x657261: return NwnResType_ARE;
		case 0x746573: return NwnResType_SET;
		case 0x6f6669: return NwnResType_IFO;
		case 0x636962: return NwnResType_BIC;
		case 0x6b6f77: return NwnResType_WOK;
		case 0x616432: return NwnResType_2DA;
		case 0x6b6c74: return NwnResType_TLK;
		case 0x697874: return NwnResType_TXI;
		case 0x746967: return NwnResType_GIT;
		case 0x697462: return NwnResType_BTI;
		case 0x697475: return NwnResType_UTI;
		case 0x637462: return NwnResType_BTC;
		case 0x637475: return NwnResType_UTC;
		case 0x676c64: return NwnResType_DLG;
		case 0x707469: return NwnResType_ITP;
		case 0x747462: return NwnResType_BTT;
		case 0x747475: return NwnResType_UTT;
		case 0x736464: return NwnResType_DDS;
		case 0x737475: return NwnResType_UTS;
		case 0x72746c: return NwnResType_LTR;
		case 0x666667: return NwnResType_GFF;
		case 0x636166: return NwnResType_FAC;
		case 0x657462: return NwnResType_BTE;
		case 0x657475: return NwnResType_UTE;
		case 0x647462: return NwnResType_BTD;
		case 0x647475: return NwnResType_UTD;
		case 0x707462: return NwnResType_BTP;
		case 0x707475: return NwnResType_UTP;
		case 0x667464: return NwnResType_DTF;
		case 0x636967: return NwnResType_GIC;
		case 0x697567: return NwnResType_GUI;
		case 0x737363: return NwnResType_CSS;
		case 0x736363: return NwnResType_CCS;
		case 0x6d7462: return NwnResType_BTM;
		case 0x6d7475: return NwnResType_UTM;
		case 0x6b7764: return NwnResType_DWK;
		case 0x6b7770: return NwnResType_PWK;
		case 0x677462: return NwnResType_BTG;
		case 0x677475: return NwnResType_UTG;
		case 0x6c726a: return NwnResType_JRL;
		case 0x766173: return NwnResType_SAV;
		case 0x777475: return NwnResType_UTW;
		case 0x637034: return NwnResType_4PC;
		case 0x667373: return NwnResType_SSF;
		case 0x6b6168: return NwnResType_HAK;
		case 0x6d776e: return NwnResType_NWM;
		case 0x6b6962: return NwnResType_BIK;
		case 0x667265: return NwnResType_ERF;
		case 0x666962: return NwnResType_BIF;
		case 0x79656b: return NwnResType_KEY;
		case 0x6d7470: return NwnResType_PTM;
		case 0x747470: return NwnResType_PTT;
	}
	return NwnResType_Unknown;
}

std::string CResource::GetFileName() const
{
	std::string filename(resref.value, strnlen(resref.value, 16));
	const char* extension = NwnGetResTypeExtension(type);
	if (extension != NULL)
	{
		filename += ".";
		filename += extension;
	}
	return filename;
}

std::unordered_set<CResourcesDirectory*>* CResourcesDirectory::watched_resdirectories;
mutex CResourcesDirectory::watch_thread_mutex;
#ifdef WIN32
DWORD WINAPI CResourcesDirectory::watch_thread(void* param)
{
	while (true)
	{
		std::vector<HANDLE> watch_handles;

		{
			lock_guard lock(watch_thread_mutex);

			if (watched_resdirectories == NULL || watched_resdirectories->size() == 0)
			{
				//log_printf("Exiting watch_thread because watched_resdirectories is NULL or empty\n");
				break;
			}

			for (auto iter=watched_resdirectories->begin(); iter!=watched_resdirectories->end(); iter++)
			{
				watch_handles.push_back((*iter)->watch_directory_handle);
			}
		}

		DWORD waitResult = WaitForMultipleObjects(watch_handles.size(), watch_handles.data(), FALSE, INFINITE);

		if (waitResult >= WAIT_OBJECT_0 && waitResult < WAIT_OBJECT_0+watch_handles.size())
		{
			std::string res_dir_path;
			void (*on_res_changed)(const std::string&) = NULL;
			HANDLE watch_handle = watch_handles.at(waitResult - WAIT_OBJECT_0);
			
			{
				lock_guard lock(watch_thread_mutex);

				for (auto iter=watched_resdirectories->begin(); iter!=watched_resdirectories->end(); iter++)
				{
					if ((*iter)->watch_directory_handle == watch_handle)
					{
						res_dir_path = (*iter)->dir_path;
						on_res_changed = (*iter)->on_res_changed;
						FindNextChangeNotification(watch_handle);
						break;
					}
				}
			}

			if (on_res_changed)
			{
				on_res_changed(res_dir_path);
			}
		}
	}
	return 0;
}
HANDLE CResourcesDirectory::watch_thread_handle = INVALID_HANDLE_VALUE;
void CResourcesDirectory::start_watch_thread()
{
	if (CResourcesDirectory::watch_thread_handle == INVALID_HANDLE_VALUE)
	{
		CResourcesDirectory::watch_thread_handle = CreateThread(NULL, 0, &watch_thread, NULL, 0, NULL);
	}
}
#endif

CResourcesDirectory::CResourcesDirectory(const std::string& dir_path, void (*on_res_changed)(const std::string&)) : dir_path(dir_path), on_res_changed(on_res_changed)
{
	RefreshResources();

	{
#ifdef WIN32
		lock_guard lock(watch_thread_mutex);

		if (watched_resdirectories == NULL)
		{
			watched_resdirectories = new std::unordered_set<CResourcesDirectory*>;
		}

		if (on_res_changed)
		{
			watch_directory_handle = FindFirstChangeNotificationA(dir_path.c_str(), FALSE, FILE_NOTIFY_CHANGE_FILE_NAME);
			CResourcesDirectory::watched_resdirectories->insert(this);
		}
		else
		{
			watch_directory_handle = INVALID_HANDLE_VALUE;
		}
#endif
	}
}

void CResourcesDirectory::AddFile(const std::string& file_name, uint32_t file_size)
{
	std::unique_ptr<CFileResource> file_res(new CFileResource(dir_path + "/" + file_name, file_size));
	uint32_t file_name_length = file_name.length();
	uint32_t resref_length = file_name_length - 4;
	if (resref_length > 16) resref_length = 16;
	CResRef file_resref;
	strncpy(file_resref.value, file_name.c_str(), resref_length);
	normalize_resref(file_resref.value);
	resources[CResource(NwnGetResTypeFromExtension(file_name.c_str() + file_name_length - 4), file_resref)] = std::move(file_res);
}

void CResourcesDirectory::RefreshResources()
{

	resources.clear();
#ifdef WIN32
	WIN32_FIND_DATAA fd;
	HANDLE find_handle = FindFirstFileA((dir_path + "/*.???").c_str(), &fd);
	if (find_handle != INVALID_HANDLE_VALUE) do
	{
		if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
		{
			AddFile(fd.cFileName, fd.nFileSizeLow);
		}
	} while (FindNextFileA(find_handle, &fd));
#elif __linux__
	DIR* dir = opendir(dir_path.c_str());
	if (dir)
	{
		while (struct dirent* dir_entry = readdir(dir)) 
		{
			if (!S_ISDIR(dir_entry->d_type)) {
				std::string file_path = dir_path + "/" + dir_entry->d_name;
				struct stat file_info;
				if (stat(file_path.c_str(), &file_info) == 0)
				{
					AddFile(dir_entry->d_name, file_info.st_size);
				}
			}
		}
		closedir(dir);
	}
#endif
}

CResourcesDirectory::~CResourcesDirectory()
{
	{
#ifdef WIN32
		lock_guard lock(watch_thread_mutex);

		if (watch_directory_handle != INVALID_HANDLE_VALUE)
		{
			FindCloseChangeNotification(watch_directory_handle);
		}
		watched_resdirectories->erase(this);
#endif
	}
}

void CFileResource::ReadData(char* buffer)
{
	CFile f(fopen(file_path.c_str(), "rb"));
	if (f)
	{
		fread(buffer, file_size, 1, f);
	}
}

CBIF::CBIF(const std::string& filepath) : file_path(filepath)
{
	CFile bif_file(fopen(filepath.c_str(), "rb"));
	if (bif_file)
	{
		fread(&header, sizeof(header), 1, bif_file);
		res_entries.resize(header.VariableResourceCount);
		fseek(bif_file, header.VariableTableOffset, SEEK_SET);
		fread(&res_entries.at(0), sizeof(CBIF::BIF_VARIABLE_RES_ENTRY)*header.VariableResourceCount, 1, bif_file);
	}
}

void CBIFResource::ReadData(char* buffer)
{
	CFile bif_file(fopen(bif->file_path.c_str(), "rb"));
	if (!bif_file) return;
	CBIF::BIF_VARIABLE_RES_ENTRY& res_entry = bif->res_entries.at(index_in_bif);
	fseek(bif_file, res_entry.Offset, SEEK_SET);
	fread(buffer, res_entry.FileSize, 1, bif_file);
}

void CResourceData::SaveToFile(const std::string& filepath)
{
	std::unique_ptr<char[]> res_data(new char[this->GetDataSize()]);
	this->ReadData(res_data.get());
	CFile f(fopen(filepath.c_str(), "wb"));
	if (f)
	{
		fwrite(res_data.get(), this->GetDataSize(), 1, f);
	}
}

CKey::CKey(const std::string& filepath, const std::string& nwn_dir)
{
	CFile f(fopen(filepath.c_str(), "rb"));
	if (!f) return;
	KEY_HEADER header;
	fread(&header, sizeof(header), 1, f);
	std::unique_ptr<KEY_BIF_ENTRY[]> bif_entries(new KEY_BIF_ENTRY[header.BIFCount]);
	fread(bif_entries.get(), sizeof(KEY_BIF_ENTRY)*header.BIFCount, 1, f);
	std::vector<std::shared_ptr<CBIF>> bif_files;
	uint32_t filename_table_offset = static_cast<uint32_t>(ftell(f));
	uint32_t filename_table_size = header.OffsetToKeyTable-filename_table_offset;
	std::unique_ptr<char[]> bif_filenames(new char[filename_table_size]);
	fread(bif_filenames.get(), filename_table_size, 1, f);
	for (uint32_t i=0; i<header.BIFCount; i++)
	{
		KEY_BIF_ENTRY& bif_entry = bif_entries[i];
		std::string bif_filename(bif_filenames.get()+bif_entry.FilenameOffset-filename_table_offset, bif_entry.FilenameSize);
		std::replace(bif_filename.begin(), bif_filename.end(), '\\', '/');
		std::string final_bif_filename = nwn_dir + "/" + bif_filename;
		bif_files.push_back(std::shared_ptr<CBIF>(new CBIF(final_bif_filename)));
	}
	std::unique_ptr<KEY_RES_ENTRY[]> res_entries(new KEY_RES_ENTRY[header.KeyCount]);
	fread(res_entries.get(), sizeof(KEY_RES_ENTRY)*header.KeyCount, 1, f);
	for (int32_t i=header.KeyCount-1; i>=0; i--)
	{
		KEY_RES_ENTRY& res_entry = res_entries[i];
		normalize_resref(res_entry.ResRef.value);
		uint32_t bif_index = ((res_entry.ResID & 0xFFF00000) >> 20);
		uint32_t index_in_bif = (res_entry.ResID & 0xFFFFF);
		resources[CResource(static_cast<NwnResType>(res_entry.ResourceType), res_entry.ResRef)] = std::unique_ptr<CBIFResource>(new CBIFResource(bif_files.at(bif_index), index_in_bif));
	}
}