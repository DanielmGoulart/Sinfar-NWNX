#include <windows.h>
#include <WinBase.h>
#include <stdio.h>
#include <tchar.h>
#include <pluginapi.h>
#include "detours.h"
#include <string>
#include <regex>
#include "Winsock.h"
#include "StackWalker.h"
#include <Psapi.h>
#include "../SinfarXLib/SinfarX.h"

using namespace sinfarx;

#include <nwncx/sinfar/nwncx_sinfar.h>
#include <nwncx/sinfar/resman.h>
using namespace nwncx;
using namespace nwncx::sinfar;

//////////////////////////////////////////////////////////////////////////
PLUGINLINK *pluginLink = 0;
PLUGININFO pluginInfo={
	sizeof(PLUGININFO),
	"NWNCX Sinfar",
	PLUGIN_MAKE_VERSION(0,0,0,2),
	"Various things for Sinfar",
	"Mavrixio",
	"mavrixio@sinfar.net",
	"© 2013 Mavrixio",
	"http://sinfar.net",
	0		//not transient
};

extern "C" __declspec(dllexport) PLUGININFO* GetPluginInfo(DWORD nwnxVersion)
{
	return &pluginInfo;
}

extern "C" __declspec(dllexport) int InitPlugin(PLUGINLINK* link)
{
	pluginLink=link;
	return 0;
}

char player_name[64];
char* __fastcall WritePlayerNameCStr(CExoString* self)
{
	sprintf_s(player_name, 64, "%s", self->text);
	return player_name;
}

int (__fastcall *DestroyClientCreature)(CNWCCreature*) = (int (__fastcall *)(CNWCCreature*))0x004BF0B0;
int __fastcall DestroyClientCreature_Hook(CNWCCreature* creature)
{
	on_destroy_creature(creature);
	return DestroyClientCreature(creature);
}

bool (__fastcall *SetObjectScale)(Gob*, int, float, bool) = (bool (__fastcall *)(Gob*, int, float, bool))0x007B0FE0;
bool __fastcall SetObjectScale_Hook(Gob* gob, int edx, float scale, bool root)
{
	//check that child gobs are valid
	for (DWORD child_index=0; child_index<gob->children.size; child_index++)
	{
		if (gob->children.data[child_index] == NULL)
		{
			log_printf("%s invalid child\n", get_timestamp());
			return 1;
		}
	}
	return SetObjectScale(gob, edx, scale, root);
}
char* (__fastcall *GetGobModelName)(Gob*) = (char* (__fastcall *)(Gob*))0x007AE6A0;
DWORD (__fastcall *GetEquippedItemId)(CNWCCreature*, int, DWORD) = (DWORD (__fastcall *)(CNWCCreature*, int, DWORD))0x004C7E40;
CNWCAnimBase* (__fastcall *GetAnimBase)(void*) = (CNWCAnimBase* (__fastcall *)(void*))0x0048FA10;
Gob* (__fastcall *GetNWCObjectModel)(void*, int, BYTE) = (Gob* (__fastcall *)(void*, int, BYTE))0x0048FA20;
CExoString (__fastcall *GetItemName)(CNWCItem*, int, int) = (CExoString (__fastcall *)(CNWCItem*, int, int))0x004EA6E0;

int (__fastcall *HandleServerToPlayerCreatureUpdate_Appearance)(void*) = (int (__fastcall *)(void*))0x00448E30;
int __fastcall HandleServerToPlayerCreatureUpdate_Appearance_Hook(void* message)
{
	int ret = HandleServerToPlayerCreatureUpdate_Appearance(message);
	CNWCCreature* creature = creature_by_id(creature_update_appearance_id);
	if (creature)
	{
		update_creature_extra(creature);
	}
	return ret;
}

DWORD __fastcall ReadObjUpdateId(CNWMessage* message)
{
	return read_creature_appearance_obj_id(message);
}


bool bypass_gob_highlight = false;
int (__fastcall *HiliteCreature)(CNWCCreature*, int, Vector, BYTE, float, int) = (int (__fastcall *)(CNWCCreature*, int, Vector, BYTE, float, int))0x004CEF70;
int __fastcall HiliteCreature_Hook(CNWCCreature* creature, int edx, Vector color, BYTE type, float p4, int p5)
{
	if (type == 1)
	{
		bypass_gob_highlight = true;
		int ret = HiliteCreature(creature, edx, color, type, p4, p5);
		bypass_gob_highlight = false;
		return ret;
	}
	else
	{
		return HiliteCreature(creature, edx, color, type, p4, p5);
	}
}

void (__fastcall *AddHighlightGob)(void*, int, void*, Vector*) = (void (__fastcall *)(void*, int, void*, Vector*))0x007932F0;
void __fastcall AddHighlightGob_Hook(void* scene, int edx, void* aur_object, Vector* vector)
{
	if (!bypass_gob_highlight) AddHighlightGob(scene, edx, aur_object, vector);
}

void** p_resman = (void**)0x0092DC3C;
bool (__fastcall *ResourceExists)(void*, int, char*, WORD, DWORD*) = (bool (__fastcall *)(void*, int, char*, WORD, DWORD*))0x005CC1C0;

///////////////////// weapon scaling ////////////////////////////////////
long (*ftol)(float) = (long (*)(float))0x0084C1EC;
long ftol_notzero(float f)
{
	long result = ftol(f);
	if (result == 0) result = 1;
	return result;
}
///////////////////// end weapon scaling /////////////////////////////////

void (__fastcall *RenderScene)(void*) = (void (__fastcall *)(void*))0x00799100;
void __fastcall RenderScene_Hook(void* scene)
{
	on_render();
	RenderScene(scene);
}

//random port
int __fastcall get_port_int(CExoString* port)
{
	return 0;
}

int (__fastcall *LoadBeam)(void*, int, WORD) = (int (__fastcall *)(void*, int, WORD))0x005740E0;
int __fastcall LoadBeam_Hook(void* vfx_on_object, int edx, WORD ProgFX_Duration)
{
	if (ProgFX_Duration > 612 && ProgFX_Duration < 1000)
	{
		C2DA* two_dimension_array = get_cached_2da("vfx_beams");
		if (two_dimension_array)
		{
			CExoString value;
			WORD beam_index = ProgFX_Duration-600;
			get_2da_string(two_dimension_array, beam_index, &CExoString("Model"), &value);
			if (value.text)
			{
				*(DWORD*)0x00574127 = (DWORD)(value.text);
				int ret = LoadBeam(vfx_on_object, edx, 600);
				*(DWORD*)0x00574127 = 0x0090C018;
				return ret;
			}
		}
	}
	return LoadBeam(vfx_on_object, edx, ProgFX_Duration);
}

