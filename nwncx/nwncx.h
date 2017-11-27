#pragma once

#include <stdint.h>
#include <ctype.h>
#include <cstdio>
#include <cstring>
#include <set>
#include <vector>
#include <string>
#ifdef WIN32
#include <boost/unordered_map.hpp>
#define unordered_map boost::unordered_map
#elif __linux__
#include <unordered_map>
#define unordered_map std::unordered_map
#endif

namespace nwncx
{

#ifdef WIN32
#define NWNCX_MEMBER_CALL __fastcall
#define NWNCX_MEMBER_X_PARAM int,
#define NWNCX_MEMBER_X_PARAM_VAL 0,
#define NWNCX_DESTRUCTOR_P2
#define NWNCX_DESTRUCTOR_P2_VAL
#define NWNCX_DESTRUCTOR_P2_VAL_3
#define snprintf sprintf_s
#elif __linux__
#define NWNCX_MEMBER_CALL
#define NWNCX_MEMBER_X_PARAM
#define NWNCX_MEMBER_X_PARAM_VAL
#define NWNCX_DESTRUCTOR_P2 ,int p2
#define NWNCX_DESTRUCTOR_P2_VAL ,p2
#define NWNCX_DESTRUCTOR_P2_VAL_3 ,3
#define __cdecl
#endif

#ifdef __linux__
#include "../nwnx/common/include/NWNXLib.h"
#elif WIN32
	struct CExoString;
	extern CExoString* (__fastcall *CExoStringSetText)(CExoString*, int, const char*);
	extern void (__fastcall *DestroyCExoString)(CExoString*);
	struct CExoString 
	{
		char* text;
		uint32_t len;
		CExoString() : text(NULL), len(0) {}
		CExoString(const CExoString& other) : text(NULL), len(0)
		{
			CExoStringSetText(this, 0, other.text); 
		}
		CExoString(const char* str) : text(NULL), len(0)
		{
			CExoStringSetText(this, 0, str); 
		}
		void operator=(const CExoString& assign)
		{
			CExoStringSetText(this, 0, assign.text); 
		}
		void operator=(const char* assign)
		{
			CExoStringSetText(this, 0, assign); 
		}
		~CExoString()
		{
			DestroyCExoString(this);
		}
	};
#include <nwnx_include/types.h>
#include <nwnx_include/nwn_const.h>
#endif

#define OBJECT_INVALID 0x7F000000
	
	struct CGuiInGame;
#ifdef WIN32
	struct CClientExoAppInternal
	{
		char unk_0x0[0x14];
		CGameObjectArray* go_array; //0x14
		uint32_t unk_0x18;
		CExoLinkedList player_list; //0x1C
		char unk_0x20[0x24];
		CGuiInGame* in_game_gui; //0x44
		char unk_0x48[0x124];
		void* message; //0x16C
	};
#elif __linux__
	struct CClientExoAppInternal
	{
		char unk_0x0[0x18];
		CGameObjectArray* go_array; //0x18
		CExoLinkedList player_list; //0x1C
		char unk_0x20[0x28]; //0x28
		CGuiInGame* in_game_gui; //0x48
		char unk_0x4C[0x120];
		void* message; //0x16C
	};
#endif
	struct CClientExoApp
	{
		uint32_t unk_0x0;
		CClientExoAppInternal* app_internal; //0x4;
	};
	
	struct CNWCMessage
	{
		void* unknown;
	};

	struct Quaternion {
		float x;
		float y;
		float z;
		float w;
	};

	template<typename T> struct List
	{
		T* data; //0x4
		uint32_t size; //0x0
		uint32_t alloc_size; //0x8
	};

	struct CAurPart;
	struct Gob
	{
		char unk_0x0[0x15];
		char enable_animations; //0x15
		char unk_0x16[0xE];
		char name[0x40]; //0x24
		bool can_scale; //0x64
		uint32_t unk_0x68;
		uint32_t unk_0x6C;
		uint32_t unk_0x70;
		uint32_t unk_0x74;
		CAurPart* root_part; //0x78
		uint32_t unk_0x7C;
		uint32_t unk_0x80;
		Vector position; //0x84;
		char unk_0x90[0x118];
		float scale1; //0x1A8
		float scale2; //0x1AC
		List<Gob*> children; //0x1B0;
		uint32_t unk_0x1BC;
	};

	struct CAurBehavior
	{
		char unk_0x0[0x3C];
		Vector position_offset; //0x3C
	};

