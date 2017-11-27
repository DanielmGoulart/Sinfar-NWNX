#include "CKey.h"

using namespace aurora;

int main()
{
	CKey key("Z:\\Windows8.1\\NWN\\xp3.key", "Z:\\Windows8.1\\NWN");
	CResourcesDirectory res_dir("Z:\\Windows8.1\\NWN\\override", NULL);
	CResourcesContainer* res_container = &res_dir;

	for (auto iter = res_container->resources.begin(); iter != res_container->resources.end(); iter++)
	{
		iter->second->SaveToFile("c:/test/"+iter->first.GetFileName());
	}

	std::system("pause");

	return 0;
}