/////////////////////// crash logging //////////////////////////
class MyStackWalker : public StackWalker
{
public:
	MyStackWalker() : StackWalker() {}
	MyStackWalker(DWORD dwProcessId, HANDLE hProcess) : StackWalker(dwProcessId, hProcess) {}
	virtual void OnCallstackEntry(CallstackEntryType eType, CallstackEntry &entry) 
	{
		log_printf("0x%X\n", entry.offset);
	}
};
bool process_handles_dumped = false;
DWORD crash_count = 0;
LONG WINAPI CrashLog(EXCEPTION_POINTERS* pExp, DWORD dwExpCode, const char* hook_id)
{
	crash_count++;
	if (crash_count >= 100) exit(1);

	std::string last_demand_res_filename_str;
	for (auto iter = last_demand_res_filename.begin(); iter!=last_demand_res_filename.end(); iter++)
	{
		last_demand_res_filename_str += *iter + ";";
	}
	log_printf("%s %s: crash:0x%08x last res:%s \n", get_timestamp(), hook_id, (pExp->ExceptionRecord ? pExp->ExceptionRecord->ExceptionAddress : NULL), last_demand_res_filename_str.c_str());
	MyStackWalker sw;
	sw.ShowCallstack(GetCurrentThread(), pExp->ContextRecord);
	if (pExp->ContextRecord)
	{
		log_printf("eax:0x%X ebx:0x%X ecx:0x%X edx:0x%X esi:0x%X edi:0x%X esp:0x%X ebp:0x%X eip:0x%X\n", 
			pExp->ContextRecord->Eax,
			pExp->ContextRecord->Ebx,
			pExp->ContextRecord->Ecx,
			pExp->ContextRecord->Edx,
			pExp->ContextRecord->Esi,
			pExp->ContextRecord->Edi,
			pExp->ContextRecord->Esp,
			pExp->ContextRecord->Ebp,
			pExp->ContextRecord->Eip);
	}
	if (!process_handles_dumped)
	{
		HMODULE modules[1024];
		DWORD num_bytes;
		if (EnumProcessModules(GetCurrentProcess(), modules, sizeof(modules), &num_bytes))
		{
			DWORD num_modules = num_bytes/sizeof(HMODULE);
			for (DWORD module_index=0; module_index<num_modules; module_index++)
			{
				char module_filename[MAX_PATH];
				GetModuleFileNameA(modules[module_index], module_filename, MAX_PATH);
				MODULEINFO module_info;
				GetModuleInformation(GetCurrentProcess(), modules[module_index], &module_info, sizeof(module_info));
				log_printf("0x%08x-0x%08x 0x%08x (%s)\n", module_info.lpBaseOfDll, (DWORD)module_info.lpBaseOfDll+module_info.SizeOfImage, module_info.lpBaseOfDll, module_filename);
			}
		}
		process_handles_dumped = true;
	}
	return EXCEPTION_EXECUTE_HANDLER;
}
void PrintStackTrace(const std::string& info)
{
	__try
	{
		*(char*)NULL = NULL;
	}
	__except (CrashLog(GetExceptionInformation(), GetExceptionCode(), info.c_str())) {
	}
}
//////////////////////////////// end crash logging ////////////////////////////////////////