	struct CAurTexture
	{
		char unk_0x0[0x7c];
		CResRef resref; //0x7c
		char  unk_0x4[0x38]; //0x8c
	};

	struct CAuroraTexture
	{
		int unk_0x0;
		int unk_0x4;
		CRes* res; //0x8;
	};

	enum NwnPalette
	{
		NwnPalette_Skin = 0,
		NwnPalette_Hair = 1,
		NwnPalette_Metal1 = 2,
		NwnPalette_Metal2 = 3,
		NwnPalette_Cloth1 = 4,
		NwnPalette_Cloth2 = 5,
		NwnPalette_Leather1 = 6,
		NwnPalette_Leather2 = 7,
		NwnPalette_Tattoo1 = 8,
		NwnPalette_Tattoo2 = 9,
	};
	
	struct PLT_Header
	{
		char achSig[4];
		char achVersion[4];
		unsigned int ulUnknown1;
		unsigned int ulUnknown2;
		unsigned int ulWidth;
		unsigned int ulHeight;
	};

	struct PLT_Pixel
	{
		unsigned char ucIndex;
		unsigned char ucPalette;
	};

	struct CResPLT
	{
		char unk_0x0[0x40];
		PLT_Pixel* data; //0x40
		PLT_Header* header; //0x44
	};

	struct CLayeredTexture
	{
		char  unk_0x0[0x8];
		CResPLT* res; //0x8
		char unk_0xc[0x14];
	};

	struct MdlNodeTriMesh_PostProcess
	{
		List<float> verts;
		List<float> tverts;
		List<float> tverts1;
		List<float> tverts2;
		List<float> tverts3;
		List<float> colors;
		List<float> faces;
		List<float> unk_vector7;
		List<float> unk_vertex;
		List<int> unk_int;
	};

	struct NewController
	{
		uint16_t float_start_index;
		uint16_t float_end_index;
		bool unk_0x4; // == 1
		int type;
	};

	struct MdlNode
	{
		int unk_0x0;
		int unk_0x4;
		int unk_0x8;
		int unk_0xc;
		int unk_0x10;
		int unk_0x14;
		int unk_0x18;
		int unk_0x1c;
		char name[0x24]; //0x20
		void* material; //0x44
		int unk_0x48;
		int unk_0x4c;
		int unk_0x50;
		List<NewController> cotrollers; //0x54
		List<float> floats; //0x60
		uint32_t type; //0x6c
		uint32_t MdlNodeTriMeshGenVertices; //0x70
		uint32_t MdlNodeTriMeshRemoveTemporaryArrays; //0x74
		List<float> faces; //0x78
		char unk_0x84[0x168];
		List<float> vertex_indices; //0x1EC
		List<float> leftovers; //0x1F8
		List<float> vertex_indices_count; //0x204
		List<float> vertex_flags; //0x210
		int unk_0x21c;
		int unk_0x220;
		int aurora_primitive_types; //0x224
		MdlNodeTriMesh_PostProcess* post_process; //0x228
		int pool_id; //0x22c
	};

	struct MdlNodeTriMesh : public MdlNode
	{
	};

	struct CAurPart;
	struct VertexPrimitive
	{
		int vertex_array_id;
		int unk_0x4;
		MdlNode* node; //0x8
		CAurPart* part; //0xc
		int unk_0xc; //def = 0
		int unk_0x10; //def = 1;
		uint8_t unk_0x14; //def = 0;
		uint8_t unk_0x15;
		uint16_t unk_0x16;
		void* vtable; //0x18
	};


	struct CAurPart
	{
		void* vtable;
		MdlNode* mdl_node; //0x4
		Vector position; //0x8
		Quaternion orientation; //0x14
		float scale; //0x24
		float unk_0x28;
		float unk_0x2C;
		List<CAurPart*> children; //0x30
		CAurPart* parent; //0x3c
		Gob* gob; //0x40;
		void* material; //0x44
		char unk_0x48[0x28];
		VertexPrimitive* vertex_primitive; //0x70
	};

	struct Model
	{
		char unk_0x0[0x48]; //0x0
		void* node; //0x48
		void* unk_0x4c;
		List<MdlNode*> nodes; //0x50
		List<long> list_longs; //0x5c
		char unk_0x68[0x10];
		List<float> animations; //0x78
	};

	struct Face
	{
		float f1;
		float f2;
		float f3;
		float f4;
		float f5;
		float f6;
		float f7;
		float f8;
	};

