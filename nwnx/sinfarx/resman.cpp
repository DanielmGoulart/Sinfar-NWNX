#include "resman.h"
#include "nwscript.h"
#include "server.h"
#include "mysql.h"
#include "erf.h"

#include "GFF/GFF.h"

#include <sys/stat.h>

#include <boost/lexical_cast.hpp>

using namespace nwnx::core;
using namespace nwnx::nwscript;
using namespace nwnx::server;
using namespace nwnx::mysql;
using namespace nwnx::erf;

namespace nwnx { namespace resman {

std::string module_core_dir;
std::string module_top_haks_dir;
std::string module_bottom_haks_dir;

std::vector<std::string> folders_2da;
std::vector<std::string>::iterator folders_2da_iter;

class ResManResource : public aurora::CResource {
public:
	ResManResource(aurora::NwnResType type, CResRef resref) : aurora::CResource(type, resref) {};
	ResManResource(const ResManResource& copy) : aurora::CResource(copy) {};
	time_t last_mod;
};
std::map<ResManResource, CRes*> resources;
std::map<ResManResource, CRes*>::iterator resources_iter;
inline void free_res_data(CRes* res)
{
	if (res->data)
	{
		free(res->data);
		res->data = 0;
		res->size = 0;
	}
}

CGFF generic_char("/sinfar/nwn/modules/generic_char.bic");
bool ValidateNewCharFile(const char* filename)
{
	FILE* f = fopen64(filename, "rb");
	if (!f)
	{
		fprintf(stderr, "failed to open:%s with error:%d\n", filename, errno);
		return false;
	}
	fseek(f, 0, SEEK_END);
	uint32_t nFileSize = ftell(f);
	fclose(f);
	if (nFileSize > 50*1024) return false;

	CGFF temp_char(filename);
	if (temp_char.GetMainStruct() == NULL) return false;
	if (temp_char.GetMainStruct()->GetList("Equip_ItemList") == NULL) return false;

	if (temp_char.GetMainStruct()->GetBYTE("IsDM") ||
		temp_char.GetMainStruct()->GetList("Equip_ItemList")->GetStructCount() > 1 ||
		temp_char.GetMainStruct()->GetDWORD("Experience") > 0)
	{
		temp_char.GetMainStruct()->SetDWORD("Experience", 1);
		temp_char.SaveToFile(filename);
		return true;
	}

	CGFF valid_char(&generic_char);

	valid_char.mainStruct->SetField(temp_char.mainStruct->GetField("FirstName"));
	valid_char.mainStruct->SetField(temp_char.mainStruct->GetField("LastName"));
	valid_char.mainStruct->SetField(temp_char.mainStruct->GetField("Age"));
	valid_char.mainStruct->SetField(temp_char.mainStruct->GetField("Gender"));
	valid_char.mainStruct->SetField(temp_char.mainStruct->GetField("Race"));
	valid_char.mainStruct->SetField(temp_char.mainStruct->GetField("SoundSetFile"));
	valid_char.mainStruct->SetField(temp_char.mainStruct->GetField("Portrait"));

	valid_char.mainStruct->SetField(temp_char.mainStruct->GetField("Description"));
	valid_char.mainStruct->SetField(temp_char.mainStruct->GetField("Deity"));
	valid_char.mainStruct->SetField(temp_char.mainStruct->GetField("Subrace"));

	valid_char.mainStruct->SetField(temp_char.mainStruct->GetField("LawfulChaotic"));
	valid_char.mainStruct->SetField(temp_char.mainStruct->GetField("GoodEvil"));

	if (temp_char.mainStruct->GetField("Race") != NULL)
	{
		CGFFField appearance("Appearance_Type");
		appearance.dataType = GFF_FIELD_TYPE_WORD;
		appearance.data = temp_char.mainStruct->GetField("Race")->data;
		valid_char.mainStruct->SetField(&appearance);
	}

	valid_char.mainStruct->SetField(temp_char.mainStruct->GetField("Appearance_Head"));
	valid_char.mainStruct->SetField(temp_char.mainStruct->GetField("BodyPart_LBicep"));
	valid_char.mainStruct->SetField(temp_char.mainStruct->GetField("BodyPart_LFArm"));
	valid_char.mainStruct->SetField(temp_char.mainStruct->GetField("BodyPart_LFoot"));
	valid_char.mainStruct->SetField(temp_char.mainStruct->GetField("BodyPart_LHand"));
	valid_char.mainStruct->SetField(temp_char.mainStruct->GetField("BodyPart_LShin"));
	valid_char.mainStruct->SetField(temp_char.mainStruct->GetField("BodyPart_LThigh"));
	valid_char.mainStruct->SetField(temp_char.mainStruct->GetField("BodyPart_Pelvis"));
	valid_char.mainStruct->SetField(temp_char.mainStruct->GetField("BodyPart_RBicep"));
	valid_char.mainStruct->SetField(temp_char.mainStruct->GetField("BodyPart_RFArm"));
	valid_char.mainStruct->SetField(temp_char.mainStruct->GetField("BodyPart_RHand"));
	valid_char.mainStruct->SetField(temp_char.mainStruct->GetField("BodyPart_RShin"));
	valid_char.mainStruct->SetField(temp_char.mainStruct->GetField("BodyPart_RThigh"));
	valid_char.mainStruct->SetField(temp_char.mainStruct->GetField("BodyPart_Torso"));

	valid_char.mainStruct->SetField(temp_char.mainStruct->GetField("Color_Hair"));
	valid_char.mainStruct->SetField(temp_char.mainStruct->GetField("Color_Skin"));
	valid_char.mainStruct->SetField(temp_char.mainStruct->GetField("Color_Tattoo1"));
	valid_char.mainStruct->SetField(temp_char.mainStruct->GetField("Color_Tattoo2"));

	CGFFField* phenotype = temp_char.mainStruct->GetField("Phenotype");
	if (phenotype != NULL && phenotype->dataType == GFF_FIELD_TYPE_INT)
	{
		if (phenotype->data != (char*)0 && phenotype->data != (char*)1)
		{
			phenotype->data = (char*)0;
		}
		valid_char.mainStruct->SetField(phenotype);
	}

	valid_char.mainStruct->SetField(temp_char.mainStruct->GetField("Cha"));
	valid_char.mainStruct->SetField(temp_char.mainStruct->GetField("Con"));
	valid_char.mainStruct->SetField(temp_char.mainStruct->GetField("Dex"));
	valid_char.mainStruct->SetField(temp_char.mainStruct->GetField("Int"));
	valid_char.mainStruct->SetField(temp_char.mainStruct->GetField("Str"));
	valid_char.mainStruct->SetField(temp_char.mainStruct->GetField("Wis"));

	valid_char.mainStruct->SetField(temp_char.mainStruct->GetField("ClassList"));
	valid_char.mainStruct->SetField(temp_char.mainStruct->GetField("FeatList"));
	valid_char.mainStruct->SetField(temp_char.mainStruct->GetField("SkillList"));
	valid_char.mainStruct->SetField(temp_char.mainStruct->GetField("LvlStatList"));
	valid_char.mainStruct->SetField(temp_char.mainStruct->GetField("SkillPoints"));

	valid_char.mainStruct->SetField(temp_char.mainStruct->GetField("FamiliarName"));
	valid_char.mainStruct->SetField(temp_char.mainStruct->GetField("FamiliarType"));
	valid_char.mainStruct->SetField(temp_char.mainStruct->GetField("CompanionName"));
	valid_char.mainStruct->SetField(temp_char.mainStruct->GetField("CompanionType"));

	valid_char.SaveToFile(filename);

	return true;
}

int AddEncapsulatedResourceFile_Hook(CExoResMan* resman, CExoString* filename)
{
	//fprintf(stderr, "AddEncapsulatedResourceFile_Hook:%s\n", filename->text);
	return 1;
}
int AddResourceImageFile_Hook(CExoResMan* resman, CExoString* filename, uint8_t* p3)
{
	//fprintf(stderr, "AddResourceImageFile_Hook:%s\n", filename->text);
	return 1;
}
int AddFixedKeyTableFile_Hook(CExoResMan* resman, CExoString* filename)
{
	//fprintf(stderr, "AddFixedKeyTableFile_Hook:%s\n", filename->text);
	return 1;
}
std::string last_added_resource_directory;
CExoString (*ResolveFileName)(CExoAliasList*, CExoString*, uint16_t) = (CExoString (*)(CExoAliasList*, CExoString*, uint16_t))0x082C9498;
int AddResourceDirectory_Hook(CExoResMan* resman, CExoString* nwn_path)
{
	//fprintf(stderr, "AddResourceDirectory_Hook:%s\n", nwn_path->text);
	CExoString filepath  = ResolveFileName((*p_exo_base)->exo_aliases, nwn_path, -1);
	last_added_resource_directory = filepath.text ? filepath.text : "";
	return 1;
}
int RemoveEncapsulatedResourceFile_Hook(CExoResMan* resman, CExoString* filename)
{
	return 1;
}
int RemoveResourceImageFile_Hook(CExoResMan* resman, CExoString* filename)
{
	return 1;
}
int RemoveFixedKeyTableFile_Hook(CExoResMan* resman, CExoString* filename)
{
	return 1;
}
int RemoveResourceDirectory_Hook(CExoResMan* resman, CExoString* filename)
{
	//fprintf(stderr, "RemoveResourceDirectory_Hook:%s\n", filename->text);
	return 1;
}
int UpdateEncapsulatedResourceFile(CExoResMan* resman, CExoString* filename)
{
	return 1;
}
int UpdateFixedKeyTableFile(CExoResMan* resman, CExoString* filename)
{
	return 1;
}
int UpdateResourceDirectory(CExoResMan* resman, CExoString* filename)
{
	return 1;
}
void* GetResOfType_Hook(CExoResMan* resman, uint16_t type, CRes* res)
{
	fprintf(stderr, "GetResOfType_Hook called!\n");
	return NULL;
}
std::string get_resource_path(ResManResource resource, struct stat* file_info)
{
	using namespace aurora;
	auto res_type = resource.type;
	std::string str_resref(resource.resref.value, 16);
	std::string res_filename = resource.GetFileName();
	std::string res_path;

   	//special first case for all resources
	res_path = module_top_haks_dir + "/" + res_filename;
	if (stat(res_path.c_str(), file_info) == 0)
	{
		return res_path;
	}
	res_path = module_core_dir + "/" + res_filename;
	if (stat(res_path.c_str(), file_info) == 0)
	{
		return res_path;
	}

	//special first case for 2das
	if (res_type == NwnResType_2DA)
	{
		for (folders_2da_iter=folders_2da.begin(); folders_2da_iter!=folders_2da.end(); folders_2da_iter++)
		{
			res_path = *folders_2da_iter + res_filename;
			if (stat(res_path.c_str(), file_info) == 0)
			{
				return res_path;
			}
		}
	}

	//look in ERFs
    if (res_type == NwnResType_UTC || res_type == NwnResType_UTP || res_type == NwnResType_UTD ||
		res_type == NwnResType_UTI || res_type == NwnResType_UTW || res_type == NwnResType_UTE ||
		res_type == NwnResType_UTS || res_type == NwnResType_UTT || res_type == NwnResType_UTM ||
		res_type == NwnResType_GIT || res_type == NwnResType_ARE || res_type == NwnResType_NCS ||
		res_type == NwnResType_DLG || res_type == NwnResType_2DA)
	{
		RESERVED_PREFIX_DATA* reserved_prefix_data = get_res_prefix_data(str_resref);
		if (reserved_prefix_data)
		{
			if (reserved_prefix_data->prefix == "ph")
			{
				res_path = "/sinfar/nwn/modules/houses/" + boost::lexical_cast<std::string>(current_server_id&sinfar_vault_id?1:current_server_id) + "/" + res_filename;
			}
			else
			{
				res_path = "/sinfar/nwn/modules/resources/" + reserved_prefix_data->prefix + "_/" + res_filename;
			}
		}
        else
        {
			res_path = "/sinfar/nwn/modules/resources/default_/" + res_filename;
		}
	}
	else if (res_type == NwnResType_BIC)
	{
		res_path = last_added_resource_directory + "/" + res_filename;
		return (stat(res_path.c_str(), file_info) == 0 ? res_path : "");
	}
	//default process
	if (stat(res_path.c_str(), file_info) != 0)
	{
		res_path = "/sinfar/nwn/modules/sf_server_res/" + res_filename;
		if (stat(res_path.c_str(), file_info) != 0)
		{
			res_path = module_bottom_haks_dir + "/" + res_filename;
			if (stat(res_path.c_str(), file_info) != 0)
			{
				res_path = "/sinfar/nwn/override/" + res_filename;
				return (stat(res_path.c_str(), file_info) == 0 ? res_path : "");
			}
		}
	}
	return res_path;
}
std::string get_resource_path(aurora::NwnResType type, CResRef resref)
{
	struct stat file_info;
	return get_resource_path(ResManResource(type, resref), &file_info);
}
void load_res(ResManResource& resource, CRes* res)
{
	struct stat file_info;
	std::string res_path = get_resource_path(resource, &file_info);
	if (!res_path.empty() && file_info.st_size > 0)
	{
		if (res->data==NULL || resource.last_mod < file_info.st_mtime)
		{
			if (resource.type == aurora::NwnResType_BIC)
			{
				const char* path = res_path.c_str();
				if (strncmp(path, "./temp.", 7) == 0 && strcmp(path+strlen(path)-14, "/temp_char.bic") == 0)
				{
					if (!ValidateNewCharFile(path)) return;
				}
			}
			FILE* f = fopen(res_path.c_str(), "rb");
			if (f)
			{
                if (res->data)
                {
                    free(res->data);
                }
                res->data = (char*)malloc(file_info.st_size);
                res->size = fread(res->data, 1, file_info.st_size, f);
				fclose(f);
                if (resource.type == aurora::NwnResType_NCS ||
                    resource.type == aurora::NwnResType_DWK ||
                    resource.type == aurora::NwnResType_PWK ||
                    resource.type == aurora::NwnResType_WOK ||
					resource.type == aurora::NwnResType_2DA)
                {
                    ((CResNCS*)res)->loaded2 = 0;
                }
                resource.last_mod = file_info.st_mtime;
                res->vtable->OnResourceServiced(res);
			}
			else
			{
				fprintf(stderr, "failed to open:%s\n", res_path.c_str());
			}
		}
	}
	else
	{
		if (resource.type != aurora::NwnResType_PWK &&
			resource.type != aurora::NwnResType_NCS)
		{
			if (resource.type == aurora::NwnResType_ARE || resource.type == aurora::NwnResType_IFO)
			{
				if (!module_loaded)
				{
					fprintf(stderr, "res path not found or empty:%s\n", resource.GetFileName().c_str());
				}
			}
		}
	}
}
void SetResObject_Hook(CExoResMan* resman, CResRef& resref, uint16_t type, CRes* res)
{
	ResManResource aurora_res(static_cast<aurora::NwnResType>(type), resref);
	resources_iter = resources.find(aurora_res);
	if (resources_iter != resources.end())
	{
		CRes* old_res = resources_iter->second;
		old_res->vtable->OnResourceFreed(old_res);
		old_res->vtable->Destructor(old_res, 3);
	}
	res->entry_info = (void*)&(resources.insert(std::pair<ResManResource, CRes*>(aurora_res, res)).first->first);
}
unsigned char d_ret_code_destroyres[0x20];
void (*DestroyRes_Org)(CRes*, int) = (void (*)(CRes*, int))&d_ret_code_destroyres;
void DestroyRes_Hook(CRes* res, int p2)
{
	DestroyRes_Org(res, p2);
	if (res->entry_info)
	{
		resources_iter = resources.find(*(ResManResource*)res->entry_info);
		if (resources_iter != resources.end())
		{
			resources.erase(resources_iter);
		}
        else
        {
            fprintf(stderr, "entry info is set but is not a key in resources map\n");
        }
	}
}
CRes* GetResObject_Hook(CExoResMan* resman, CResRef& resref, uint16_t type)
{
	ResManResource aurora_res(static_cast<aurora::NwnResType>(type), resref);
	resources_iter = resources.find(aurora_res);
	if (resources_iter != resources.end())
	{
		return resources_iter->second;
	}
	return NULL;
}
void* DemandRes_Hook(CExoResMan* resman, CRes* res)
{
	if (res && res->entry_info)
	{
		ResManResource* p_resource = (ResManResource*)res->entry_info;
		load_res(*p_resource, res);
		return res->data;
	}
	fprintf(stderr, "can't respond to demanded res:%lx\n", (long)res);
	return NULL;
}
int ResExists_Hook(CExoResMan* resman, CResRef& resref, uint16_t type, uint32_t* table)
{
	ResManResource aurora_res(static_cast<aurora::NwnResType>(type), resref);
	struct stat file_info;
	if (get_resource_path(aurora_res, &file_info).empty())
	{
		//ERF files are not used anyway
		if (type != aurora::NwnResType_MOD &&
			type != aurora::NwnResType_ERF &&
			type != aurora::NwnResType_HAK)
		{
			return false;
		}
	}
	return true;
}
int GetTableCount_Hook(CExoResMan* resman, CRes* res, int p3)
{
	return 0;
}
void (*FreeResData)(CExoResMan*, CRes*) = (void (*)(CExoResMan*, CRes*))0x82b3990;
int ReleaseRes_Hook(CExoResMan* resman, CRes* res)
{
	return true;
}
int ReleaseResObject_Hook(CExoResMan* resman, CRes* res)
{
	return true;
}
void FreeResData_Hook(CExoResMan* resman, CRes* res)
{
	free_res_data(res);
}
int FreeRes_Hook(CExoResMan* resman, CRes* res)
{
	fprintf(stderr, "FreeRes_Hook called !!!!\n");
	return true;
}
int FreeChunk_Hook(CExoResMan* resman)
{
	fprintf(stderr, "FreeChunk_Hook called !!!!\n");
	return true;
}
int RequestRes_Hook(CExoResMan* resman, CRes* res)
{
	return true;
}
int CancelRequestRes_Hook(CExoResMan* resman, CRes* res)
{
	return true;
}
unsigned char d_ret_code_loadmodprogres[0x20];
int (*LoadModuleInProgress_Org)(CNWSModule*, int, int) = (int (*)(CNWSModule*, int, int))&d_ret_code_loadmodprogres;
int LoadModuleInProgress_Hook(CNWSModule* module, int area_index, int p3)
{
	int ret = LoadModuleInProgress_Org(module, area_index, p3);
	if (ret != 0)
	{
		(*p_app_manager)->loading_areas->size--;
		(*p_app_manager)->loading_areas->current_index++;
	}
	return 0;
}
int (*CExoFile_Init)(CExoFile*, CExoString*, uint16_t, CExoString*) = (int (*)(CExoFile*, CExoString*, uint16_t, CExoString*))0x82C94C0;
int OnLoadModule_OpenModFile(CExoFile* exo_file, CExoString* nwn_path, uint16_t type, CExoString* mode)
{
	CExoString empty_module_path("MODULES:empty");
	return 	CExoFile_Init(exo_file, &empty_module_path, type, mode);
}
CExoString OnLoadModule_ResolveFileName(CExoAliasList* exo_alias, CExoString* nwn_path, uint16_t type)
{
	return CExoString("./modules/empty.mod");
}
//debugging tools...
int (*CExoFile_IsOpened)(CExoFile*) = (int (*)(CExoFile*))0x082c957c;
unsigned char d_ret_code_cexofile[0x20];
int (*CExoFile_Init_Org)(CExoFile*, CExoString*, uint16_t, CExoString*) = (int (*)(CExoFile*, CExoString*, uint16_t, CExoString*))&d_ret_code_cexofile;
int CExoFile_Init_Hook(CExoFile* exo_file, CExoString* nwn_path, uint16_t type, CExoString* mode)
{
	int ret = CExoFile_Init_Org(exo_file, nwn_path, type, mode);
	if (!CExoFile_IsOpened(exo_file))
	{
		fprintf(stderr, "failed to open file: %s\n", nwn_path->text);
		char wait_debugger;
		scanf("%c", &wait_debugger);
	}
	return ret;
}//*/
void NoLog_Hook(void* exo_debug, const char *format, ...)
{
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
}//*/
unsigned char d_ret_code_rsfilename[0x20];
CExoString (*ResolveFileName_Org)(CExoAliasList*, CExoString*, uint16_t) = (CExoString (*)(CExoAliasList*, CExoString*, uint16_t))&d_ret_code_rsfilename;
CExoString ResolveFileName_Hook(CExoAliasList* exo_alias, CExoString* nwn_path, uint16_t type)
{
	CExoString ret = ResolveFileName_Org(exo_alias, nwn_path, type);
	fprintf(stderr, "%s resolved to: %s\n", nwn_path->text, ret.text);
	/*if (ret.text && strncmp(nwn_path->text, "MODULES:", 8)==0)
	{
		char wait_debugger;
		scanf("%c", &wait_debugger);
	}*/
	return ret;
}//*/

void init()
{
    module_core_dir = "./modules/servers_core/"+boost::lexical_cast<std::string>(current_server_port-5120);
	module_top_haks_dir = "./modules/servers_haks/"+boost::lexical_cast<std::string>(current_server_port-5120)+"/top_built";
	module_bottom_haks_dir = "./modules/servers_haks/"+boost::lexical_cast<std::string>(current_server_port-5120)+"/bottom_built";

	folders_2da.push_back("/sinfar/nwn/modules/resources/"+mysql_admin->query_string("SELECT core_erf FROM servers WHERE port=" + std::to_string(current_server_port))+"_/");
	folders_2da.push_back("/sinfar/nwn/modules/resources/"+mysql_admin->query_string("SELECT core_erf FROM servers WHERE port=5121")+"_/");

    hook_function(0x082b37e4, (unsigned long)AddEncapsulatedResourceFile_Hook, d_ret_code_nouse, 10);
	hook_function(0x082b3800, (unsigned long)AddResourceImageFile_Hook, d_ret_code_nouse, 11);
	hook_function(0x082b381c, (unsigned long)AddFixedKeyTableFile_Hook, d_ret_code_nouse, 10);
	hook_function(0x082b3838, (unsigned long)AddResourceDirectory_Hook, d_ret_code_nouse, 10);
	hook_function(0x082b3854, (unsigned long)RemoveEncapsulatedResourceFile_Hook, d_ret_code_nouse, 11);
	hook_function(0x082b386c, (unsigned long)RemoveResourceImageFile_Hook, d_ret_code_nouse, 11);
	hook_function(0x082b3884, (unsigned long)RemoveFixedKeyTableFile_Hook, d_ret_code_nouse, 11);
	hook_function(0x082b389c, (unsigned long)RemoveResourceDirectory_Hook, d_ret_code_nouse, 11);
	hook_function(0x082b38b4, (unsigned long)UpdateEncapsulatedResourceFile, d_ret_code_nouse, 11);
	hook_function(0x082b38cc, (unsigned long)UpdateFixedKeyTableFile, d_ret_code_nouse, 11);
	hook_function(0x082b38e4, (unsigned long)UpdateResourceDirectory, d_ret_code_nouse, 11);
	hook_function(0x082b39f4, (unsigned long)GetResOfType_Hook, d_ret_code_nouse, 10);
	hook_function(0x082b3a80, (unsigned long)SetResObject_Hook, d_ret_code_nouse, 10);
	hook_function(0x082b3668, (unsigned long)DestroyRes_Hook, d_ret_code_destroyres, 12);
	hook_function(0x082b3a30, (unsigned long)GetResObject_Hook, d_ret_code_nouse, 10);
	hook_function(0x082af408, (unsigned long)DemandRes_Hook, d_ret_code_nouse, 12);
	hook_function(0x082B393C, (unsigned long)ResExists_Hook, d_ret_code_nouse, 12);
	hook_function(0x082b0684, (unsigned long)GetTableCount_Hook, d_ret_code_nouse, 12);
	hook_function(0x082b075c, (unsigned long)ReleaseRes_Hook, d_ret_code_nouse, 12);
	hook_function(0x082b3990, (unsigned long)FreeResData_Hook, d_ret_code_nouse, 10);
	hook_function(0x082afbb8, (unsigned long)FreeRes_Hook, d_ret_code_nouse, 12);
	hook_function(0x082afc7c, (unsigned long)FreeChunk_Hook, d_ret_code_nouse, 12);
	hook_function(0x082b3d48, (unsigned long)ReleaseResObject_Hook, d_ret_code_nouse, 10);
	hook_function(0x082b3db0, (unsigned long)RequestRes_Hook, d_ret_code_nouse, 10);
	hook_function(0x082af2e4, (unsigned long)CancelRequestRes_Hook, d_ret_code_nouse, 11);
	//ignore expansion pack (error #7)
	/*enable_write(0x081b4e40);
	*(uint16_t*)0x081b4e40 = 0xE990;//*/
	//do not fail to load the module if an area fail to be created
	hook_function(0x081B8584, (unsigned long)LoadModuleInProgress_Hook, d_ret_code_loadmodprogres, 12);
	//the module file is not needed
	hook_call(0x0809EDA0, (uint32_t)OnLoadModule_OpenModFile);
	hook_call(0x0809F1C5, (uint32_t)OnLoadModule_OpenModFile);
	hook_call(0x080a01ed, (uint32_t)OnLoadModule_OpenModFile);
	hook_call(0x0809F01C, (uint32_t)OnLoadModule_ResolveFileName);
	
	//hook_function(0x082c94c0, (unsigned long)CExoFile_Init_Hook, d_ret_code_cexofile, 12);
	//hook_function(0x082c9498, (unsigned long)ResolveFileName_Hook, d_ret_code_rsfilename, 10);
	//hook_function(0x08094078, (unsigned long)NoLog_Hook, d_ret_code_nouse, 6);
	
	register_hook(hook::module_loading, []{
		//free all resources as most of them are not needed anymore
		for (auto& resource_iter: resources)
		{
			free_res_data(resource_iter.second);
		}
	});
}
REGISTER_INIT(init);

VM_FUNC_NEW(GetResExists, 7)
{
	CExoString res_filename = vm_pop_string();
	int result = false;
	if (res_filename.text)
	{
		char* res_ext = strrchr(res_filename.text, '.');
		if (res_ext)
		{
			*res_ext = 0;
			res_ext++;
			CResRef resref;
			strncpy(resref.value, res_filename.text, 16);
			struct stat file_info;
			result = (!get_resource_path(ResManResource(aurora::NwnGetResTypeFromExtension(res_ext), resref), &file_info).empty());
		}
	}
	vm_push_int(result);
}
void (*GetSetSectionEntryValue)(CResSET*, const char*, const char*, char*) = (void (*)(CResSET*, const char*, const char*, char*))0x080957F4;
CNWTileSet* (*RegisterTileSet)(CNWTileSetManager*, CResRef) = (CNWTileSet* (*)(CNWTileSetManager*, CResRef))0x08086BA4;
VM_FUNC_NEW(GetTilesetInformation, 353)
{
	CResRef tileset_resref(vm_pop_string().text);
	std::string section = vm_pop_string();
	std::string entry = vm_pop_string();
	CNWTileSet** tilesets = (*p_app_manager)->tileset_manager->tilesets;
	CResSET* res = NULL;
	for (uint32_t i=0; i<64; i++)
	{
		if (tilesets[i] == NULL)
		{
			if (!get_resource_path(aurora::NwnResType_SET, tileset_resref).empty())
			{
				CNWTileSet* tileset = RegisterTileSet((*p_app_manager)->tileset_manager, tileset_resref);
				if (tileset)
				{
					res = tileset->res;
				}
			}
			break;
		}
		else if (strncmp(tileset_resref.value, tilesets[i]->resref.value, 16) == 0)
		{
			res = tilesets[i]->res;
			break;
		}
	}
	char result_c_str[1000];
	result_c_str[0] = 0;
	if (res)
	{
		GetSetSectionEntryValue(res, section.c_str(), entry.c_str(), result_c_str);
	}
	CExoString result(result_c_str);
	vm_push_string(&result);
}

}
}
