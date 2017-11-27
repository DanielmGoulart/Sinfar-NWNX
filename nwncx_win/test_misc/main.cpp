#include <Windows.h>
#include <cstdio>
#include <stdint.h>

struct test_destructor
{
	test_destructor(int val) {value=val; printf("created:%d\n", val);}
	~test_destructor(){printf("destructor called:%d!\n", value);}
	bool operator!=(const test_destructor& other){printf("operator == called\n"); return false;}
	int value;
};

int main()
{
	/*const char* armor_parts_code[] = {
		"footr",
		"footl",
		"shinr",
		"shinl",
		"legl",
		"legr",
		"pelvis",
		"chest",
		"belt",
		"neck",
		"forer",
		"forel",
		"bicepr",
		"bicepl",
		"shor",
		"shol",
		"handr",
		"handl",
		"robe"
	};
	for (int i=0; i<19; i++)
	{
		const char* part_name = armor_parts_code[i];
		size_t part_name_len = strlen(part_name);
		part_name += (part_name_len-4);
		printf("0x%x\n", *(int*)part_name);
	}*/
	/*test_destructor t1(1);
	test_destructor t2(2);
	printf("!= %d\n", (t1 != t2));*/

	const char XDATA_CHAR_TO_ASCII_TABLE[] = "abcdefghijklmnopqrstuvwxyz0123456789_-|¦,.:;!@#$%^&*()[]{}+=><~?";
	for (int i=0; i<256; i++)
	{
		uint8_t result = 255;
		int len = strlen(XDATA_CHAR_TO_ASCII_TABLE);
		for (int j=0; j<len; j++)
		{
			if (XDATA_CHAR_TO_ASCII_TABLE[j] == i)
			{
				result = j;
			}
		}
		if (result == 255)
			printf("?");
		else
			printf("\\x%02x", result);
	}

}