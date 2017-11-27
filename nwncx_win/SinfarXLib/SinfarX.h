#pragma once

#include <Windows.h>
#include "../Sinfar NWN Launcher/sinfarx_file_map.h"
#include <cstdio>

namespace sinfarx
{
	extern SINFARX_FILE_MAP* file_map;

	extern bool perform_test;
	extern DWORD launch_tickcount;
	extern DWORD(__stdcall *MyGetTickCount)(void);

	void EnableWrite(DWORD addr);
	void HookCall(DWORD src_addr, DWORD to_addr);
	LPVOID HookAPI(LPVOID lpOldProc, LPVOID lpNewProc);

	bool FileExists(const char* fileName);
	DWORD FileSize(const char* filename);
	char* get_timestamp();
}
