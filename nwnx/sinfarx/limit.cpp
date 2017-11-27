#include "core.h"
#include "nwscript.h"

using namespace nwnx::core;
using namespace nwnx::nwscript;

namespace nwnx { namespace limit {

CNWSObject* OnGetCreatureById_ReturnObject(CServerExoApp* server, uint32_t object_id)
{
	CGameObject* o = GetGameObject(object_id);
	if (o)
	{
		return o->vtable->AsNWSObject(o);
	}
	return NULL;
}
int (*VMPopInteger)(CVirtualMachine*, int*) = (int (*)(CVirtualMachine*, int*))0x82629fc;
int last_set_ai_value = 0;
int OnSetAI_VMPopInteger(CVirtualMachine* vm, int* p_i)
{
	int ret = VMPopInteger(vm, p_i);
	last_set_ai_value = *p_i;
	return ret;
}
int (*SetAILevel)(CServerAIMaster*, CNWSObject*, int) = (int (*)(CServerAIMaster*, CNWSObject*, int))0x08096318;
CNWSCreature* OnSetAI_GetCreatureById(CServerExoApp* server, uint32_t object_id)
{
	CGameObject* o = GetGameObject(object_id);
	if (o)
	{
		if (o->type == OBJECT_TYPE_CREATURE)
		{
			return o->vtable->AsNWSCreature(o);
		}
		else
		{
			CNWSObject* object = o->vtable->AsNWSObject(o);
			if (object)
			{
				SetAILevel(server_internal->srv_ai, object, last_set_ai_value);
			}
		}
	}
	return NULL;
}

int GetResourceExists(CExoResMan* resman, CResRef* resref, uint16_t p3, uint32_t* p4)
{
	return true;
}


void init()
{
	//placeables can whisper
	enable_write(0x08068825);
	*(uint8_t*)0x08068825 = 0x18;
	enable_write(0x0806883C);
	*(uint8_t*)0x0806883C = 0x18;
	enable_write(0x08068859);
	*(uint8_t*)0x08068859 = 0x18;
    
    //on get AI, support any object type
	hook_call(0x08235e06, (uint32_t)OnGetCreatureById_ReturnObject);
	//on set AI, add support from non-creature objects
	hook_call(0x0822AE95, (uint32_t)OnSetAI_VMPopInteger);
	hook_call(0x0822AEBA, (uint32_t)OnSetAI_GetCreatureById);
    
    //copy and mod any appearance
	enable_write(0x082202d8);
	*(uint32_t*)(0x082202d8) = ((uint32_t)GetResourceExists-(uint32_t)0x082202dc);
	enable_write(0x08220320);
	*(uint32_t*)(0x08220320) = ((uint32_t)GetResourceExists-(uint32_t)0x08220324);
	enable_write(0x0822055d);
	*(uint32_t*)(0x0822055d) = ((uint32_t)GetResourceExists-(uint32_t)0x08220561);
	enable_write(0x0822057a);
	*(uint32_t*)(0x0822057a) = ((uint32_t)GetResourceExists-(uint32_t)0x0822057e);
    
    //set TMI limit
	enable_write(0x825FE05);
    
    //set color, up to 255
	enable_write(0x0823183c);
	*(uint8_t*)0x0823183c = 255;
	enable_write(0x08220687);
	*(uint8_t*)0x08220687 = 255;
    
    //enable all effect duration
	enable_write(0x0820dc50);
	const char* set_effect_duration_asm =
		"\x66\x83\x63\x0a\xf8" //andw $0xfff8,0xa(%ebx) //clear the current duration
		"\x83\xe0\x07" //and $0x7,%eax //validate the new duration
		"\x66\x09\x43\x0a"; //or %ax,0xa(%ebx) //store the new duration
	memcpy((void*)0x0820dc50, set_effect_duration_asm, strlen(set_effect_duration_asm));
	for (int i=strlen(set_effect_duration_asm); i<16; i++)
	{
		*(uint8_t*)(0x0820dc50+i) = 0x90;
	}
    
    //on SetBodyPart, do not scrap the equiped armor, ignore it
	enable_write(0x0822ceee);
	*(uint8_t*)0x0822ceee = 0xEB;
    
    //set the maximum - 1, number of pages, the inventory of a placeable can have
	enable_write(0x081a6b69);
	*(uint8_t*)0x081a6b69 = 9;
}
REGISTER_INIT(init);

VM_FUNC_NEW(SetTMILimit, 26)
{
	int lim = vm_pop_int();
	if (lim < 16383)
	{
        lim = 16383;
	}
    else if (lim > 8388607)
	{
        lim = 8388607;
	}
	*(int*)0x825FE05 = lim;
}

 
}
}