////////////////////////////////////// crash fix /////////////////////////////////////////
void* __cdecl FindModel_Hook(char* model_name)
{
	__try{      
		return find_model_hook(model_name);
	}
	__except(CrashLog(GetExceptionInformation(), GetExceptionCode(), "find model")){
		log_printf("model name:%s\n", model_name);
		return NULL;
	}
}
BOOL WINAPI MyHeapFree(HANDLE hHeap, DWORD dwFlags, LPVOID lpMem)
{
	__try{      
	  return HeapFree(hHeap, dwFlags, lpMem);
	}
	__except(CrashLog(GetExceptionInformation(), GetExceptionCode(), "free mem")){
		return 0;
	}
}
LPVOID WINAPI MyHeapAlloc(HANDLE hHeap, DWORD dwFlags, SIZE_T dwBytes)
{
	__try{      
	  return HeapAlloc(hHeap, dwFlags, dwBytes);
	}
	__except(CrashLog(GetExceptionInformation(), GetExceptionCode(), "alloc")){
		return malloc(dwBytes);
	}
}
int (__fastcall *MdlNodeDestructor)(void*) = (int (__fastcall *)(void*))0x007A52F0;
int __fastcall MdlNodeDestructor_Hook(void* mdl_node)
{
	__try{      
	  return MdlNodeDestructor(mdl_node);
	}
	__except(CrashLog(GetExceptionInformation(), GetExceptionCode(), "destroy node")){
		return 0;
	}
}
void (__fastcall *RenderSkyboxes)(void*) = (void (__fastcall *)(void*))0x007A0400;
void __fastcall RenderSkyboxes_Hook(void* scene)
{
	__try{ //*(char*)0 = 0;      
		return RenderSkyboxes(scene);
	}
	__except(CrashLog(GetExceptionInformation(), GetExceptionCode(), "render skypbox")){
	}
}
void (__fastcall *SetGlobalWind)(void*, int, Vector) = (void (__fastcall *)(void*, int, Vector))0x007DEB40;
void __fastcall SetGlobalWind_Hook(void* wind_manager, int edx, Vector v)
{
	__try{      
		return SetGlobalWind(wind_manager, edx, v);
	}
	__except(CrashLog(GetExceptionInformation(), GetExceptionCode(), "set global wind")){
	}
}
int (__fastcall *AddStaticPlc)(char*, int, DWORD, WORD, Vector, Vector) = (int (__fastcall *)(char*, int, DWORD, WORD, Vector, Vector))0x0048DAA0;
int __fastcall OnLoadArea_AddStaticPlc(char* area, int edx, DWORD p3, WORD p4, Vector position, Vector orientation)
{
	__try{
		return AddStaticPlc(area, edx, p3, p4, position, orientation);
	}
	__except(CrashLog(GetExceptionInformation(), GetExceptionCode(), "static plc")){
		log_printf("static placeable in %s at (%d,%d) with the appearance %d is making you crash, please report it.\n", ((CExoString*)(area+0x128))->text, position.x, position.y, p4);
		return 0;
	}
}
void (__fastcall *DestroyGameObjectArray)(void*) = (void (__fastcall *)(void*))0x0042E2B0;
void __fastcall DestroyGameObjectArray_Hook(void* go_array)
{
	__try{      
		return DestroyGameObjectArray(go_array);
	}
	__except(CrashLog(GetExceptionInformation(), GetExceptionCode(), "destroy game object array")){
	}
}
void* (__fastcall *GetAttachedModel)(void*, int, int) = (void* (__fastcall*)(void*, int, int))0x007B45F0;
void* __fastcall GetAttachedModel_Hook(void* string_gob, int edx, int p3)
{
	__try{      
		return GetAttachedModel(string_gob, edx, p3);
	}
	__except(CrashLog(GetExceptionInformation(), GetExceptionCode(), "attached model")){
		return NULL;
	}
}
void* __fastcall OnAIUpdate_GetAnimBase(void* object)
{
	void* ret = GetAnimBase(object);
	if (ret == NULL)
	{
		log_printf( "1\n");
		*(DWORD*)0x004BFC98 = 0x90909090;
		*(WORD*)(0x004BFC98+4) = 0x9090;
		*(WORD*)(0x004BFC98+6) = 0xE990;
	}
	else
	{
		*(DWORD*)0x004BFC98 = 0x00CCA839;
		*(WORD*)(0x004BFC98+4) = 0x0000;
		*(WORD*)(0x004BFC98+6) = 0x840F;
	}
	return ret;
}
void (__fastcall *ArturoTextureController_Control)(void*) = (void (__fastcall *)(void*))0x007CC210;
void __fastcall ArturoTextureController_Control_Hook(void* controller)
{
	__try{      
		return ArturoTextureController_Control(controller);
	}
	__except(CrashLog(GetExceptionInformation(), GetExceptionCode(), "control texture")){
	}
}
int (__fastcall *CAurBehaviorAttach_Control)(void*, int, float) = (int (__fastcall *)(void*, int, float))0x007C6DD0;
int __fastcall CAurBehaviorAttach_Control_Hook(void* self, int edx, float p3)
{
	__try{      
		return CAurBehaviorAttach_Control(self, edx, p3);
	}
	__except(CrashLog(GetExceptionInformation(), GetExceptionCode(), "attach control")){
		return 1;
	}
}
void (__fastcall *BindTexture)(void*, int, int) = (void (__fastcall *)(void*, int, int))0x00782E50;
void __fastcall BindTexture_Hook(void* self, int edx, int p3)
{
	__try{      
		return BindTexture(self, edx, p3);
	}
	__except(CrashLog(GetExceptionInformation(), GetExceptionCode(), "bind texture")){
	}
}
void (__fastcall *DestroyPlaceable)(void*) = (void (__fastcall *)(void*))0x005244A0;
void __fastcall DestroyPlaceable_Hook(void *plc)
{
	__try{      
		return DestroyPlaceable(plc);
	}
	__except(CrashLog(GetExceptionInformation(), GetExceptionCode(), "destroy plc")){
	}
}
void (__fastcall *DestroyInGameGuiChatwindow)(void*) = (void (__fastcall *)(void*))0x0057AE20;
void __fastcall DestroyInGameGuiChatwindow_Hook(void* self)
{
	__try{      
		return DestroyInGameGuiChatwindow(self);
	}
	__except(CrashLog(GetExceptionInformation(), GetExceptionCode(), "chat window")){
	}
}
void (__fastcall *DestroyStringGob)(void*) = (void (__fastcall *)(void*))0x007B1E80;
void __fastcall DestroyStringGob_Hook(void* string_gob)
{
	__try{      
		return DestroyStringGob(string_gob);
	}
	__except(CrashLog(GetExceptionInformation(), GetExceptionCode(), "destroy gob str")){
	}
}
void (__fastcall *DestroyAurTexture)(void*) = (void (__fastcall *)(void*))0x00783560;
void __fastcall DestroyAurTexture_Hook(void* aur_texture)
{
	__try{      
		return DestroyAurTexture(aur_texture);
	}
	__except(CrashLog(GetExceptionInformation(), GetExceptionCode(), "destroy texture")){
	}
}
void (__fastcall *DestroyGob)(Gob*) = (void (__fastcall *)(Gob*))0x007AA430;
void __fastcall DestroyGob_Hook(Gob* gob)
{
	__try{      
		return DestroyGob(gob);
	}
	__except(CrashLog(GetExceptionInformation(), GetExceptionCode(), "destroy gob")){
	}
}
void (__fastcall *DoGobShadows)(void*, int, void*) = (void (__fastcall *)(void*, int, void*))0x00796CE0;
void __fastcall DoGobShadows_Hook(void* scene, int edx, void* parts)
{
	__try{      
		return DoGobShadows(scene, edx, parts);
	}
	__except(CrashLog(GetExceptionInformation(), GetExceptionCode(), "shadow")){
	}
}
void (__fastcall *HandleSwitchWeapon)(void*) = (void (__fastcall *)(void*))0x004C02B0;
void __fastcall HandleSwitchWeapon_Hook(CNWCCreature* creature)
{
	__try{
		HandleSwitchWeapon(creature);
	}
	__except(CrashLog(GetExceptionInformation(), GetExceptionCode(), "switch weapon")){
	}
}
void (__fastcall *ProcessInput)(void*) = (void (__fastcall *)(void*))0x00424E10;
void __fastcall ProcessInput_Hook(void* server_internal)
{
	__try{      
		return ProcessInput(server_internal);
	}
	__except(CrashLog(GetExceptionInformation(), GetExceptionCode(), "input")){
	}
}
void (__cdecl *AurResFree)(void*) = (void (__cdecl *)(void*))0x00790B50;
void __cdecl AurResFree_Hook(void* res)
{
	__try{      
		return AurResFree(res);
	}
	__except(CrashLog(GetExceptionInformation(), GetExceptionCode(), "free res")){
	}
}
BYTE* (__fastcall *GetDWKData)(void*) = (BYTE* (__fastcall *)(void*))0x005BA1B0;
BYTE* __fastcall GetDWKData_Hook(void* self)
{
	__try{      
		return GetDWKData(self);
	}
	__except(CrashLog(GetExceptionInformation(), GetExceptionCode(), "dwk")){
		return NULL;
	}
}