	struct CGuiInGameChatDialog
	{
		char unk_0x0;
	};

#ifdef WIN32
	struct CGuiInGame
	{
		char unk_0x0[0x40];
		CGuiInGameChatDialog* chat_dialog;
	};
#elif __linux__
	struct CGuiInGame
	{
		char unk_0x0[0x44];
		CGuiInGameChatDialog* chat_dialog;
	};
#endif

	struct CNWCAnimBase
	{
		char unk_0x0[0xB8];
		Gob* gob; //0xB8
	};

	struct CNWCVisualEffectOnObject
	{
		char unk_0x0[0x70];
		Gob* ground_gob; //0x70
		Gob* impact_gob; //0x74
		Gob* head_gob; //0x78
		char unk_0x7c[0x48];
		uint16_t vfx; //0xc4
	};

#ifdef WIN32
	struct CNWCObject
	{
		void* vtable;
		uint32_t obj_id; //0x4
		uint8_t obj_type; //0x8
		uint8_t unk_0x9;
		uint8_t unk_0xa;
		uint8_t unk_0xb;
		char unk_0xc[0x5c];
		CNWCAnimBase* anim_base; //0x68
		CExoLinkedListHeader* vfx_list;
		char unk_0x70[0xA4];
		CResRef resref;//0x114;
	};
	struct CNWCItem;
	struct CNWCItem_vtable
	{
		void(__fastcall *Destructor)(CNWCItem*, int edx, int p2);
		char unk_0x4[0x78];
		void(__fastcall *LoadModel)(CNWCObject*, int, CResRef, uint8_t, uint8_t);
	};
	struct CNWCItem  
	{
		CNWCItem_vtable* vtable;
		uint32_t unk_0x4;
		uint32_t unk_0x8;
		uint16_t it_baseitem; //0xC
		uint16_t unk_0xa;
		CNWCObject object; //0x10
	};
#elif __linux__
	struct CNWCObject;
	struct CNWCObject_vtable
	{
		char unk_0x0[0x8];
		void(*Destructor)(CNWCObject*, int p2);
	};
	struct CNWCObject
	{
		uint32_t unk_0x0;
		uint32_t obj_id; //0x4
		uint8_t obj_type;
		uint8_t unk_0x9;
		uint8_t unk_0xa;
		uint8_t unk_0xb;
		CNWCObject_vtable* vtable; //0xc
		char unk_0x10[0x5c];
		CNWCAnimBase* anim_base; //0x6c
		CExoLinkedListHeader* vfx_list;
	};
	struct CNWCItem  {
		uint32_t unk_0x0;
		uint32_t unk_0x4;
		uint16_t it_baseitem; //0x8
		uint16_t unk_0xa;
		uint32_t unk_0xc;
		CNWCObject object; //0x10
	};
#endif

	struct CExoResFile
	{
		uint32_t unk_0x0;
		CExoString file_name;
	};

	struct CNWCPlaceable
	{
		CNWCObject plc_obj;
	};

	struct CNWCPlayer
	{
		char unk_0x0[0x24];
#ifdef WIN32
		uint32_t unk_0x24;
#endif
		uint32_t player_pc_oid;
		uint32_t player_oid;
	};

	struct CNWCCreatureAppearance;
#ifdef WIN32
	struct CNWCCreature 
	{
		uint32_t unk_0x0;
		uint32_t cre_id; //4
		uint32_t unk_0x8;
		uint32_t unk_0xC;
		uint32_t unk_0x10;
		uint32_t unk_0x14;
		uint32_t unk_0x18;
		uint32_t unk_0x1C;
		uint32_t unk_0x20;
		uint32_t unk_0x24;
		uint32_t unk_0x28;
		Vector position; //0x2C
		uint32_t unk_0x38;
		uint32_t unk_0x3C;
		uint32_t unk_0x40;
		uint32_t unk_0x44;
		uint32_t unk_0x48;
		uint32_t unk_0x4C;
		uint16_t animation; //0x50;
		uint8_t unk_0x52[0x186];
		Vector orientation; //0x1D8
		char unk_0x1e4[0x40];
		CNWCCreatureAppearance* appearance; //0x224
	};
#elif __linux__
	struct CNWCCreature 
	{
		uint32_t unk_0x0;
		uint32_t cre_id; //4
		uint32_t unk_0x8;
		uint32_t unk_0xC;
		uint32_t unk_0x10;
		uint32_t unk_0x14;
		uint32_t unk_0x18;
		uint32_t unk_0x1C;
		uint32_t unk_0x20;
		uint32_t unk_0x24;
		uint32_t unk_0x28;
		Vector position; //0x2C
		uint32_t unk_0x38;
		uint32_t unk_0x3C;
		uint32_t unk_0x40;
		uint32_t unk_0x44;
		uint32_t unk_0x48;
		uint32_t unk_0x4C;
		uint16_t animation; //0x50;
		uint8_t unk_0x52[0x186];
		Vector orientation; //0x1D8
		char unk_0x1e4[0x44];
		CNWCCreatureAppearance* appearance; //0x228
	};
#endif

