#include "CERF.h"

using namespace aurora;

int main()
{
	CERF erf("Z:/Windows8.1/NWN/texturepacks/Tiles_Tpb.erf");

	for (auto iter = erf.resources.begin(); iter != erf.resources.end(); iter++)
	{
		iter->second->SaveToFile("c:/test/" + iter->first.GetFileName());
	}

	std::system("pause");

	return 0;
}