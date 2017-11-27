#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#define CRYPTOPP_ENABLE_NAMESPACE_WEAK  1
#endif

#include <Windows.h>
#include <string>
#include "sinfarx_file_map.h"
#include "../NWN Launcher Lib/NWNLauncherLib.h"
#include <json/reader.h>
#include <fstream>
#include <cryptopp562/md5.h>
#include <cryptopp562/hex.h>
#include "ini/IniFile.h"

const char* INI_SECTION = "SinfarX";
const char* INI_KEY_NWMAIN = "nwmain";
const char* INI_KEY_AUTO_UPDATE = "CheckForUpdates";

bool SetCurrentDirectory_IsNWNDirectory(const std::string directory)
{
	//if (!GetFileExists(directory)) return false;
	SetCurrentDirectoryA(directory.c_str());
	return (GetFileExists("./dialog.tlk") && GetFileExists("./xp1.key") /*&& GetFileExists("xp3.key")*/);
}

bool TestFileAgainstMD5(const std::string& file_path, const std::string& MD5)
{
	FILE* f = fopen(file_path.c_str(), "rb");
	fseek(f, 0, SEEK_END);
	size_t file_size = ftell(f);
	fseek(f, 0, SEEK_SET);
	BYTE* file_data = new BYTE[file_size];
	fread(file_data, file_size, 1, f);
	fclose(f);
	CryptoPP::Weak1::MD5 md5;
	BYTE file_md5_digest[CryptoPP::Weak1::MD5::DIGESTSIZE];
	md5.CalculateDigest(file_md5_digest, file_data, file_size);
	delete[] file_data;
	CryptoPP::HexEncoder encoder;
	std::string output;
	encoder.Attach(new CryptoPP::StringSink(output));
	encoder.Put(file_md5_digest, CryptoPP::Weak1::MD5::DIGESTSIZE);
	encoder.MessageEnd();
	return (output == MD5);
}

bool GetIsNWMainValid(const std::string& nwmain_path)
{
	return (GetFileSize(nwmain_path) == 5661928);
}

char* RemoveFilenameFromPath(char* file_path)
{
	char* file_path_temp = file_path;
	char* last_backslash = file_path;
	while (*file_path_temp)
	{
		if (*file_path_temp == '\\' || *file_path_temp == '/') last_backslash = file_path_temp;
		file_path_temp++;
	}
	*last_backslash = 0;
	return file_path;
}

