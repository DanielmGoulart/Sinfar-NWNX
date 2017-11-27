#include "GFFList.h"

CGFFList* CGFFList::Clone(CGFFList* lpGFFList)
{
	std::vector<CGFFStruct*>::iterator iter;
	for (iter=data.begin(); iter!=data.end(); ++iter)
	{
		lpGFFList->data.push_back((*iter)->Clone(new CGFFStruct()));
	}

	return lpGFFList;
}

CGFFList::CGFFList(void)
{
}

CGFFList::~CGFFList(void)
{
	std::vector<CGFFStruct*>::iterator iter;
	for(iter=data.begin(); iter!=data.end(); ++iter) 
	{
		delete *iter;
	}
}

uint32_t CGFFList::GetStructCount()
{
	return data.size();
}

CGFFStruct* CGFFList::AddStruct(CGFFStruct* lpGFFStruct)
{
	CGFFStruct* lpNewGFFStruct = lpGFFStruct->Clone(new CGFFStruct);
	data.push_back(lpNewGFFStruct);
	return lpNewGFFStruct;
}

CGFFStruct* CGFFList::GetElementAt(uint32_t n)
{
	return data[n];
}

void CGFFList::RemoveElementAt(uint32_t n)
{
	data.erase(data.begin()+n);
}

void CGFFList::RemoveAllElements()
{
	for (uint32_t n=0; n<data.size(); n++)
	{
		delete data[n];
	}
	data.clear();
}

void CGFFList::SetElementAt(uint32_t n, CGFFStruct* lpGFFStruct)
{
	if (n >= data.size()) return;

	delete data[n];
	data[n] = lpGFFStruct->Clone(new CGFFStruct);
}