DWORD __fastcall ReadDWORD_BaseItem(CNWMessage* message, int edx, int p3)
{
	DWORD base_item = read_msg_dword(message);
	void* base_item_object = get_base_item((*p_rules)->ru_baseitems, 0, base_item);
	if (base_item_object)
	{
		return base_item;
	}
	else
	{
		return 67;
	}
}
void (__cdecl *MaxTree_Delete)(void*) = (void (__cdecl *)(void*))0x7A4890;
void __cdecl MaxTree_Delete_Hook(void* maxtree)
{
	__try{      
		return MaxTree_Delete(maxtree);
	}
	__except(CrashLog(GetExceptionInformation(), GetExceptionCode(), "delete tree")){
	}
}
void (__fastcall *DestroyNWTileData)(void*) = (void (__fastcall *)(void*))0x50A4C0;
void __fastcall DestroyNWTileData_Hook(void* self)
{
	__try{      
		return DestroyNWTileData(self);
	}
	__except(CrashLog(GetExceptionInformation(), GetExceptionCode(), "destroy tile data")){
	}
}
void (__fastcall *DestroyPartTrimesh)(void*) = (void (__fastcall *)(void*))0x7A82C0;
void __fastcall DestroyPartTrimesh_Hook(void* self)
{
	__try{      
		return DestroyPartTrimesh(self);
	}
	__except(CrashLog(GetExceptionInformation(), GetExceptionCode(), "destroy trimesh")){
	}	
}
void(__cdecl *AtiInitializeShaders_Org)() = (void(__cdecl *)())0x007DCD30;
bool DoInitAtiShaders()
{
	__try
	{
		AtiInitializeShaders_Org();
		return true;
	}
	__except (EXCEPTION_EXECUTE_HANDLER){
		return false;
	}
}
void WINAPI glFogf_hook(
	int  pname,
	float param
)
{

}
void(__fastcall  *render_gob_org)(Gob*, int, bool) = (void(__fastcall *)(Gob*, int, bool))0x007B9790;
void __fastcall render_gob_hook(Gob* gob, int, bool b)
{
	if (strncmp(gob->name, "body", 4) == 0 || strncmp(gob->name, "robe", 4) == 0)
	{
		log_printf("render gob:%s\n", gob->name);
	}
	return render_gob_org(gob, 0, b);
}
void(__fastcall  *draw_gob_org)(Gob*, int, CAurPart*) = (void(__fastcall *)(Gob*, int, CAurPart*))0x004FBB40;
void __fastcall draw_gob_hook(Gob* gob, int, CAurPart* part)
{
	if (strncmp(gob->name, "body", 4) == 0 || strncmp(gob->name, "robe", 4) == 0)
	{
		log_printf("draw gob:%s\n", gob->name);
	}
	return draw_gob_org(gob, 0, part);
}
void(__fastcall  *read_msg_overflow_org)(Gob*, int, CAurPart*) = (void(__fastcall *)(Gob*, int, CAurPart*))0x004FBB40;
void __fastcall read_msg_overflow_hook(Gob* gob, int, CAurPart* part)
{

}

void AtiInitializeShaders_Hook()
{
	if (!DoInitAtiShaders())
	{
		/*if (MessageBoxA(
			NULL,
			"The ATI shaders are making you crash, do you want to disable them?",
			"",
			MB_ICONEXCLAMATION | MB_YESNO) == IDYES)
		{
			CIniFileA ini;
			ini.Load(ini_filename);
			ini.SetKeyValue(INI_SECTION, INI_KEY_DISABLE_ATI_SHADERS, "1");
			ini.Save(ini_filename);
			MessageBoxA(NULL, "Done! Restart NWN and it shouln't happen anymore.", "", 0);
		}
		exit(1);*/
		log_printf("Failed to initialize ATI shaders\n");
	}
}
void (__fastcall *AddTextLineToEditScroll)(void*, int, CExoString) = (void (__fastcall *)(void*, int, CExoString))0x0043B260;
void __fastcall AddTextLineToEditScroll_Hook(char* edit_scroll, int edx, CExoString line)
{     
	if (*(edit_scroll+0x28))
	{
		return AddTextLineToEditScroll(edit_scroll, edx, line);
	}
}
void (__fastcall *DestroyAurAttach)(void*) = (void (__fastcall *)(void*))0x007C6BC0;
void __fastcall DestroyAurAttach_Hook(void* aur_attach)
{
	__try{
		return DestroyAurAttach(aur_attach);
	}
	__except(CrashLog(GetExceptionInformation(), GetExceptionCode(), "aur attach")){
	}
}
int (__fastcall *HandleServerToPlayerMessage)(void*, int, BYTE*, DWORD) = (int (__fastcall *)(void*, int, BYTE*, DWORD))0x452420;
int __fastcall HandleServerToPlayerMessage_Hook(void* message, int edx, BYTE* msg, DWORD len)
{
	__try{      
		return HandleServerToPlayerMessage(message, edx, msg, len);
	}
	__except(CrashLog(GetExceptionInformation(), GetExceptionCode(), "server msg")){
		return 0;
	}
}
void (__fastcall *UpdateFountainEmitter)(void*, int, float) = (void (__fastcall *)(void*, int, float))0x007E8080;
void __fastcall UpdateFountainEmitter_Hook(void* emitter, int edx, float p2)
{
	__try{      
		return UpdateFountainEmitter(emitter, edx, p2);
	}
	__except(CrashLog(GetExceptionInformation(), GetExceptionCode(), "update fountain")){
	}
}
void (__fastcall *AnimateGob)(void*, int, float) = (void (__fastcall *)(void*, int, float))0x007ED3A0;
void __fastcall AnimateGob_Hook(Gob* gob, int edx, float p2)
{
	__try{
		return AnimateGob(gob, edx, p2);
	}
	__except(CrashLog(GetExceptionInformation(), GetExceptionCode(), "animate gob")){
		log_printf("crashy gob:%s", gob->name);
	}
}
int (__fastcall *Unload2da)(void*) = (int (__fastcall *)(void*))0x005696F0;
int __fastcall Unload2da_Hook(void* tables)
{
	__try{
		return Unload2da(tables);
	}
	__except(CrashLog(GetExceptionInformation(), GetExceptionCode(), "unload 2da")){
		return 1;
	}
}
void (__fastcall *UpdateAoEPosition)(void*) = (void (__fastcall *)(void*))0x005517F0;
void __fastcall UpdateAoEPosition_Hook(void* aoe)
{
	__try{
		UpdateAoEPosition(aoe);
	}
	__except(CrashLog(GetExceptionInformation(), GetExceptionCode(), "update aoe")){
	}
}
uint32_t(__fastcall *GetResFileSize)(CExoResFile*, int, uint32_t) = (uint32_t(__fastcall *)(CExoResFile*, int, uint32_t))0x005D63C0;
uint32_t __fastcall GetResFileSize_Hook(CExoResFile* exo_file, int edx, uint32_t p2)
{
	__try{
		return GetResFileSize(exo_file, edx, p2);
	}
	__except (CrashLog(GetExceptionInformation(), GetExceptionCode(), "get res size")){
		log_printf("failed to get res size of:%s p2:%u\n", exo_file->file_name.text, p2);
		return 0;
	}
}
void __fastcall UninitializeClientExoApp(void* client)
{
	exit(0);
}
/////////////////////////////// end crash fix //////////////////////////////////////

int (__fastcall *LoadNWNModule)(void*, int, CExoString, int, void*) = (int (__fastcall *)(void*, int, CExoString, int, void*))0x005FFE50;
int __fastcall LoadNWNModule_Hook(void* server, int edx, CExoString module_name, int p4, void* player)
{
	exit(0); //Hooks wont be compatible with that module so better crash right now

	return LoadNWNModule(server, edx, module_name, p4, player);
}

