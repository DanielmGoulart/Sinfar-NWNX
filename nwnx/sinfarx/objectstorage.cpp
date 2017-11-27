
#include "objectstorage.h"
#include "core.h"
#include <math.h>
#include <memory>

using namespace nwnx::core;

namespace nwnx
{
namespace odbc
{

//ServerExoApp
CGameObject* (*CServerExoApp__GetGameObject)(CServerExoApp* pServerExoApp, uint32_t nObjID) = (CGameObject* (*)(CServerExoApp*,uint32_t)) 0x080B1D98;
CNWSArea *(*CServerExoApp__GetAreaByGameObjectID)(CServerExoApp* pServerExoApp, uint32_t nObjID) = (CNWSArea *(*)(CServerExoApp*,uint32_t)) 0x080B1E10;

//Vector
Vector (*normalize)(Vector const &) = (Vector (*)(Vector const &)) 0x0826B144;

//CResGFF
void (*CResGFF__CResGFF)(CResGFF*) = (void (*)(CResGFF*)) 0x082B8370;
void (*CResGFF___CResGFF)(CResGFF*, int n) = (void (*)(CResGFF*,int)) 0x082B85E0;
int (*CResGFF__CreateGFFFile)(CResGFF*, CResStruct*,CExoString const &sType,CExoString const &sVersion) = (int (*)(CResGFF*,CResStruct*,const CExoString&,const CExoString&)) 0x082BAAC4;
int (*CResGFF__WriteGFFToPointer)(CResGFF*, void **ppBuffer, int&nLength) = (int (*)(CResGFF*,void**,int&)) 0x082BAE78;
int (*CResGFF__GetDataFromPointer)(CResGFF*, void *data, int size) = (int (*)(CResGFF*,void*,int)) 0x082B961C;
void (*CResGFF__GetGFFFileInfo)(CResGFF*, CExoString *sType, CExoString *sVersion) = (void (*)(CResGFF*,CExoString*,CExoString*)) 0x082BB70C;
int (*CResGFF__GetTopLevelStruct)(CResGFF*, CResStruct*) = (int (*)(CResGFF*,CResStruct*)) 0x082BBA5C;

//CNWSPlaceable
void (*CNWSPlaceable__CNWSPlaceable)(CNWSPlaceable*, uint32_t a1) = (void (*)(CNWSPlaceable*,uint32_t)) 0x081DB8F0;
int (*CNWSPlaceable__SavePlaceable)(CNWSPlaceable*, CResGFF*, CResStruct*) = (int (*)(CNWSPlaceable*,CResGFF *,CResStruct*)) 0x081DE414;
int (*CNWSPlaceable__LoadPlaceable)(CNWSPlaceable*, CResGFF*, CResStruct*, CExoString *sTag) = (int (*)(CNWSPlaceable*,CResGFF*,CResStruct*,CExoString*)) 0x081DC150;
int (*CNWSPlaceable__AddToArea)(CNWSPlaceable*, CNWSArea *area, float x, float y, float z, int a) = (int (*)(CNWSPlaceable*,CNWSArea*,float,float,float,int)) 0x081DF174;
void (*CNWSPlaceable__SetOrientation)(CNWSPlaceable*, Vector v) = (void (*)(CNWSPlaceable*,Vector)) 0x081DF084;
int (*CNWSPlaceable__AcquireItem)(CNWSPlaceable*, CNWSItem**,uint32_t,uint32_t,uint8_t,int) = (int (*)(CNWSPlaceable*, CNWSItem**,uint32_t,uint32_t,uint8_t,int))0x81E0860;

//CNWSStore
void (*CNWSStore__CNWSStore)(CNWSStore*, uint32_t a1) = (void (*)(CNWSStore*,uint32_t)) 0x08083C0C;
int (*CNWSStore__SaveStore)(CNWSStore*, CResGFF*, CResStruct*) = (int (*)(CNWSStore*,CResGFF *,CResStruct*)) 0x08084DB8;
int (*CNWSStore__LoadStore)(CNWSStore*, CResGFF*, CResStruct*, CExoString *sTag) = (int (*)(CNWSStore*,CResGFF*,CResStruct*,CExoString*)) 0x08084230;
int (*CNWSStore__AddToArea)(CNWSStore*, CNWSArea *area, float x, float y, float z, int a) = (int (*)(CNWSStore*,CNWSArea*,float,float,float,int)) 0x08085B0C;
int (*CNWSStore__AcquireItem)(CNWSStore*, CNWSItem*,int,uint8_t,uint8_t) = (int (*)(CNWSStore*, CNWSItem*,int,uint8_t,uint8_t))0x8085404;

//CNWSTrigger
void (*CNWSTrigger__CNWSTrigger)(CNWSTrigger *object, uint32_t a1) = (void (*)(CNWSTrigger*,uint32_t)) 0x081EE180;
int (*CNWSTrigger__SaveTrigger)(CNWSTrigger *object, CResGFF*, CResStruct*) = (int (*)(CNWSTrigger*,CResGFF *,CResStruct*)) 0x081F1610;
int (*CNWSTrigger__LoadTrigger)(CNWSTrigger *object, CResGFF*, CResStruct*, CExoString *sTag) = (int (*)(CNWSTrigger*,CResGFF*,CResStruct*,CExoString*)) 0x081F0138;
int (*CNWSTrigger__AddToArea)(CNWSTrigger *object, CNWSArea *area, float x, float y, float z, int a) = (int (*)(CNWSTrigger*,CNWSArea*,float,float,float,int)) 0x081EE668;

//CNWSCreature
void (*CNWSCreature__CNWSCreature)(CNWSCreature*, uint32_t, int, int) = (void (*)(CNWSCreature*, uint32_t, int, int))0x8113D80;
int (*CNWSCreature__LoadCreature)(CNWSCreature*,CResGFF*,CResStruct*,int,int,int,int) = (int (*)(CNWSCreature*,CResGFF*,CResStruct*,int,int,int,int))0x8120218;
int (*CNWSCreature__AddToArea)(CNWSCreature*, CNWSArea*, float, float, float, int) = (int (*)(CNWSCreature*, CNWSArea*, float, float, float, int))0x8118D20;
int (*CNWSCreature__AcquireItem)(CNWSCreature*,CNWSItem**,uint32_t,uint32_t,uint8_t,uint8_t,int,int) = (int (*)(CNWSCreature*,CNWSItem**,uint32_t,uint32_t,uint8_t,uint8_t,int,int))0x80FFE20;
inline int CNWSCreature__SaveCreature(CNWSCreature* creature, CResGFF* gff, CResStruct* gff_struct)
{
	return ((int (*)(CNWSCreature*,CResGFF*,CResStruct*,int,int,int))0x081209fc)(creature, gff, gff_struct, 0, 0, 0);
}

//CNWSItem
void (*CNWSItem__CNWSItem)(CNWSItem*, uint32_t) = (void (*)(CNWSItem*, uint32_t))0x819EDA0;
int (*CNWSItem__LoadItem)(CNWSItem*, CResGFF*, CResStruct*, int) = (int (*)(CNWSItem*, CResGFF*, CResStruct*, int))0x81A17CC;
int (*CNWSItem__AddToArea)(CNWSItem*,CNWSArea*,float,float,float,int) = (int (*)(CNWSItem*,CNWSArea*,float,float,float,int))0x81A65D0;
int (*CNWSItem__AcquireItem)(CNWSItem*, CNWSItem**,uint32_t,uint8_t,uint8_t,int) = (int (*)(CNWSItem*, CNWSItem**,uint32_t,uint8_t,uint8_t,int))0x81A5F4C;
inline int CNWSItem__SaveItem(CNWSItem* item, CResGFF* gff, CResStruct* gff_struct)
{
	return ((int (*)(CNWSItem*,CResGFF*,CResStruct*,int))0x081a19c4)(item, gff, gff_struct, 0);
}

//CNWSObject
void (*CNWSObject__SaveVarTable)(CNWSObject *object, CResGFF*, CResStruct*) = (void (*)(CNWSObject*,CResGFF*,CResStruct*)) 0x081D4BD8;
void (*CNWSObject__LoadVarTable)(CNWSObject *object, CResGFF*, CResStruct*) = (void (*)(CNWSObject*,CResGFF*,CResStruct*)) 0x081D4BC0;
void (*CNWSObject__SetPosition)(CNWSObject *object, Vector vPos, int bSkipArea) = (void (*)(CNWSObject*,Vector,int)) 0x081D4E30;
void (*CNWSObject__SetOrientation)(CNWSObject *object, Vector v) = (void (*)(CNWSObject*,Vector)) 0x081D4E10;

Vector FacingToOrientation(float fFacing)
{
	Vector v;
	v.x = cos(fFacing*M_PI/180);
	v.y = sin(fFacing*M_PI/180);
	v.z = 0;
	return normalize(v);
}

void ApplyInventoryLoadPatch(unsigned long pAddr, bool bEnabled)
{
	unsigned char * pCode = (unsigned char *)pAddr;
	enable_write((unsigned long)pCode);
	if(bEnabled)
	{
		pCode[1] = 0x88;
	}
	else
	{
		pCode[1] = 0x80;
	}
}

class CResGFF_SelfDestroy
{
private:
	CResGFF res_gff;
public:
	CResGFF_SelfDestroy()
	{
		CResGFF__CResGFF(&res_gff);
	}
	~CResGFF_SelfDestroy()
	{
		CResGFF___CResGFF(&res_gff, 2);
	}
	operator CResGFF*(){return &res_gff;}
};

char *SaveObject(uint32_t object_id, int* size)
{
	int (*SaveObjectInternal)(void* object, CResGFF*, CResStruct*);
	const char * sGFFType = NULL;
	*size = 0;
	CGameObject* o = CServerExoApp__GetGameObject((*p_app_manager)->app_server, object_id);
	if(o)
	{
		CNWSObject* object = o->vtable->AsNWSObject(o);
		void* serializable_object = NULL;
		if (object)
		{
			if(o->type == 9) //placeable
			{
				serializable_object = o->vtable->AsNWSPlaceable(o);
				SaveObjectInternal = (int (*)(void*,CResGFF *,CResStruct*)) CNWSPlaceable__SavePlaceable;
				sGFFType = "UTP ";
			}
			else if(o->type == 0xE) //store
			{
				serializable_object = o->vtable->AsNWSStore(o);
				SaveObjectInternal = (int (*)(void*,CResGFF *,CResStruct*)) CNWSStore__SaveStore;
				sGFFType = "UTM ";
			}
			else if(o->type == 0x7) //trigger
			{
				serializable_object = o->vtable->AsNWSTrigger(o);
				SaveObjectInternal = (int (*)(void*,CResGFF *,CResStruct*)) CNWSTrigger__SaveTrigger;
				sGFFType = "UTT ";
			}
			else if(o->type == 0x5) //creature
			{
				serializable_object = o->vtable->AsNWSCreature(o);
				SaveObjectInternal = (int (*)(void*,CResGFF *,CResStruct*)) CNWSCreature__SaveCreature;
				sGFFType = "BIC ";
			}
			else if(o->type == 0x6) //item
			{
				serializable_object = o->vtable->AsNWSItem(o);
				SaveObjectInternal = (int (*)(void*,CResGFF *,CResStruct*)) CNWSItem__SaveItem;
				sGFFType = "UTI ";
			}
		}

		if (serializable_object)
		{
			CExoString sVersion("V2.0");
			CExoString sType(sGFFType);
			CResGFF_SelfDestroy res_gff;
			
			CResStruct res_struct;
			if(CResGFF__CreateGFFFile(res_gff, &res_struct, sType, sVersion)){
				if(SaveObjectInternal(serializable_object, res_gff, &res_struct)){
					CNWSObject__SaveVarTable(object, res_gff, &res_struct);
					void *data;
					int nDataSize;
					if(CResGFF__WriteGFFToPointer(res_gff, &data, nDataSize)){
						*size = nDataSize;
						return (char *)data;
					}
				}
			}
		}
	}
	return NULL;
}

uint32_t LoadObject(void* data, int size, const CScriptLocation& location, uint32_t owner_id)
{
	CExoString sVersion, sType;

	CNWSArea* area = CServerExoApp__GetAreaByGameObjectID((*p_app_manager)->app_server, location.loc_area);
	if (!area) return OBJECT_INVALID;
	
	CResGFF_SelfDestroy res_gff;
	CResGFF__CResGFF(res_gff);
	CResStruct res_struct;
	if(CResGFF__GetDataFromPointer(res_gff, data, size))
	{
		CResGFF__GetGFFFileInfo(res_gff, &sType, &sVersion);
		CResGFF__GetTopLevelStruct(res_gff, &res_struct);
	}
	//new
	//constructor
	//Load*
	//LoadVarTable
	//SetPosition
	//SetOrientation
	//AddToArea
	if(sType == "UTP ")
	{
		CNWSPlaceable* placeable = new CNWSPlaceable;
		CNWSPlaceable__CNWSPlaceable(placeable, 0x7F000000);
		ApplyInventoryLoadPatch(0x081DD49D, true);
		CNWSPlaceable__LoadPlaceable(placeable, res_gff, &res_struct, NULL);
		ApplyInventoryLoadPatch(0x081DD49D, false);
		CNWSObject__LoadVarTable((CNWSObject*)placeable, res_gff, &res_struct);
		CNWSObject__SetPosition((CNWSObject*)placeable, location.loc_position, 0);
		CNWSPlaceable__SetOrientation(placeable, location.loc_orientation);
		CNWSPlaceable__AddToArea(placeable, area, location.loc_position.x, location.loc_position.y, location.loc_position.z, 1);
		return placeable->obj.obj_id;
	}
	else if(sType == "UTM ")
	{
		CNWSStore* store = new CNWSStore;
		CNWSStore__CNWSStore(store, 0x7F000000);
		ApplyInventoryLoadPatch(0x080847FE, true);
		CNWSStore__LoadStore(store, res_gff, &res_struct, NULL);
		ApplyInventoryLoadPatch(0x080847FE, false);
		CNWSObject__LoadVarTable((CNWSObject*)store, res_gff, &res_struct);
		CNWSObject__SetPosition((CNWSObject*)store, location.loc_position, 0);
		CNWSObject__SetOrientation((CNWSObject *)store, location.loc_orientation);
		CNWSStore__AddToArea(store, area, location.loc_position.x, location.loc_position.y, location.loc_position.z, 1);
		return store->obj.obj_id;
	}
	else if(sType == "UTT ")
	{
		CNWSTrigger* trigger = new CNWSTrigger;
		CNWSTrigger__CNWSTrigger(trigger, 0x7F000000);
		CNWSTrigger__LoadTrigger(trigger, res_gff, &res_struct, NULL);
		CNWSObject__LoadVarTable((CNWSObject*)trigger, res_gff, &res_struct);
		CNWSObject__SetPosition((CNWSObject*)trigger, location.loc_position, 0);
		CNWSObject__SetOrientation((CNWSObject *)trigger, location.loc_orientation);
		CNWSTrigger__AddToArea(trigger, area, location.loc_position.x, location.loc_position.y, location.loc_position.z, 1);
		return trigger->obj.obj_id;
	}
	else if (sType == "BIC ")
	{
		CNWSCreature* creature = new CNWSCreature;
		CNWSCreature__CNWSCreature(creature, 0x7F000000, 0, 1);
		CNWSCreature__LoadCreature(creature, res_gff, &res_struct, 0, 0, 0, 0);
		CNWSObject__SetOrientation((CNWSObject*)creature, location.loc_orientation);
		CNWSCreature__AddToArea(creature, area, location.loc_position.x, location.loc_position.y, location.loc_position.z, 1);
		return creature->obj.obj_id;
	}
	else if (sType == "UTI ")
	{
		CNWSItem* item = new CNWSItem;
		CNWSItem__CNWSItem(item, 0x7F000000);
		CNWSItem__LoadItem(item, res_gff, &res_struct, 0);
		CGameObject* o_owner = CServerExoApp__GetGameObject((*p_app_manager)->app_server, owner_id);
		bool item_acquired = false;
		if (o_owner)
		{
			if (o_owner->type == OBJECT_TYPE_CREATURE)
			{
				item_acquired = CNWSCreature__AcquireItem(o_owner->vtable->AsNWSCreature(o_owner), &item, 0x7F000000, 0x7F000000, 0xFF, 0xFF, 1, 1);
			}
			else if (o_owner->type == OBJECT_TYPE_PLACEABLE)
			{
				item_acquired = CNWSPlaceable__AcquireItem(o_owner->vtable->AsNWSPlaceable(o_owner), &item, 0x7F000000, 0xFF, 0xFF, 1);
			}
			else if (o_owner->type == OBJECT_TYPE_STORE)
			{
				item_acquired = CNWSStore__AcquireItem(o_owner->vtable->AsNWSStore(o_owner), item, 0, 0xFF, 0xFF);
			}
			else if (o_owner->type == OBJECT_TYPE_ITEM)
			{
				item_acquired = CNWSItem__AcquireItem(o_owner->vtable->AsNWSItem(o_owner), &item, 0x7F000000, 0xFF, 0xFF, 1);
			}
		}
		if (!item_acquired)
		{
			CNWSObject__SetPosition(&item->obj, location.loc_position, 0);
			CNWSObject__SetOrientation(&item->obj, location.loc_orientation);
			CNWSItem__AddToArea(item, area, location.loc_position.x, location.loc_position.y, location.loc_position.z, 1);
		}
		return item->obj.obj_id;
	}
	return 0x7F000000;
}

}
}

