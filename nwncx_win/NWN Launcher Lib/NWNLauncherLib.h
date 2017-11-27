#pragma once

#include <Windows.h>
#include <cstdio>
#include <string>

size_t write_data(void *ptr, size_t size, size_t nmemb, FILE *stream);
bool download_file(const char* url, const char* fname);
void LoadLibraryInProcessSync(HANDLE process, const char* library_path);
bool GetFileExists(const std::string& file_path);
DWORD GetFileSize(const std::string& file_path);