int (_cdecl *HandleWMCharMessage_Org)(uint16_t) = (int (__cdecl *)(uint16_t))0x00403D80;
int __cdecl HandleWMCharMessage_Hook(uint16_t param)
{
	if (param == 'g')
	{
		MessageBoxA(0, "go pressed", "go", 0);
		export_all_best_resources("c:/all_169_res");
		return true; //event handled
	}
	else if (param = 'f')
	{
		PrintStackTrace("HandleWMCharMessage");
		return true;
	}
	else
	{
		return HandleWMCharMessage_Org(param);
	}
}

int (__fastcall *SendDirectMessage)(void*, int, DWORD, char*, DWORD, DWORD, DWORD) = (int (__fastcall *)(void*, int, DWORD, char*, DWORD, DWORD, DWORD))0x005C5FF0;
int __fastcall SendPlayerInfo_Hook(void* exo_net, int edx, DWORD p1, char* send_data, DWORD p3, DWORD p4, DWORD p5)
{
	perform_test = false;	
	char* exo_net_internal = *(char**)exo_net;
	char* addr_info = *(char**)(exo_net_internal+0x44);
	server_ip = *(DWORD*)((addr_info+4)+(p1*16));
	int sinfar_server_type = is_sinfar_server(server_ip);
	if (sinfar_server_type && *(WORD*)(send_data+0xb) == 3)
	{
		log_printf("%s entering a Sinfar world\n", get_timestamp());

		*(WORD*)(send_data+0xb) = NWNCX_SINFAR_VERSION;

		if (!is_on_sinfar)
		{
			is_on_sinfar = sinfar_server_type;
			
			DetourTransactionBegin();
			DetourUpdateThread(GetCurrentThread());

			//crash fix
			DetourAttach((PVOID*)&MdlNodeDestructor, MdlNodeDestructor_Hook);
			DetourAttach((PVOID*)&RenderSkyboxes, RenderSkyboxes_Hook);
			DetourAttach((PVOID*)&SetGlobalWind, SetGlobalWind_Hook);
			HookCall(0x00487508, (DWORD)OnLoadArea_AddStaticPlc);
			DetourAttach((PVOID*)&DestroyGameObjectArray, DestroyGameObjectArray_Hook);
			DetourAttach((PVOID*)&SetObjectScale, SetObjectScale_Hook);
			HookCall(0x004BFC93, (DWORD)OnAIUpdate_GetAnimBase); EnableWrite(0x004BFC98);
			DetourAttach((PVOID*)&GetAttachedModel, GetAttachedModel_Hook);
			DetourAttach((PVOID*)&ArturoTextureController_Control, ArturoTextureController_Control_Hook);
			DetourAttach((PVOID*)&CAurBehaviorAttach_Control, CAurBehaviorAttach_Control_Hook);
			DetourAttach((PVOID*)&BindTexture, BindTexture_Hook);
			DetourAttach((PVOID*)&DestroyPlaceable, DestroyPlaceable_Hook);
			DetourAttach((PVOID*)&DestroyInGameGuiChatwindow, DestroyInGameGuiChatwindow_Hook);
			DetourAttach((PVOID*)&DestroyStringGob, DestroyStringGob_Hook);
			DetourAttach((PVOID*)&DestroyAurTexture, DestroyAurTexture_Hook);
			DetourAttach((PVOID*)&DestroyGob, DestroyGob_Hook);
			DetourAttach((PVOID*)&DoGobShadows, DoGobShadows_Hook);
			DetourAttach((PVOID*)&HandleSwitchWeapon, HandleSwitchWeapon_Hook);
			DetourAttach((PVOID*)&ProcessInput, ProcessInput_Hook);
			DetourAttach((PVOID*)&AurResFree, AurResFree_Hook);
			DetourAttach((PVOID*)&MaxTree_Delete, MaxTree_Delete_Hook);
			DetourAttach((PVOID*)&DestroyNWTileData, DestroyNWTileData_Hook);
			DetourAttach((PVOID*)&DestroyPartTrimesh, DestroyPartTrimesh_Hook);
			DetourAttach((PVOID*)&AddTextLineToEditScroll, AddTextLineToEditScroll_Hook);
			DetourAttach((PVOID*)&DestroyAurAttach, DestroyAurAttach_Hook);
			DetourAttach((PVOID*)&HandleServerToPlayerMessage, HandleServerToPlayerMessage_Hook);
			DetourAttach((PVOID*)&UpdateFountainEmitter, UpdateFountainEmitter_Hook);
			DetourAttach((PVOID*)&AnimateGob, AnimateGob_Hook);
			DetourAttach((PVOID*)&Unload2da, Unload2da_Hook);
			DetourAttach((PVOID*)&UpdateAoEPosition, UpdateAoEPosition_Hook);//*/
			
			HookCall(0x00475810, (DWORD)GetDWKData_Hook);
			HookCall(0x00475F9B, (DWORD)GetDWKData_Hook);
			HookCall(0x00486EF1, (DWORD)GetDWKData_Hook);
			HookCall(0x0048E58E, (DWORD)GetDWKData_Hook);
			HookCall(0x0048E59D, (DWORD)GetDWKData_Hook);
			HookCall(0x0048EA27, (DWORD)GetDWKData_Hook);
			HookCall(0x0048EA4D, (DWORD)GetDWKData_Hook);
			HookCall(0x0048EC5F, (DWORD)GetDWKData_Hook);
			HookCall(0x0048EDF6, (DWORD)GetDWKData_Hook);
			HookCall(0x0050FA84, (DWORD)GetDWKData_Hook);
			HookCall(0x00562A40, (DWORD)GetDWKData_Hook);
			HookCall(0x00583847, (DWORD)GetDWKData_Hook);
			HookCall(0x005A048F, (DWORD)GetDWKData_Hook);
			HookCall(0x005CEFD9, (DWORD)GetDWKData_Hook);

			HookCall(0x004514C9, (DWORD)ReadDWORD_BaseItem);

			//local module after using sinfar hooks
			DetourAttach((PVOID*)&LoadNWNModule, LoadNWNModule_Hook);

			//weapon scaling
			DetourAttach((PVOID*)&anim_wield_org, anim_wield_hook);
			HookCall(0x007DFE80, (DWORD)ftol_notzero);

			//tail scaling
			HookCall(0x00540A13, (DWORD)create_tail_hook);
			HookCall(0x005426C2, (DWORD)create_tail_hook);
			HookCall(0x00542947, (DWORD)create_tail_hook);
			EnableWrite(0x0044941A);
			EnableWrite(0x00541C77);
			//wings scaling
			HookCall(0x00540B83, (DWORD)create_wings_hook);
			HookCall(0x005426A0, (DWORD)create_wings_hook);
			HookCall(0x0054292E, (DWORD)create_wings_hook);
			EnableWrite(0x0044942B);
			EnableWrite(0x00542237);

			//visual effects adjustment
			if (is_on_sinfar >= 2)
			{
				HookCall(0x0044ED9B, (DWORD)on_update_vfx_read_vfx);
				HookCall(0x0044A015, (DWORD)on_update_vfx_read_vfx);
				HookCall(0x00444D0B, (DWORD)on_update_vfx_read_vfx);
				DetourAttach((PVOID*)&delete_visual_effect_to_object_org, delete_visual_effect_to_object_hook);
				DetourAttach((PVOID*)&add_visual_effect_to_object_org, add_visual_effect_to_object_hook);
				DetourAttach((PVOID*)&apply_fnf_anim_at_location_org, apply_fnf_anim_at_location_hook);
				DetourAttach((PVOID*)&update_fountain_emitter_org, update_fountain_emitter_hook);
				DetourAttach((PVOID*)&update_explosion_emitter_org, update_explosion_emitter_hook);
				DetourAttach((PVOID*)&update_single_emitter_org, update_single_emitter_hook);
				DetourAttach((PVOID*)&update_lightning_emitter_org, update_lightning_emitter_org);

				HookCall(0x00487391, (DWORD)read_placeable_id);
				HookCall(0x00487441, (DWORD)read_placeable_id);
				DetourAttach((PVOID*)&add_static_placeable_to_area_org, add_static_placeable_to_area_hook);
				DetourAttach((PVOID*)&spawn_tile_org, spawn_tile_hook);
				DetourAttach((PVOID*)&load_area_org, load_area_hook);

				if (file_exists("./sinfar_override"))
				{
					add_resource_directory("HD0:sinfar_override", DYNAMIC_DIRECTORY);
					aurora::CResourcesDirectory::start_watch_thread();
				}

				HookCall(0x005481E3, (long)on_build_player_list_get_player_at_pos);
			}

			//heads adjustment
			DetourAttach((PVOID*)&create_head_org, create_head_hook);

			DetourAttach((PVOID*)&RenderScene, RenderScene_Hook);

			//extra chests
			HookCall(0x005444F4, (DWORD)on_get_armor_model_concat_str_hook);
			HookCall(0x0054430C, (DWORD)on_get_armor_model_concat_str_hook);
			HookCall(0x005447AC, (DWORD)on_get_armor_model_concat_str_hook);
			HookCall(0x00544938, (DWORD)on_get_armor_model_concat_str_hook);
			HookCall(0x00544B56, (DWORD)on_get_armor_model_concat_str_hook);
			HookCall(0x00544F79, (DWORD)on_get_armor_model_concat_str_hook);
			HookCall(0x005450D2, (DWORD)on_get_armor_model_concat_str_hook);
			HookCall(0x005452EA, (DWORD)on_get_armor_model_concat_str_hook);
			HookCall(0x005468DB, (DWORD)on_get_armor_model_concat_str_hook);
			HookCall(0x00546AE4, (DWORD)on_get_armor_model_concat_str_hook);
			DetourAttach((PVOID*)&create_body_parts_org, create_body_parts_hook);

			//plc scaling
			DetourAttach((PVOID*)&destroy_placeable_org, destroy_placeable_hook);

			HookCall(0x0044AE9C, (DWORD)on_update_creature_add_to_dm_party);
			HookCall(0x00528723, (DWORD)on_update_creature_add_to_dm_party);
			HookCall(0x004544B1, (DWORD)on_player_list_change_append_to_msg_buffer);
			HookCall(0x0045497B, (DWORD)on_player_list_change_append_to_msg_buffer);
			HookCall(0x00453DBA, (DWORD)on_player_list_change_read_obj_id);
			HookCall(0x00453E23, (DWORD)on_player_list_change_read_obj_id);

			//detect invalid res files
			DetourAttach((PVOID*)&GetResFileSize, GetResFileSize_Hook);

			//quick exit and save a crash
			HookCall(0x004026BC, (DWORD)UninitializeClientExoApp);

			DetourTransactionCommit();
		}
	}
	else if (is_on_sinfar && !testing)
	{
		exit(0);
	}
	else
	{
		log_printf("%s entering a non Sinfar world\n", get_timestamp());
	}

	return SendDirectMessage(exo_net, edx, p1, send_data, p3, p4, p5);
}

