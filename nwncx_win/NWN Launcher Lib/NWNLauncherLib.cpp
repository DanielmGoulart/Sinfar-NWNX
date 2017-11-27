#include "NWNLauncherLib.h"
#include <curl/curl.h>

size_t write_data(void *ptr, size_t size, size_t nmemb, FILE *stream) {
	size_t written;
	written = fwrite(ptr, size, nmemb, stream);
	return written;
}

bool download_file(const char* url, const char* fname) {
	CURL *curl;
	FILE *fp;
	CURLcode res;
	curl = curl_easy_init();
	if (curl) {
		if (fopen_s(&fp, fname, "wb") == 0)
		{
			curl_easy_setopt(curl, CURLOPT_URL, url);
			curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, true);
			curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
			curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
			res = curl_easy_perform(curl);
			curl_easy_cleanup(curl);
			fclose(fp);
			return (res == CURLE_OK);
		}
	}
	return false;
}

void LoadLibraryInProcessSync(HANDLE process, const char* library_path)
{
	size_t library_path_size = strlen(library_path) + 1;
	char* injected_library_path = (char*)VirtualAllocEx(process, NULL, library_path_size, MEM_COMMIT, PAGE_READWRITE);
	WriteProcessMemory(process, injected_library_path, library_path, library_path_size, NULL);
	HANDLE lib_thread = CreateRemoteThread(process, NULL, 0, (LPTHREAD_START_ROUTINE)GetProcAddress(LoadLibraryA("kernel32.dll"), "LoadLibraryA"), injected_library_path, 0, NULL);
	WaitForSingleObject(lib_thread, INFINITE);
	VirtualFreeEx(process, injected_library_path, 0, MEM_RELEASE);
}

bool GetFileExists(const std::string& file_path)
{
	WIN32_FIND_DATAA FindFileData;
	HANDLE handle = FindFirstFileA(file_path.c_str(), &FindFileData);
	if (handle != INVALID_HANDLE_VALUE)
	{
		FindClose(handle);
		return true;
	}
	else
	{
		return false;
	}
}

DWORD GetFileSize(const std::string& file_path)
{
	WIN32_FIND_DATAA FindFileData;
	HANDLE handle = FindFirstFileA(file_path.c_str(), &FindFileData);
	if (handle != INVALID_HANDLE_VALUE)
	{
		FindClose(handle);
		return FindFileData.nFileSizeLow;
	}
	else
	{
		return 0;
	}
}