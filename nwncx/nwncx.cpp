#include "nwncx.h"
#include <math.h>

#ifdef WIN32
#include <Windows.h>
#endif

namespace nwncx
{

int wildcmp(const char *wild, const char *string)
{
	const char *cp = NULL, *mp = NULL;
	while ((*string) && (*wild != '*')) {
		if ((*wild != *string) && (*wild != '?')) {
			return 0;
		}
		wild++;
		string++;
	}
	while (*string) {
		if (*wild == '*') {
			if (!*++wild) {
				return 1;
			}
			mp = wild;
			cp = string+1;
		} else if ((*wild == *string) || (*wild == '?')) {
			wild++;
			string++;
		} else {
			wild = mp;
			string = cp++;
		}
	}
	while (*wild == '*') {
		wild++;
	}
	return !*wild;
}

char* strtoupper(char* string)
{
	char* temp = string;
	if (temp == NULL) return NULL;
	do
	{
		*temp = toupper(*temp);
	}
	while (*++temp);
	return string;
}

Vector quaternion_to_vector(Quaternion q)
{
	Vector v;
	v.x = atan2f(2*((q.w * q.x) + (q.y * q.z)), 1 - (2 * ((q.x* q.x) + (q.y * q.y))));
	v.y = asinf(2 * ((q.w * q.y) - (q.z * q.x)));
	v.z = atan2f(2 * ((q.w * q.z) + (q.x * q.y)), 1 - (2 * ((q.y * q.y) + (q.z * q.z))));
	return v;
}

Quaternion vector_to_quaternion(Vector v)
{
	Quaternion q;
	float c1 = cosf(v.z / 2);
	float c2 = cosf(v.y / 2);
	float c3 = cosf(v.x / 2);
	float s1 = sinf(v.z / 2);
	float s2 = sinf(v.y / 2);
	float s3 = sinf(v.x / 2);
	q.x = c1*c2*s3 - s1*s2*c3;
	q.y = c1*s2*c3 + s1*c2*s3;
	q.z = s1*c2*c3 - c1*s2*s3;
	q.w = c1*c2*c3 + s1*s2*s3;
	return q;
}

#ifdef WIN32
CExoString* (__fastcall *CExoStringSetText)(CExoString*, int, const char*) = (CExoString* (__fastcall *)(CExoString*, int, const char*))0x005BA520;
void (__fastcall *DestroyCExoString)(CExoString*) = (void (__fastcall *)(CExoString*))0x005BA420;
#endif

//global variables
#ifdef WIN32
CClientExoApp*** p_client_exo_app = (CClientExoApp***)0x92DC50;
CNWRules** p_rules = (CNWRules**)0x92DC64;
Scheduler** p_scheduler = (Scheduler**)0x92DC64;
CExoBase** p_exo_base = (CExoBase**)0x0092DC34;
uint32_t* paletteheight = (uint32_t*)0x0093271C;
#elif __linux__
CClientExoApp*** p_client_exo_app = (CClientExoApp***)0x862C354;
CNWRules** p_rules = (CNWRules**)0x0862C360;
CExoBase** p_exo_base = (CExoBase**)0x0862C334;
uint32_t* paletteheight = (uint32_t*)0x0863F1D8;
#endif

CNWCCreature* creature_by_id(uint32_t cre_id)
{
#ifdef WIN32
	return ((CNWCCreature* (__fastcall *)(CClientExoApp*, int, uint32_t))(0x004078D0))(**p_client_exo_app, 0, cre_id);
#elif __linux__
	return ((CNWCCreature* (*)(CClientExoApp*, uint32_t))(0x8076B64))(**p_client_exo_app, cre_id);;
#endif
}

CNWCPlaceable* placeable_by_id(uint32_t plc_id)
{
#ifdef WIN32
	return ((CNWCPlaceable* (__fastcall *)(CClientExoApp*, int, uint32_t))(0x004078F0))(**p_client_exo_app, 0, plc_id);
#elif __linux__
	return ((CNWCPlaceable* (*)(CClientExoApp*, uint32_t))(0x08113C20))(**p_client_exo_app, plc_id);
#endif
}

float read_msg_float(CNWMessage* msg)
{
#ifdef WIN32
	return ((float (__fastcall *)(CNWMessage*, int, float, int))(0x004FB840))(msg, 0, 1.0, 0x20);
#elif __linux__
	return ((float (*)(CNWMessage*, float, int))(0x081E1DBC))(msg, 1.0, 0x20);
#endif
}

uint8_t read_msg_byte(CNWMessage* msg)
{
#ifdef WIN32
	return ((uint8_t (__fastcall *)(CNWMessage*, int, int))(0x004FB4D0))(msg, 0, 0x8);
#elif __linux__
	return ((uint8_t (*)(CNWMessage*, int))(0x081E1B48))(msg, 0x8);
#endif
}

uint16_t read_msg_word(CNWMessage* msg)
{
#ifdef WIN32
	return ((uint16_t (__fastcall *)(CNWMessage*, int, int))(0x004FB5F0))(msg, 0, 0x10);
#elif __linux__
	return ((uint16_t (*)(CNWMessage*, int))(0x081E1BEC))(msg, 0x10);
#endif
}

uint32_t read_msg_dword(CNWMessage* msg)
{
#ifdef WIN32
	return ((uint32_t (__fastcall *)(CNWMessage*, int, int))(0x004FB720))(msg, 0, 0x20);
#elif __linux__
	return ((uint32_t (*)(CNWMessage*, int))(0x81E1C8C))(msg, 0x20);
#endif
}

uint32_t read_msg_obj_id(CNWMessage* msg)
{
#ifdef WIN32
	return ((uint32_t (__fastcall *)(CNWMessage*))(0x0053E690))(msg);
#elif __linux__
	return ((uint32_t (*)(CNWMessage*))(0x081A28CC))(msg);
#endif
}

void update_creature_animation(CNWCCreature* creature)
{
#ifdef WIN32
	return ((void (__fastcall *)(CNWCCreature*))(0x004C0360))(creature);
#elif __linux__
	return ((void (*)(CNWCCreature*))(0x81238C4))(creature);
#endif
}

bool is_stationary_animation(CNWCCreature* creature, uint16_t anim)
{
#ifdef WIN32
	return ((bool (__fastcall *)(CNWCCreature*, int, uint16_t))(0x00490640))(creature, 0, anim);
#elif __linux__
	return ((bool (*)(CNWCCreature*, uint16_t))(0x081B07C8))(creature, anim);
#endif
}

int get_gob_part_position(Gob* gob, const char* name, Vector* position, Quaternion* orientation)
{
#ifdef WIN32
	return ((int (__fastcall *)(Gob*, int, const char*, Vector*, Quaternion*))0x007B09D0)(gob, 0, name, position, orientation);
#elif __linux__
	return ((int (*)(Gob*, const char*, Vector*, Quaternion*))0x084E8830)(gob, name, position, orientation);
#endif
}
int set_gob_part_position(Gob* gob, const char* name, Vector position, Quaternion orientation)
{
#ifdef WIN32
	return ((int (__fastcall *)(Gob*, int, const char*, Vector, Quaternion))0x007B0970)(gob, 0, name, position, orientation);
#elif __linux__
	return ((int (*)(Gob*, const char*, Vector, Quaternion))0x084E87DC)(gob, name, position, orientation);
#endif
}

Gob* find_gob(Gob* gob, const char* name)
{
	Gob* result = NULL;
	if (gob)
	{
		if (wildcmp(name, gob->name))
		{
			result = gob;
		}
		else
		{
			for (uint32_t child_index=0; child_index<gob->children.size && result == NULL; child_index++)
			{
				if (gob->children.data[child_index] != NULL)
				{
					result = find_gob(gob->children.data[child_index], name);
				}
			}
		}
	}
	return result;
}

CAurPart* find_part(CAurPart* part, const char* name)
{
	CAurPart* result = NULL;
	if (part)
	{
		if (part->mdl_node && part->mdl_node->name && wildcmp(name, part->mdl_node->name))
		{
			result = part;
		}
		else
		{
			for (uint32_t child_index=0; child_index<part->children.size && result == NULL; child_index++)
			{
				if (part->children.data[child_index] != NULL)
				{
					result = find_part(part->children.data[child_index], name);
				}
			}
		}
	}
	return result;
}

void print_gobs(Gob* gob, void (*log_printf)(const char* format, ...), std::string spacer)
{
	log_printf("gob:%s\n", gob->name);
	for (size_t i=0; i<gob->children.size; i++)
	{
		print_gobs(gob->children.data[i], log_printf, spacer+"    ");
	}
}

void print_parts(CAurPart* part, void (*log_printf)(const char* format, ...), std::string spacer)
{
	log_printf("part:%s\n", part->mdl_node->name);
	for (size_t i=0; i<part->children.size; i++)
	{
		print_parts(part->children.data[i], log_printf, spacer+"->");
	}
}

bool set_gob_scale(Gob* gob, float scale)
{
#ifdef WIN32
	return ((bool (__fastcall *)(Gob*, int, float, bool))0x007B0FE0)(gob, 0, scale, true);
#elif __linux__
	return ((bool (*)(Gob*, float, bool))0x084E8900)(gob, scale, true);
#endif
}

Gob* get_creature_gob(CNWCCreature* creature)
{
#ifdef WIN32
	return ((Gob* (__fastcall *)(CNWCCreature*, int, uint8_t, int))0x004CE830)(creature, 0, 0xFF, 0);
#elif __linux__
	return ((Gob* (*)(CNWCCreature*, uint8_t, int))0x0813824C)(creature, 0xFF, 0);
#endif
}

C2DA* get_cached_2da(const char* table_name)
{
#ifdef WIN32
	return ((C2DA* (__fastcall *)(void*, int, CExoString, int))0x0056A9B0)((*p_scheduler)->two_dimensions_arrays, 0, CExoString(table_name), 1);
#elif __linux__
	return ((C2DA* (*)(CTwoDimArrays*, CExoString, int))0x082261E8)((*p_rules)->ru_2das, CExoString(table_name), 1);
#endif
}

int get_2da_string(C2DA* table, int row, CExoString* col_name, CExoString* value)
{
#ifdef WIN32
	return ((int (__fastcall *)(void*, int, int, CExoString*, CExoString*))0x005CE980)(table, 0, row, col_name, value);
#elif __linux__
	return ((int (*)(C2DA*, int, CExoString*, CExoString*))0x08594814)(table, row, col_name, value);
#endif
}

CNWCItem* (*get_item_by_id)(uint32_t) = (CNWCItem* (*)(uint32_t))
#ifdef WIN32
0x004E9760;
#elif __linux__
0x0819BE80;
#endif

int set_item_equipped_by(CNWCItem* item, CNWCCreature* creature)
{
#ifdef WIN32
	return ((int (__fastcall *)(CNWCItem*, int, void*))0x004E9B20)(item, 0, creature);
#elif __linux__
	return ((int (*)(CNWCItem*, void*))0x0819BF10)(item, creature);
#endif
}

CNWCCreature* (NWNCX_MEMBER_CALL *get_item_equipped_by)(CNWCItem*) = (CNWCCreature* (NWNCX_MEMBER_CALL *)(CNWCItem*))
#ifdef WIN32
0x004E9B50;
#elif __linux__
0x0819BF34;
#endif

uint32_t(NWNCX_MEMBER_CALL *get_item_in_slot)(CNWCCreature*, NWNCX_MEMBER_X_PARAM uint32_t) =
(uint32_t(NWNCX_MEMBER_CALL *)(CNWCCreature*, NWNCX_MEMBER_X_PARAM uint32_t))
#ifdef WIN32
0x004C7E40;
#elif __linux__
0x0812CDE8;
#endif

Gob* (__cdecl *new_aur_object)(char*, char*, void*) = (Gob* (__cdecl *)(char*, char*, void*))
#ifdef WIN32
0x007B02C0;
#elif __linux__
0x084E4A1C;
#endif

CNWBaseItem* (NWNCX_MEMBER_CALL *get_base_item)(void*, NWNCX_MEMBER_X_PARAM int) = (CNWBaseItem* (NWNCX_MEMBER_CALL *)(void*, NWNCX_MEMBER_X_PARAM int))
#ifdef WIN32
0x4BC890;
#elif __linux__
0x081E0040;
#endif

void* (NWNCX_MEMBER_CALL *get_game_objects_array)(CClientExoApp*) = (void* (NWNCX_MEMBER_CALL *)(CClientExoApp*))
#ifdef WIN32
0x407890;
#elif __linux__
0x08076AF4;
#endif

void* (NWNCX_MEMBER_CALL *delete_game_object_id)(void*, NWNCX_MEMBER_X_PARAM uint32_t) = (void* (NWNCX_MEMBER_CALL *)(void*, NWNCX_MEMBER_X_PARAM uint32_t)) 
#ifdef WIN32
0x0042E670;
#elif __linux__
0x080A8AD0;
#endif

void (__cdecl *nwn_free)(void*) = (void (__cdecl *)(void*))
#ifdef WIN32
0x0084AC11;
#elif __linux__
free;
#endif

void* (__cdecl *nwn_alloc)(uint32_t size) = (void* (__cdecl *)(uint32_t))
#ifdef WIN32
0x0084AC1C;
#elif __linux__
malloc;
#endif

}