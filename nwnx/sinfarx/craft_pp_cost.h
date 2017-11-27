#include "core.h"

namespace nwnx
{
namespace cost
{

inline void LoadEffectToItemProperty(CGameEffect* effect, CNWItemProperty* ip);
int GetItemPropertyPP(CNWItemProperty* ip);
float GetItemPropertyCost(CNWItemProperty* ip);
int GetItemPropertiesPP(CNWSItem* item);
std::string GetItemPropertyMaterial(CNWItemProperty* ip);

}
}
