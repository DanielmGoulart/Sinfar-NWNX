#include "craft_pp_cost.h"
#include "core.h"
#include "nwscript.h"
#include "cached2da.h"

using namespace nwnx::core;
using namespace nwnx::nwscript;
using namespace nwnx::cached2da;

namespace nwnx
{
namespace cost
{

inline void LoadEffectToItemProperty(CGameEffect* effect, CNWItemProperty* ip)
{
	ip->ip_type = *(effect->eff_integers+0);
	unsigned char param_table = *(effect->eff_integers+4);
	ip->ip_subtype = (param_table<255?*(effect->eff_integers+5):*(effect->eff_integers+1));
	ip->ip_cost_value = *(effect->eff_integers+3);
	//ip->ip_cost_table = *(effect->eff_integers+2);
	//ip->ip_param_table = *(effect->eff_integers+4);
	//ip->ip_param_value = *(effect->eff_integers+5);
	//ip->ip_chance = *(effect->eff_integers+7);
}

int GetItemPropertyPP(CNWItemProperty* ip)
{
	float fCost = 0.0;
	char sCost[10];
	C2da* f2daProps = get_cached_2da("cost_props");
	if (f2daProps == NULL) return 0;
	char sTableName[17];
	C2da* f2daThisProp = NULL;
	char sColsType[2];
	if (f2daProps->GetString("PP_Table", ip->ip_type, sTableName, 17) &&
		(f2daThisProp = get_cached_2da(sTableName)) != NULL &&
		f2daProps->GetString("Cols_Type", ip->ip_type, sColsType, 2))
	{
		int nRow, nCol;
		if (sColsType[0] == '1')
		{
			nCol = ip->ip_subtype;
			nRow = ip->ip_cost_value;
		}
		else
		{
			nCol = ip->ip_cost_value;
			nRow = ip->ip_subtype;
		}

		char sPropertyColsTable[17];
		if (f2daProps->GetString("Table_Cols", ip->ip_type, sPropertyColsTable, 17))
		{
			char sColName[25];
			C2da* f2daPropCols = get_cached_2da(sPropertyColsTable);
			if (f2daPropCols != NULL &&
				f2daPropCols->GetString("Col_Name", nCol, sColName, 25))
			{
				if (f2daThisProp->GetString(sColName, nRow, sCost, 10) ||
					f2daThisProp->GetString(sColName, 0, sCost, 10))
				{
					sscanf(sCost, "%f", &fCost);
					return fCost;
				}
			}
		}

		if (f2daThisProp->GetString("Default", nRow, sCost, 10) ||
			f2daThisProp->GetString("Default", 0, sCost, 10))
		{
			sscanf(sCost, "%f", &fCost);
			return fCost;
		}
	}

	if (f2daProps->GetString("PP", ip->ip_type, sCost, 10))
	{
		sscanf(sCost, "%f", &fCost);
		return fCost;
	}

	return -1;
}

float GetItemPropertyCost(CNWItemProperty* ip)
{
	float fCost = 0.0;
	char sCost[10];
	C2da* f2daProps = get_cached_2da("cost_props");
	if (f2daProps == NULL) return 0;
	char sTableName[17];
	C2da* f2daThisProp = NULL;
	char sColsType[2];
	if (f2daProps->GetString("Cost_Table", ip->ip_type, sTableName, 17) &&
		(f2daThisProp = get_cached_2da(sTableName)) != NULL &&
		f2daProps->GetString("Cols_Type", ip->ip_type, sColsType, 2))
	{
		int nRow, nCol;
		if (sColsType[0] == '1')
		{
			nCol = ip->ip_subtype;
			nRow = ip->ip_cost_value;
		}
		else
		{
			nCol = ip->ip_cost_value;
			nRow = ip->ip_subtype;
		}

		char sPropertyColsTable[17];
		if (f2daProps->GetString("Table_Cols", ip->ip_type, sPropertyColsTable, 17))
		{
			char sColName[25];
			C2da* f2daPropCols = get_cached_2da(sPropertyColsTable);
			if (f2daPropCols != NULL &&
				f2daPropCols->GetString("Col_Name", nCol, sColName, 25))
			{
				if (f2daThisProp->GetString(sColName, nRow, sCost, 10) ||
					f2daThisProp->GetString(sColName, 0, sCost, 10))
				{
					sscanf(sCost, "%f", &fCost);
					return fCost;
				}
			}
		}

		if (f2daThisProp->GetString("Default", nRow, sCost, 10) ||
			f2daThisProp->GetString("Default", 0, sCost, 10))
		{
			sscanf(sCost, "%f", &fCost);
			return fCost;
		}
	}

	if (f2daProps->GetString("Cost", ip->ip_type, sCost, 10))
	{
		sscanf(sCost, "%f", &fCost);
		return fCost;
	}

	return 0;
}

int (*GetItemAC)(CNWSItem*) = (int (*)(CNWSItem*))0x081a2d58;
void CalculateItemCost_HookProc(CNWSItem* item)
{
	double fCost = 0;
	double fItemCost = 0.0;
	double fItemIPMultiplier = 1.0;
	C2da* f2da = get_cached_2da("cost_items");
	char sItemCost[20];
	if (f2da != NULL)
	{
		if (f2da->GetString("Cost", item->it_baseitem, sItemCost, 10) &&
			sscanf(sItemCost, "%lf", &fItemCost))
		{
			fCost = fItemCost;
		}
		else
		{
			char sItemCostTable[17];
			char sCostTableType[2];
			if (f2da->GetString("Cost_Table", item->it_baseitem, sItemCostTable, 17) &&
				f2da->GetString("Table_Type", item->it_baseitem, sCostTableType, 2))
			{
				C2da* fItemCostTable = get_cached_2da(sItemCostTable);
				if (fItemCostTable != NULL)
				{
					if (sCostTableType[0] == '1')//read resref
					{
						int nAC = GetItemAC(item);
						if (fItemCostTable->GetString("Cost", nAC, sItemCost, 10) &&
							sscanf(sItemCost, "%lf", &fItemCost))
						{
							fCost = fItemCost;
						}
					}
				}
			}
		}

		char sfItemIPMultiplier[20];
		if (f2da->GetString("IP_Multiplier", item->it_baseitem, sfItemIPMultiplier, 20))
		{
			sscanf(sfItemIPMultiplier, "%lf", &fItemIPMultiplier);
		}
	}

	CNWSObject* object = &(item->obj);
	for (uint32_t nEffect=0; nEffect<object->obj_effects_len; nEffect++)
	{
		CGameEffect* effect = *(object->obj_effects+nEffect);
		if (effect->eff_type == EFFECT_TRUETYPE_ITEMPROPERTY &&
			effect->eff_dursubtype == DURATION_TYPE_PERMANENT)
		{
			CNWItemProperty ip;
			LoadEffectToItemProperty(effect, &ip);
			fCost += GetItemPropertyCost(&ip) * fItemIPMultiplier;
		}
	}

	item->it_cost_ided = fCost;
}

int GetItemPropertiesPP(CNWSItem* item)
{
	C2da* f2daProps = get_cached_2da("cost_props");
	CNWSObject* object = &(item->obj);
	int nItemPropertiesPP = 0;
	for (uint32_t nEffect=0; nEffect<object->obj_effects_len; nEffect++)
	{
		CGameEffect* effect = *(object->obj_effects+nEffect);
		if (effect->eff_type != EFFECT_TRUETYPE_ITEMPROPERTY ||
			effect->eff_dursubtype != DURATION_TYPE_PERMANENT) continue;

		CNWItemProperty ip;
		LoadEffectToItemProperty(effect, &ip);
		int nItemPropertyPP = GetItemPropertyPP(&ip);
		if (nItemPropertyPP == -1)
		{
			nItemPropertiesPP += 1000;
			continue;
		}
		char stacking_group[12];
		if (f2daProps->GetString("Stacking_Group", ip.ip_type, stacking_group, 12))
		{
			char sPropertyColsTable[17];
			bool no_sub_type = (f2daProps->GetString("Table_Cols", ip.ip_type, sPropertyColsTable, 17)?false:true);

			int nItemPropertyGroupMaxPP = 0;
			uint32_t nItemPropertyGroupMaxPPIndex = 0;
			uint32_t nEffect2;
			for (nEffect2=0; nEffect2<object->obj_effects_len; nEffect2++)
			{
				CGameEffect* effect2 = *(object->obj_effects+nEffect2);
				if (effect2->eff_type != EFFECT_TRUETYPE_ITEMPROPERTY ||
					effect2->eff_dursubtype != DURATION_TYPE_PERMANENT) continue;

				CNWItemProperty ip2;
				LoadEffectToItemProperty(effect2, &ip2);
				char stacking_group2[12];
				if (f2daProps->GetString("Stacking_Group", ip2.ip_type, stacking_group2, 12))
				{
					if (strcmp(stacking_group, stacking_group2)==0)
					{
						if (no_sub_type || ip.ip_subtype == ip2.ip_subtype)
						{
							int nItemProperty2PP = GetItemPropertyPP(&ip2);
							if (nItemProperty2PP > nItemPropertyGroupMaxPP)
							{
								nItemPropertyGroupMaxPP = nItemProperty2PP;
								nItemPropertyGroupMaxPPIndex = nEffect2;
							}
						}
					}
				}
			}
			if (nEffect == nItemPropertyGroupMaxPPIndex) nItemPropertiesPP += nItemPropertyPP;
		}
		else
		{
			nItemPropertiesPP += nItemPropertyPP;
		}
	}
	return nItemPropertiesPP;
}

std::string GetItemPropertyMaterial(CNWItemProperty* ip)
{
	C2da* f2daProps = get_cached_2da("cost_props");
	if (f2daProps == NULL) return "";
	char sTableName[17];
	C2da* f2daThisProp;
	char sColsType[2];
	if (f2daProps->GetString("Cost_Table", ip->ip_type, sTableName, 17) &&
		(f2daThisProp = get_cached_2da(sTableName)) != NULL &&
		f2daProps->GetString("Cols_Type", ip->ip_type, sColsType, 2))
	{
		int nCol;
		if (sColsType[0] == '1')
		{
			nCol = ip->ip_subtype;
		}
		else
		{
			nCol = ip->ip_cost_value;
		}

		char sPropertyColsTable[17];
		if (f2daProps->GetString("Table_Cols", ip->ip_type, sPropertyColsTable, 17))
		{
			C2da* f2daPropCols = get_cached_2da(sPropertyColsTable);
			if (f2daPropCols)
			{
				std::string map_for_subtype = f2daPropCols->GetString("Material_ResRef", nCol);
				if (!map_for_subtype.empty())
				{
					return map_for_subtype;
				}
			}
		}
	}

	return f2daProps->GetString("Material_ResRef", ip->ip_type);
}

void init()
{
	hook_function(0x081a5794, (unsigned long)CalculateItemCost_HookProc, d_ret_code_nouse, 12);
    
    //fix for saving throw properties: read from the melee cost table instead of the bonus cost table
	enable_write(0x081b04fd);
	*(uint8_t*)(0x081b04fd) = 2;
}
REGISTER_INIT(init);

VM_FUNC_NEW(GetItemPropertyCostByValues, 47)
{
	CNWItemProperty ip;
	ip.ip_type = vm_pop_int();
	ip.ip_subtype = vm_pop_int();
	ip.ip_cost_value = vm_pop_int();
	vm_push_float(GetItemPropertyCost(&ip));
}
VM_FUNC_NEW(GetItemPropertyPPByValues, 48)
{
	CNWItemProperty ip;
	ip.ip_type = vm_pop_int();
	ip.ip_subtype = vm_pop_int();
	ip.ip_cost_value = vm_pop_int();
	vm_push_int(GetItemPropertyPP(&ip));
}
VM_FUNC_NEW(GetItemPropertiesPP, 49)
{
	int result = 0;
	uint32_t item_id = vm_pop_object();
	CNWSItem* item = GetItemById(item_id);
	if (item)
	{
		result = GetItemPropertiesPP(item);
	}
	vm_push_int(result);
}
VM_FUNC_NEW(GetItemPropertyMaterialByValues, 50)
{
	CNWItemProperty ip;
	ip.ip_type = vm_pop_int();
	ip.ip_subtype = vm_pop_int();
	ip.ip_cost_value = vm_pop_int();
	CExoString result(GetItemPropertyMaterial(&ip).c_str());
	vm_push_string(&result);
}

}
}