	struct CNWCCreatureAppearance 
	{
		char unk_0x0[0x41];
		uint8_t skin_color; //0x41
		uint8_t hair_color; //0x42
		uint8_t tattoo1_color; //0x43
		uint8_t tattoo2_color; //0x44
		char unk_0x45[0x27];
		CNWCCreature* creature; //0x6C;
	};

	struct CCreatureAppearanceInfo
	{
		char unk_0x0[0x34];
		uint32_t helmet_id; //0x34
		int has_armor; //0x38
		uint32_t unk_0x3C;
		uint8_t phenotype; //0x40
		uint8_t unk_0x41;
		uint16_t unk_0x42;
		uint32_t unk_0x44;
		uint32_t unk_0x48;
		uint32_t unk_0x4C;
		float perspace; //0x50
		float height; //0x54
		uint32_t unk_0x58;
		uint32_t unk_0x5C;
		uint32_t unk_0x60;
	};
	
#ifdef WIN32
	struct Scheduler
	{
		char unk_0x0[0xF4];
		void* two_dimensions_arrays;
	};
#endif
	
	int wildcmp(const char *wild, const char *string);
	char* strtoupper(char* string);
	Vector quaternion_to_vector(Quaternion q);
	Quaternion vector_to_quaternion(Vector v);
	
	extern CClientExoApp*** p_client_exo_app;
	extern CNWRules** p_rules;
	extern CExoBase** p_exo_base;
	extern uint32_t* paletteheight;
#ifdef WIN32
	extern Scheduler** p_scheduler;
#endif

	CNWCCreature* creature_by_id(uint32_t cre_id);
	CNWCPlaceable* placeable_by_id(uint32_t plc_id);
	float read_msg_float(CNWMessage* msg);
	uint8_t read_msg_byte(CNWMessage* msg);
	uint16_t read_msg_word(CNWMessage* msg);
	uint32_t read_msg_dword(CNWMessage* msg);
	uint32_t read_msg_obj_id(CNWMessage* msg);
	
	void update_creature_animation(CNWCCreature* creature);
	bool is_stationary_animation(CNWCCreature* creature, uint16_t anim);
	
	int get_gob_part_position(Gob*, const char*, Vector*, Quaternion*);
	int set_gob_part_position(Gob*, const char*, Vector, Quaternion);

	Gob* find_gob(Gob* gob, const char* name);
	CAurPart* find_part(CAurPart* part, const char* name);
	void print_gobs(Gob* gob, void (*log_printf)(const char* format, ...), std::string spacer="    ");
	void print_parts(CAurPart* part, void (*log_printf)(const char* format, ...), std::string spacer="->");
	
	bool set_gob_scale(Gob* gob, float scale);
	
	Gob* get_creature_gob(CNWCCreature* creature);
	
	C2DA* get_cached_2da(const char* table_name);
	
	int get_2da_string(C2DA* table, int row, CExoString* col_name, CExoString* value);
	
	extern CNWCItem* (*get_item_by_id)(uint32_t);
	
	int set_item_equipped_by(CNWCItem*, CNWCCreature*);
	
	extern CNWCCreature* (NWNCX_MEMBER_CALL *get_item_equipped_by)(CNWCItem*);

	extern uint32_t (NWNCX_MEMBER_CALL *get_item_in_slot)(CNWCCreature*, NWNCX_MEMBER_X_PARAM uint32_t);

	extern Gob* (__cdecl *new_aur_object)(char*, char*, void*);

	extern CNWBaseItem* (NWNCX_MEMBER_CALL *get_base_item)(void*, NWNCX_MEMBER_X_PARAM int);

	extern void* (NWNCX_MEMBER_CALL *get_game_objects_array)(CClientExoApp*);

	extern void* (NWNCX_MEMBER_CALL *delete_game_object_id)(void*, NWNCX_MEMBER_X_PARAM uint32_t);

	extern void* (__cdecl *nwn_alloc)(uint32_t size); 
	extern void (__cdecl *nwn_free)(void*);
}