#include "GFFExoLocString.h"
#include <cstring>

const char* CGFFExoLocString::GetString(int nLangId)
{
	return data[nLangId];
}

void CGFFExoLocString::SetString(int nLangId, const char* string)
{
	char* newString = new char[strlen(string)+1];
	strcpy(newString, string);

	char* currentString = data[nLangId];
	if (currentString != NULL)
	{
		delete[] currentString;
	}
	data[nLangId] = newString;
}

CGFFExoLocString::STRINGS_ITERATOR CGFFExoLocString::GetLangsIterator()
{
	return data.begin();
}

int CGFFExoLocString::GetLangAtPos(STRINGS_ITERATOR& iterator)
{
	if (iterator == data.end()) return -1;
	return iterator->first;	
}

int CGFFExoLocString::GetNextLang(STRINGS_ITERATOR& iterator)
{
	if (iterator == data.end()) return -1;
	iterator++;
	if (iterator == data.end()) return -1;
	return iterator->first;	
}

int CGFFExoLocString::GetStrRef()
{
	return (int)nStrRef;
}

void CGFFExoLocString::SetStrRef(int strref)
{
	nStrRef = (uint32_t)strref;
}

CGFFExoLocString* CGFFExoLocString::Clone(CGFFExoLocString* lpGFFExoLocString)
{
	lpGFFExoLocString->nStrRef = nStrRef;
	char* string;
	std::map<int, char*>::iterator iter;
	for (iter=data.begin(); iter!=data.end(); ++iter)
	{
		string = new char[strlen(iter->second)+1];
		strcpy(string, iter->second);
		lpGFFExoLocString->data[iter->first] = string;
	}

	return lpGFFExoLocString;
}

CGFFExoLocString::CGFFExoLocString(void)
{
	nStrRef = (uint32_t)-1;
}

void CGFFExoLocString::RemoveAllString()
{
	std::map<int, char*>::iterator iter;
	for(iter=data.begin(); iter!=data.end(); ++iter)
	{
		delete[] iter->second;
	}
	data.clear();
}

CGFFExoLocString::~CGFFExoLocString(void)
{
	std::map<int, char*>::iterator iter;
	for(iter=data.begin(); iter!=data.end(); ++iter)
	{
		delete[] iter->second;
	}
}
