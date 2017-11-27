#include "SinfarX.h"
#include <imagehlp.h>
#pragma comment(lib, "imagehlp")

namespace sinfarx
{
	SINFARX_FILE_MAP* file_map = NULL;

	bool perform_test = false;
	DWORD launch_tickcount = (DWORD)-1;
	DWORD(__stdcall *MyGetTickCount)(void) = (DWORD(__stdcall *)(void))GetTickCount;

	void EnableWrite(DWORD addr)
	{
		DWORD old_protect;
		VirtualProtect((PVOID)addr, 1024, PAGE_EXECUTE_READWRITE, &old_protect);
	}
	void HookCall(DWORD src_addr, DWORD to_addr)
	{
		EnableWrite(src_addr);
		*(DWORD*)(src_addr + 1) = to_addr - (src_addr + 5);
	}
	LPVOID HookAPI(LPVOID lpOldProc, LPVOID lpNewProc)
	{
		PVOID pImageBase = GetModuleHandle(NULL);
		DWORD ulSize;
		PIMAGE_IMPORT_DESCRIPTOR pImportDescriptor = (PIMAGE_IMPORT_DESCRIPTOR)ImageDirectoryEntryToData(pImageBase, TRUE, IMAGE_DIRECTORY_ENTRY_IMPORT, &ulSize);
		while (pImportDescriptor->Characteristics != 0)
		{
			PIMAGE_THUNK_DATA pThunkData = (PIMAGE_THUNK_DATA)((DWORD)pImageBase + pImportDescriptor->FirstThunk);
			while (pThunkData->u1.Function != NULL)
			{
				if ((LPVOID)pThunkData->u1.Function == lpOldProc)
				{
					DWORD dwOldProtect, dwNewProtect;
					if (VirtualProtect(&(pThunkData->u1.Function), 4, PAGE_EXECUTE_READWRITE, &dwOldProtect))
					{
						pThunkData->u1.Function = (DWORD)lpNewProc;
						VirtualProtect(&(pThunkData->u1.Function), 4, dwOldProtect, &dwNewProtect);
					}
				}
				pThunkData++;
			}
			pImportDescriptor++;
		}
		return lpOldProc;
	}

	bool FileExists(const char* fileName)
	{
		return (GetFileAttributesA(fileName) != 0xFFFFFFFF);
	}
	DWORD FileSize(const char* filename)
	{
		WIN32_FIND_DATAA FindFileData;
		HANDLE handle = FindFirstFileA(filename, &FindFileData);
		if (handle != INVALID_HANDLE_VALUE)
		{
			FindClose(handle);
			return FindFileData.nFileSizeLow;
		}
		else
		{
			return false;
		}
	}

	char timestamp[30];
	char* get_timestamp()
	{
		SYSTEMTIME st;
		GetSystemTime(&st);
		sprintf_s(timestamp, 30, "[%d/%02d/%02d %02d:%02d:%02d.%03d]", st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);
		return timestamp;
	}
}
