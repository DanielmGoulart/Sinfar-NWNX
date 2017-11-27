#include "core.h"
#include "mysql.h"
#include "nwscript.h"

using namespace nwnx::core;
using namespace nwnx::mysql;
using namespace nwnx::nwscript;

namespace nwnx { namespace loot {

VM_FUNC_NEW(GetLoot, 149)
{
	int nLootId = vm_pop_int();
	float fCR = vm_pop_float();
	float fGoldChance = 0;
	bool bInheritGold = true;
	std::vector<std::string> loot_items;
	std::vector<int> inherited_loot;
	std::unordered_set<std::string> individual_items;
	std::unordered_set<int> inventories;
	char sql[200];
	//for the main all all inherited loot table..
	inherited_loot.push_back(nLootId);
	size_t inherited_loot_index = 0;
	do
	{
		nLootId = inherited_loot.at(inherited_loot_index);
		//get all inherited loot
		sprintf(sql, "SELECT child_id FROM loot_inheritances WHERE parent_id=%d ORDER BY weight", nLootId);
		if (mysql_admin->query(sql))
		{
			CMySQLRes result = mysql_admin->store_result();
			MYSQL_ROW row;
			while ((row = result.fetch_row()) != NULL)
			{
				int inherited_loot_id = atoi(row[0]);
				uint32_t i;
				for (i=0; i<inherited_loot.size(); i++)
				{
					if (inherited_loot_id == inherited_loot.at(i)) break;
				}
				if (i == inherited_loot.size())
				{
					inherited_loot.push_back(inherited_loot_id);
				}
			}
		}
		//get the gold if we inherit it
		if (bInheritGold)
		{
			sprintf(sql, "SELECT gold_chance FROM loot_categories WHERE (gold_inherit IS NULL OR gold_inherit=0) AND id=%d", nLootId);
			if (mysql_admin->query(sql))
			{
				CMySQLRes result2 = mysql_admin->store_result();
				MYSQL_ROW row2;
				if ((row2 = result2.fetch_row()) != NULL)
				{
					float fChance = atof(row2[0]);
					if (((float)rand()/(float)RAND_MAX*100.0)<fChance)
					{
						fGoldChance = fChance;
					}
					bInheritGold = false;
				}
			}
		}
		//get individual items from the current table
		sprintf(sql, "SELECT resref,max_count,chance FROM loot_items WHERE category_id=%d AND min_cr<=%f AND max_cr>=%f", nLootId, fCR, fCR);
		if (mysql_admin->query(sql))
		{
			CMySQLRes result2 = mysql_admin->store_result();
			MYSQL_ROW row2;
			while ((row2 = result2.fetch_row()) != NULL)
			{
				char* item_resref = row2[0];
				if (individual_items.count(item_resref)) continue;
				individual_items.insert(item_resref);
				//add this item to the loot
				int nMaxCount = atoi(row2[1]);
				if (nMaxCount == 0 || nMaxCount > 50) nMaxCount = 50;
				float fChance = atof(row2[2]);
				int nCount = 0;
				while (nCount<nMaxCount && ((float)rand()/(float)RAND_MAX*100.0)<fChance)
				{
					loot_items.push_back(item_resref);
					nCount++;
				}
			}
		}
		//get inventories
		sprintf(sql, "SELECT inventory_id,max_count,chance FROM loot_categories_inventories WHERE category_id=%d AND min_cr<=%f AND max_cr>=%f ORDER BY max_cr DESC", nLootId, fCR, fCR);
		if (mysql_admin->query(sql))
		{
			CMySQLRes result2 = mysql_admin->store_result();
			MYSQL_ROW row2;
			while ((row2 = result2.fetch_row()) != NULL)
			{
				int nInventoryId = atoi(row2[0]);
				if (inventories.count(nInventoryId)) continue;
				inventories.insert(nInventoryId);
				//add this inventory to the loot
				int nMaxCount = atoi(row2[1]);
				if (nMaxCount == 0 || nMaxCount > 50) nMaxCount = 50;
				float fChance = atof(row2[2]);
				int nCount = 0;
				if (((float)rand()/(float)RAND_MAX*100.0)<fChance)
				{
					//get the number of items in the inventory
					sprintf(sql, "SELECT COUNT(*) FROM loot_inventories_items WHERE inventory_id=%d", nInventoryId);
					if (mysql_admin->query(sql))
					{
						CMySQLRes result3 = mysql_admin->store_result();
						MYSQL_ROW row3;
						int inventory_size = (((row3 = result3.fetch_row()) != NULL)?atoi(row3[0]):0);
						if (inventory_size > 0)
						{
							do
							{
								//get a random item from the inventory
								int nRandomItem = (rand()%inventory_size);
								sprintf(sql, "SELECT item_resref FROM loot_inventories_items WHERE inventory_id=%d LIMIT %d,1", nInventoryId, nRandomItem);
								if (mysql_admin->query(sql))
								{
									CMySQLRes result3 = mysql_admin->store_result();
									MYSQL_ROW row3;
									if ((row3 = result3.fetch_row()) != NULL)
									{
										loot_items.push_back(row3[0]);
									}
								}
								nCount++;
							}
							while (nCount<nMaxCount && ((float)rand()/(float)RAND_MAX*100.0)<fChance);
						}
					}
				}
			}
		}
	}
	while (++inherited_loot_index < inherited_loot.size());

	CScriptArray result(new CScriptArrayData);
	result->push_back(CScriptVarValue(fGoldChance));
	CScriptArray result_items(new CScriptArrayData);
	for (uint32_t nLootItem=0; nLootItem<loot_items.size(); nLootItem++)
	{
		result_items->push_back(CScriptVarValue(loot_items.at(nLootItem)));
	}
	result->push_back(CScriptVarValue(result_items));
	vm_push_array(&result);
}


}
}