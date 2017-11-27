#pragma once

#include "GFFStruct.h"
#include <vector>

class CGFFList
{
public:
	CGFFStruct* AddStruct(CGFFStruct* lpGFFStruct);
	CGFFStruct* GetElementAt(uint32_t n);
	void SetElementAt(uint32_t n, CGFFStruct* lpGFFStruct);
	uint32_t GetStructCount();
	void RemoveElementAt(uint32_t n);
	void RemoveAllElements();
	
	CGFFList(void);
	~CGFFList(void);
	CGFFList* Clone(CGFFList* lpGFFList);

	std::vector<CGFFStruct*> data;
};
