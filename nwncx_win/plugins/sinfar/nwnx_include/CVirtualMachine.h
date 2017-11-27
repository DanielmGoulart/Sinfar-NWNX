#ifndef _NX_NWN_STRUCT_CVIRTUALMACHINE_
#define _NX_NWN_STRUCT_CVIRTUALMACHINE_

typedef struct
{
	uint32_t Stack;
	uint32_t StackSize;
	uint32_t InstructionPtr;
	uint32_t SecondaryPtr;
	uint32_t Code;
	uint32_t CodeSize;
	CExoString Name;
	//uint32_t field_20;
} CVirtualMachineScript;

typedef struct
{
	uint32_t  StackPointer;
	uint32_t  field_4;
	uint32_t  AllocatedSize;
	uint8_t  *VarTypes;
	uint32_t *Values;
	uint32_t  field_14;
	uint32_t  CurrentInstruction;
} CVirtualMachineStack;

struct CVirtualMachine_s {
	uint32_t field_0;
	uint32_t field_4;
	uint32_t field_8;
	uint32_t RecursionLevel;
	CVirtualMachineScript Scripts[8];
	uint32_t LevelActive[8];
	uint32_t ObjectID[8];
	CVirtualMachineStack Stack;
	uint32_t field_18C[8];
	uint32_t field_1AC[123];
	void *Commands; 

    void                       *unknown;
    
    int Runscript(CExoString *ScriptName, nwn_objid_t OBJECT_SELF, int a4 = 1);
    int StackPopEngineStructure(int a2, void **a3);
    int StackPopInteger(int *iINT);
    int StackPopObject(nwn_objid_t *oID);
    int StackPopString(CExoString *str);
    int StackPushEngineStructure(int a2, void *a3);
    int StackPushFloat(float f);
    int StackPushInteger(int i);
    int StackPushObject(nwn_objid_t Object);
    
};

#endif 