void* (*InitGameApp)(void*, int) = (void* (*)(void*, int))0x00402760;
void* InitGameApp_Hook(void* p1, int p2)
{
	//weapon vfx
	EnableWrite(0x004EA0FB);
	
	//switch weapon : support custom bows
	EnableWrite(0x004BE29F);
	
	//do not update the head (used by both the xchest and head)
	EnableWrite(0x00449409);

	//extra proj type
	next_proj_type = (uint32_t*)0x004F991B;
	EnableWrite((DWORD)next_proj_type);

	//update cloak appr
	EnableWrite((DWORD)0x00542D33);
	EnableWrite((DWORD)0x00542DF2);

	//beams
	EnableWrite(0x00574127);

	//bored anims
	EnableWrite((DWORD)skip_innactive_anim_jb);

	return InitGameApp(p1, p2);
}

int(__stdcall *win_main_org)(uint32_t, uint32_t, char*, int) = (int(__stdcall *)(uint32_t, uint32_t, char*, int))0x00401EF0;
int __stdcall win_main_hook(uint32_t hInstance, uint32_t hPrevInstance, char* lpCmdLine, int nCmdShow)
{
	__try
	{
		return win_main_org(hInstance, hPrevInstance, lpCmdLine, nCmdShow);
	}
	__except (CrashLog(GetExceptionInformation(), GetExceptionCode(), "main")){
		exit(1);
	}
}

int ini_load_all_plugins = false;
int ini_disable_custom_resman = false;
void load_extra_ini_settings(CIniFileA& ini)
{
	const char* INI_KEY_LOAD_ALL_NWNCX_PLUGINS = "LoadAllNWNCXPlugins";
	const char* INI_KEY_DISABLE_CUSTOM_RESMAN = "DisableCustomResourceManager";
	CIniSectionA* ini_section = ini.GetSection(INI_SECTION);
	if (ini_section)
	{
		ini_load_all_plugins = atoi(ini_section->GetKeyValue(INI_KEY_LOAD_ALL_NWNCX_PLUGINS, "0").c_str());
		ini_disable_custom_resman = atoi(ini_section->GetKeyValue(INI_KEY_DISABLE_CUSTOM_RESMAN, "0").c_str());	
	}
	ini.SetKeyValue(INI_SECTION, INI_KEY_LOAD_ALL_NWNCX_PLUGINS, std::to_string(int64_t(ini_load_all_plugins)).c_str());
	ini.SetKeyValue(INI_SECTION, INI_KEY_DISABLE_CUSTOM_RESMAN, std::to_string(int64_t(ini_disable_custom_resman)).c_str());
}

