#include "CKey.h"
#include <algorithm>
#include <string> 

using namespace aurora;

int main(int argc, char *argv[])
{
	if (argc < 4)
	{
		fprintf(stderr, "Too few parameters:<key file> <data dir> <out dir>\n");
		return 1;
	}

	CKey key(argv[1], argv[2]);

	for (auto iter=key.resources.begin(); iter!=key.resources.end(); iter++)
	{
		std::string filename = iter->first.GetFileName();
		std::transform(filename.begin(), filename.end(), filename.begin(), ::tolower);
		iter->second->SaveToFile(std::string()+argv[3]+"/"+filename);
	}

	return 0;
}