int WINAPI WinMain(HINSTANCE hInstance,
                   HINSTANCE hPrevInstance,
                   LPSTR lpCmdLine,
                   int nCmdShow)
{
	char current_directory[MAX_PATH];
	GetCurrentDirectoryA(MAX_PATH, current_directory);
	char current_drive[2];
	sprintf(current_drive, "%c", current_directory[0]);

	char exe_directory[MAX_PATH];
	GetModuleFileNameA(GetModuleHandleA(NULL), exe_directory, MAX_PATH);
	RemoveFilenameFromPath(exe_directory);
	
	if (!SetCurrentDirectory_IsNWNDirectory(current_directory) &&
		!SetCurrentDirectory_IsNWNDirectory(exe_directory))
	{
		DWORD dwType = REG_SZ;
		HKEY hKey = 0;
		char reg_nwn_location[MAX_PATH] = { 0 };
		DWORD reg_nwn_location_size = MAX_PATH;
		bool found_reg_key = false;
		if (RegOpenKeyA(HKEY_LOCAL_MACHINE, "SOFTWARE\\Bioware\\NWN\\Neverwinter", &hKey) == ERROR_SUCCESS)
		{
			if (RegQueryValueExA(hKey, "Location", NULL, &dwType, (LPBYTE)reg_nwn_location, &reg_nwn_location_size) == ERROR_SUCCESS)
			{
				found_reg_key = SetCurrentDirectory_IsNWNDirectory(reg_nwn_location);
			}
		}

		if (!found_reg_key &&
			!SetCurrentDirectory_IsNWNDirectory(std::string(current_drive) + ":\\NeverwinterNights\\NWN") &&
			!SetCurrentDirectory_IsNWNDirectory(std::string(current_drive) + ":\\GOG\\Neverwinter Nights Diamond Edition"))
		{
			MessageBoxA(0, "Please move this file in your NWN directory.", "nwmain.exe couldn't be located", 0);
			return 0;
		}
	}

	//read ini to get a custom nwmain.exe path
	std::string ini_filepath = std::string(exe_directory) + "/sinfarx.ini";
	std::string nwmain_exe_path = "./nwmain.exe";
	CIniFileA ini;
	ini.Load(ini_filepath);
	CIniSectionA* ini_section = ini.GetSection(INI_SECTION);
	if (ini_section)
	{
		nwmain_exe_path = ini_section->GetKeyValue(INI_KEY_NWMAIN);
	}

	if (!GetIsNWMainValid(nwmain_exe_path))
	{
		nwmain_exe_path = "./nwmain.exe";
		if (!GetIsNWMainValid(nwmain_exe_path))
		{
			nwmain_exe_path.clear();
			WIN32_FIND_DATAA find_data;
			HANDLE hFind = FindFirstFileA("*.exe", &find_data);
			if (hFind != INVALID_HANDLE_VALUE) do
			{
				if (GetIsNWMainValid(find_data.cFileName))
				{
					nwmain_exe_path = find_data.cFileName;
				}
			} while (FindNextFileA(hFind, &find_data));
			if (nwmain_exe_path.empty())
			{
				MessageBoxA(0, "Your NWN installation is not compatible!", "", MB_ICONERROR);
				return 0;
			}
		}
	}

	//get auto update
	Json::Value nwncx_sinfar_config_data;
	bool auto_update = (!ini_section || ini_section->GetKeyValue(INI_KEY_AUTO_UPDATE) !="0");
	
	//save ini settings
	if (ini_section) ini_section->RemoveAllKeys(); //cleanup old settings
	ini.SetKeyValue(INI_SECTION, INI_KEY_NWMAIN, nwmain_exe_path);
	ini.SetKeyValue(INI_SECTION, INI_KEY_AUTO_UPDATE, auto_update ? "1" : "0");
	ini.Save(ini_filepath);

	if (auto_update)
	{
		char temp_dir[MAX_PATH];
		GetTempPathA(MAX_PATH, temp_dir);
		std::string nwncx_sinfar_config_path = temp_dir + std::string("/nwncx_sinfar.txt");
		if (download_file("http://nwn.sinfar.net/nwncx_sinfar.txt", nwncx_sinfar_config_path.c_str()))
		{
			std::ifstream ifs;
			ifs.open (nwncx_sinfar_config_path, std::ifstream::in);
			Json::Reader json_reader;
			json_reader.parse(ifs, nwncx_sinfar_config_data, false);
			ifs.close();
			_unlink(nwncx_sinfar_config_path.c_str());
		}
		else
		{
			auto_update = false;
		}
	}
	
	std::string nwncx_sinfar_path = std::string(exe_directory) + "/nwncx_sinfar.dll";
	if (!GetFileExists(nwncx_sinfar_path))
	{
		nwncx_sinfar_path = "./nwncx_sinfar.dll";
	}

	if (auto_update)
	{
		if (!GetFileExists(nwncx_sinfar_path) || !TestFileAgainstMD5(nwncx_sinfar_path, nwncx_sinfar_config_data.get("nwncxSinfarMD5", "").asString()))
		{
			if (MessageBoxA(0, "You do not have the latest version of nwncx_sinfar.dll, do you want to download it now?", "", MB_YESNO | MB_ICONWARNING) == IDYES)
			{
				if (!download_file("http://nwn.sinfar.net/files/nwncx_sinfar.dll", nwncx_sinfar_path.c_str()))
				{
					MessageBoxA(0, "The download of nwncx_sinfar.dll failed.", "error", MB_ICONWARNING);
				}
			}
		}
	}

	if (GetFileExists(nwncx_sinfar_path))
	{
		STARTUPINFOA si;
		PROCESS_INFORMATION pi;
		ZeroMemory(&si, sizeof(STARTUPINFO));
		si.cb = sizeof(STARTUPINFO);
		si.dwFlags = STARTF_USESHOWWINDOW;
		si.wShowWindow = SW_SHOWNORMAL;
		if (!CreateProcessA(NULL, (char*)(nwmain_exe_path + " " + lpCmdLine).c_str(), NULL, NULL, false, CREATE_NEW_CONSOLE|CREATE_SUSPENDED, NULL, NULL, &si, &pi))
		{
			MessageBoxA(0, "Failed to start nwmain.exe!", "", 0);
			return 1;
		}
		
		HANDLE file_map_handle = CreateFileMappingA(INVALID_HANDLE_VALUE, NULL, PAGE_EXECUTE_READWRITE, 0, sizeof(SINFARX_FILE_MAP), "SINFARX");	
		SINFARX_FILE_MAP* file_map = (SINFARX_FILE_MAP*)MapViewOfFile(file_map_handle, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(SINFARX_FILE_MAP));
		file_map->sinfarx_version = 370;
		//load plugin and launch NWN!
		LoadLibraryInProcessSync(pi.hProcess, nwncx_sinfar_path.c_str());
		ResumeThread(pi.hThread);
		return 0;
	}
	else
	{
		MessageBoxA(0, (std::string("File not found: ") + nwncx_sinfar_path).c_str(), "nwncx_sinfar.dll not found", 0);
		return 1;
	}
}