INLINE void DoHook()
{
	DWORD log_size = FileSize(log_filename);
	if (log_size > 1024*1024)
	{
		_unlink(log_filename);
		log_printf("log file was too large (%ul)\n", log_size);
	}
	log_printf("%s running nwncx_sinfar v%d\n", get_timestamp(), NWNCX_SINFAR_VERSION);

	DetourTransactionBegin();
	DetourUpdateThread(GetCurrentThread());

	nwncx::sinfar::print_stacktrace = PrintStackTrace;

	nwncx_sinfar_init(load_extra_ini_settings);

	/*///////////////////////// TEST ////////////////////////////////////

	testing = true;

	DetourAttach((PVOID*)&HandleWMCharMessage_Org, HandleWMCharMessage_Hook);

	MessageBoxA(0, "testing 5", "", 0);

	///////////////////////////////////////////////////////////////*/

	HookCall(0x00401FB3, (DWORD)InitGameApp_Hook);

	HANDLE file_map_handle = CreateFileMappingA(INVALID_HANDLE_VALUE, NULL, PAGE_EXECUTE_READWRITE, 0, sizeof(SINFARX_FILE_MAP), "SINFARX");	
	file_map = (SINFARX_FILE_MAP*)MapViewOfFile(file_map_handle, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(SINFARX_FILE_MAP));
	if (file_map && file_map->sinfarx_version)
	{
		DetourAttach((PVOID*)&load_item_visual_effect_org, load_item_visual_effect_hook);
		if (file_map->sinfarx_version < 370)
		{
			MessageBoxA(0, "There's an upate for sinfarx available. Download it at http://nwn.sinfar.net/files/sinfarx.exe", "sinfarx.exe update", MB_ICONWARNING);
		}

		//custom resman
		if (!ini_disable_custom_resman)
		{
			DetourAttach((PVOID*)&add_encapsulated_resource_file_org, add_encapsulated_resource_file_hook);
			DetourAttach((PVOID*)&add_resource_image_file_org, add_resource_image_file_hook);
			DetourAttach((PVOID*)&add_fixed_key_table_file_org, add_fixed_key_table_file_hook);
			DetourAttach((PVOID*)&add_resource_directory_org, add_resource_directory_hook);
			DetourAttach((PVOID*)&remove_encapsulated_resource_file_org, remove_encapsulated_resource_file_hook);
			DetourAttach((PVOID*)&remove_resource_image_file_org, remove_resource_image_file_hook);
			DetourAttach((PVOID*)&remove_fixed_key_table_file_org, remove_fixed_key_table_file_hook);
			DetourAttach((PVOID*)&remove_resource_directory_org, remove_resource_directory_hook);
			DetourAttach((PVOID*)&update_resource_directory_org, update_resource_directory_hook);
			DetourAttach((PVOID*)&destroy_res_org, destroy_res_hook);
			DetourAttach((PVOID*)&get_table_count_org, get_table_count_hook);
			DetourAttach((PVOID*)&get_res_of_type_org, get_res_of_type_hook);
			DetourAttach((PVOID*)&resource_exists_org, resource_exists_hook);
			DetourAttach((PVOID*)&release_res_org, release_res_hook);
			DetourAttach((PVOID*)&release_res_object_org, release_res_object_hook);
			HookCall(0x005D0AB8, (long)on_release_res_before_destroying_it);
			HookCall(0x005D0F24, (long)on_release_res_before_destroying_it);
			DetourAttach((PVOID*)&free_res_org, free_res_hook);
			DetourAttach((PVOID*)&free_res_data_org, free_res_data_hook);
			DetourAttach((PVOID*)&free_chunk_org, free_chunk_hook);
			DetourAttach((PVOID*)&request_res_org, request_res_hook);
			DetourAttach((PVOID*)&cancel_request_res_org, cancel_request_res_hook);
			DetourAttach((PVOID*)&demand_res_org, demand_res_hook);
			DetourAttach((PVOID*)&get_res_object_org, get_res_object_hook);
			DetourAttach((PVOID*)&set_res_object_org, set_res_object_hook);
			DetourAttach((PVOID*)&resman_update_org, resman_update_hook);
		}

		//camera hack
		if (ini_enable_camera_hack)
		{
			EnableWrite(0x004A8F34);
			*(float*)(0x004A8F34+6) = ini_cam_min_zoom;
			*(float*)(0x004A8F3E+6) = ini_cam_max_zoom;
			*(float*)(0x004A8F48+6) = ini_cam_min_pitch;
			*(float*)(0x004A8F52+6) = ini_cam_max_pitch;
			EnableWrite(0x004A93E7);
			*(float*)(0x004A93E7+6) = ini_cam_min_zoom;
			*(float*)(0x004A93F1+6) = ini_cam_max_zoom;
			*(float*)(0x004A93FB+6) = ini_cam_min_pitch;
			*(float*)(0x004A9405+6) = ini_cam_max_pitch;
			EnableWrite(0x004A9685);
			*(float*)(0x004A9685+6) = ini_cam_min_zoom;
			*(float*)(0x004A968F+6) = ini_cam_max_zoom;
			*(float*)(0x004A9699+6) = ini_cam_min_pitch;
			*(float*)(0x004A96A3+6) = ini_cam_max_pitch;
			EnableWrite(0x00427211+2);
			*(float**)(0x00427211+2) = &ini_cam_default_min_zoom;
			EnableWrite(0x0042728A+2);
			*(float**)(0x0042728A+2) = &ini_cam_default_max_zoom;
		}

		if (ini_load_all_plugins)
		{
			std::string nwncx_sinfar_plugin_filename = "nwncx_sinfar.dll";
			WIN32_FIND_DATAA wfd;
			HANDLE hFind = FindFirstFileA("./nwncx_*.dll", &wfd);
			if (hFind != INVALID_HANDLE_VALUE) do
			{
				if (nwncx_sinfar_plugin_filename != wfd.cFileName)
				{
					log_printf("Loading plugin:%s\n", wfd.cFileName);
					HINSTANCE extra_plugin_handle = LoadLibraryA((std::string("./") + wfd.cFileName).c_str());
					if (extra_plugin_handle)
					{
						void* getPluginInfo = GetProcAddress(extra_plugin_handle, "GetPluginInfo");
						void* initPlugin = GetProcAddress(extra_plugin_handle, "InitPlugin");
						if (getPluginInfo && initPlugin)	
						{
							PLUGININFO* pi = ((PLUGININFO* (*)(uint32_t))getPluginInfo)(NWNCX_SINFAR_VERSION);
							if (!pi) 
							{
								log_printf("%s: The plugin returned NULL on GetPluginInfo\n", wfd.cFileName);
								continue;
							}
							int init_result = ((int (*)(PLUGINLINK*))initPlugin)(NULL);
							if (init_result != 0) 
							{
								log_printf("%s: The plugin returned %d on InitPlugin\n", wfd.cFileName, init_result);
								continue;
							}
						}
						else
						{
							log_printf("%s: Doesn't implement the plugin interface\n", wfd.cFileName);
						}
					}
				}
			}
			while (FindNextFileA(hFind, &wfd));
		}
		else //nwncx_sinfar alone
		{
			//additional projectile types
			HookCall(0x004F98AD, (DWORD)read_projectile_type_2da_string);
			EnableWrite(0x004F9915);
			*(WORD*)0x004F9915 = 0x9090;

			DetourAttach((PVOID*)&LoadBeam, LoadBeam_Hook);

			DetourAttach((PVOID*)&win_main_org, win_main_hook);

			DetourAttach((PVOID*)&new_server_list_panel_org, new_server_list_panel_hook);
			EnableWrite(0x004DDB88);
			*(uint8_t*)0x004DDB88 = 0xEB;
		}
	}
	else
	{
		NWNCX_SINFAR_VERSION -= 1;
	}

	//custom plt
	DetourAttach((PVOID*)&replace_texture_on_object_org, replace_texture_on_object_hook);
	DetourAttach((PVOID*)&cnwcanimbase_replace_texture_org, cnwcanimbase_replace_texture_hook);
	HookCall(0x004496BA, (long)on_update_creature_appearance_create_item);
	HookCall(0x0044966A, (long)on_update_creature_appearance_get_item_by_id);
	DetourAttach((PVOID*)&create_cloak_org, create_cloak_hook);
	DetourAttach((PVOID*)&replace_texture_on_gob_sub_tree, replace_texture_on_gob_sub_tree_hook);
	DetourAttach((PVOID*)&restore_texture_on_object_org, restore_texture_on_object_hook);
	HookCall(0x005701FC, (DWORD)on_apply_vfx_on_object_get_helmet_id);

	//too many options dialogs bug
	DetourAttach((PVOID*)&handle_server_to_player_dialog_org, handle_server_to_player_dialog_hook);
	DetourAttach((PVOID*)&update_gui_and_render_org, update_gui_and_render_hook);

	//display <c???></c> colors in dialogs
	DetourAttach((PVOID*)&parse_str_org, parse_str_hook);

	HookAPI(GetProcAddress(GetModuleHandleA("KERNEL32.dll"), "HeapFree"), MyHeapFree);
	//HookAPI(GetProcAddress(GetModuleHandleA("KERNEL32.dll"), "HeapAlloc"), MyHeapAlloc);

	//disable master server
	BYTE* patch = (BYTE*)0x004D4AF7;
	EnableWrite((DWORD)patch);
	patch[0] = 0x84;
	patch[1] = 0xF2;
	patch[2] = 0x06;

	HookCall(0x005F67FC, (DWORD)SendPlayerInfo_Hook);

	/*//random port
	EnableWrite(0x004119FD);
	*(WORD*)0x004119FD = 0x9090;
	HookCall(0x00411A03, (DWORD)get_port_int);//*/

	DetourAttach((PVOID*)&find_model_org, FindModel_Hook);

	//custom scaling, z axis, z orientation
	HookCall(0x00448E87, (DWORD)ReadObjUpdateId);
	DetourAttach((PVOID*)&HandleServerToPlayerCreatureUpdate_Appearance, HandleServerToPlayerCreatureUpdate_Appearance_Hook);
	DetourAttach((PVOID*)&set_creature_appearance_info_org, set_creature_appearance_info_hook);
	EnableWrite(0x005435BB);
	DetourAttach((PVOID*)&force_update_creature_appearance_org, force_update_creature_appearance_hook);
	DetourAttach((PVOID*)&set_gob_position_org, set_gob_position_hook);
	//DetourAttach((PVOID*)&MoveCreatureToPosition, MoveCreatureToPosition_Hook);

	DetourAttach((PVOID*)&set_gob_orientation_org, set_gob_orientation_hook);

	DetourAttach((PVOID*)&DestroyClientCreature, DestroyClientCreature_Hook);

	DetourAttach((PVOID*)&AddHighlightGob, AddHighlightGob_Hook);
	DetourAttach((PVOID*)&HiliteCreature, HiliteCreature_Hook);

	DetourAttach((PVOID*)&gob_play_animation_org, gob_play_animation_hook);

	DetourAttach((PVOID*)&creature_walk_update_org, creature_walk_update_hook);

	//custom longbow
	HookCall(0x004BE24F, (DWORD)on_switch_weapon_set_equipped_by);
	DetourAttach((PVOID*)&update_visible_weapons_org, update_visible_weapons_hook);

	HookCall(0x007B0400, (DWORD)alloc_gob_hook);
	DetourAttach((PVOID*)&destroy_caurobject_org, destroy_caurobject_hook);

	DetourAttach((PVOID*)&get_texture_ref_org, get_texture_ref_hook);

	DetourAttach((PVOID*)&creature_ai_update_org, creature_ai_update_hook);

	DetourAttach((PVOID*)&AtiInitializeShaders_Org, AtiInitializeShaders_Hook);
	
	DetourAttach((PVOID*)&destroy_game_object_array_org, destroy_game_object_array_hook);

	DetourAttach((PVOID*)&enable_vertex_program_org, enable_vertex_program_hook);

	//HookAPI(GetProcAddress(GetModuleHandleA("Opengl32.dll"), "glFogf"), glFogf_hook);
	//DetourAttach((PVOID*)&render_gob_org, render_gob_hook);
	//DetourAttach((PVOID*)&draw_gob_org, draw_gob_hook);
	DetourAttach((PVOID*)&read_msg_overflow_org, read_msg_overflow_hook);

	//get player name
	HookCall(0x005F675D, (DWORD)WritePlayerNameCStr);

	//multi-instance
	EnableWrite(0x00401F0E);
	*(BYTE*)0x00401F0E = 0xEB;
	EnableWrite(0x00401F4F);
	*(BYTE*)0x00401F4F = 0xEB;

	//unlock effect icons
	EnableWrite(0x0055476C);
	*(uint8_t*)0x0055476C = 0xFE;

	/*//Do not disconnect on frame timeout
	EnableWrite(0x005F3F1F);
	*(uint8_t*)0x005F3F1F = 0xEB;*/

	//fix the party list
	EnableWrite(0x00526961);
	*(uint8_t*)0x0052695B = 0xC2;
	*(uint32_t*)0x0052695C = 0x90909090;

	//always reset flag when freeing GFF data
	EnableWrite(0x005D0ECA);
	*(uint16_t*)0x005D0ECA = 0x9090;

	DetourTransactionCommit();
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
	if (ul_reason_for_call == DLL_PROCESS_ATTACH)
	{
		//MessageBoxA(0, "attach me", "attach me", 0);

		DoHook();
	}
	return TRUE;
}