
#ifndef _NX_NWN_STRUCT_CRES_
#define _NX_NWN_STRUCT_CRES_

#pragma pack(push, 1)

struct CRes;

struct CRes_vtable
{
	void(__fastcall *Destructor)(CRes*, int edx, int p2); //0x8
	void* unk_0x4;
	void* unk_0x8;
	void(__fastcall *OnResourceFreed)(CRes*); //0xC
	int(__fastcall *OnResourceServiced)(CRes*); //0x10
};

struct CRes {
	CRes_vtable* vtable;
	unsigned short demands; //0x4
	unsigned short requests; //0x6
	unsigned long id; //0x8
	long unk4; //0xc
	char* data; //0x10
	void* entry_info; //0x14
	unsigned long size; //0x18
	int owner_counter; //0x1c
	char* has_low_buffer; //0x20
	char* has_high_buffer; //0x24
	void* list_element;	//0x28
};

struct CResNCS
{
	CRes Res;
	int loaded2; //0x2C
	int size2; //0x30
	void* data2; //0x34
};

#pragma pack(pop)

#endif /* _NX_NWN_STRUCT_CRES_ */

/* vim: set sw=4: */
