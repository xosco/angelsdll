#include <windows.h>
#include <vector>
#include <tlhelp32.h>
#include <cstring>
#include <string>
#include <sstream>
#include <iostream>
#include <fstream>
#include <map>
#include <iomanip>
#include "xorstr.h"
bool replasing = false;
char id[9]{};
char newid[9]{};
char config[13]{};
int typepattern = 0;
namespace fs = std::filesystem;
std::string FileName;
std::string cfgPath = "C:\\ProgramData\\UdClient\\Configs\\";
int count = 0;
int errorsc = 0;
bool bypasschams;

void ReplaceSkin(const std::string &pattern, const std::string &replace) {
	std::vector<BYTE> patterns;
	std::vector<BYTE> replaces;
	std::istringstream issOld(pattern);
	std::istringstream issNew(replace);
	std::string byteStr;
	replasing = true;
	while (issOld >> byteStr) {
		BYTE byte = static_cast<BYTE>(std::stoi(byteStr, nullptr, 16));
		patterns.push_back(byte);
	}
	while (issNew >> byteStr) {
		BYTE byte = static_cast<BYTE>(std::stoi(byteStr, nullptr, 16));
		replaces.push_back(byte);
	}
	SYSTEM_INFO si;
	GetSystemInfo(&si);
	uintptr_t startaddress = 0;
	uintptr_t endaddress = 0;
	if (startaddress == 0) {
		startaddress = (uintptr_t)(si.lpMinimumApplicationAddress);
	}
	if (endaddress == 0) {
		endaddress = (uintptr_t)(si.lpMaximumApplicationAddress);
	}
	MEMORY_BASIC_INFORMATION mbi{ 0 };
	auto protectflags = (PAGE_GUARD | PAGE_NOCACHE | PAGE_NOACCESS);
	for (uintptr_t i = startaddress; i < endaddress - patterns.size(); i++) {
		if (VirtualQuery((LPCVOID)i, &mbi, sizeof(mbi))) {
			if (mbi.Protect & protectflags || mbi.Protect & PAGE_READONLY || !(mbi.State & MEM_COMMIT)) {
				i += mbi.RegionSize;
				continue;
			}
			for (uintptr_t k = (uintptr_t)mbi.BaseAddress; k < (uintptr_t)mbi.BaseAddress + mbi.RegionSize - patterns.size(); k++) {
				for (uintptr_t j = 0; j < patterns.size(); j++) {
					if (patterns.at(j) != -1 && patterns.at(j) != *(byte *)(k + j))
						break;
					if (j + 1 == patterns.size()) {
						DWORD OldProtection;
						if (VirtualProtect((LPVOID)k, replaces.size(), PAGE_EXECUTE_READWRITE, &OldProtection)) {
							SIZE_T bytesWritten;
							if (!WriteProcessMemory(GetCurrentProcess(), (LPVOID)k, &replaces[0], replaces.size(), &bytesWritten) || bytesWritten != replaces.size()) {
								errorsc++;
							}
							else {
								count++;
							}
							VirtualProtect((LPVOID)k, replaces.size(), OldProtection, NULL);
						}
						else {
							errorsc++;
						}
					}
				}
			}
			i = (uintptr_t)mbi.BaseAddress + mbi.RegionSize;
		}
	}
	replasing = false;
	count = 0;
	errorsc = 0;
}

void tobyte()
{
	std::string id_str = id;
	std::string newid_str = newid;

	int id_value = std::stoi(id_str);
	int newid_value = std::stoi(newid_str);

	std::vector<BYTE> byte_arr;
	for (int i = 0; i < 4; i++) {
		byte_arr.push_back(static_cast<BYTE>((id_value >> (i * 8)) & 0xFF));
	}

	std::string byte_arr_with_padding;
	for (const BYTE &byte : byte_arr) {
		std::stringstream ss;
		ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(byte);
		byte_arr_with_padding += ss.str() + " ";
	}

	std::vector<BYTE> byte_array;
	for (int i = 0; i < 4; i++) {
		byte_array.push_back(static_cast<BYTE>((newid_value >> (i * 8)) & 0xFF));
	}

	std::string byte_array_with_padding;
	for (const BYTE &byte : byte_array) {
		std::stringstream ss;
		ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(byte);
		byte_array_with_padding += ss.str() + " ";
	}

	std::string pattern = byte_arr_with_padding + ("01 00 00 00\n");
	std::string replace = byte_array_with_padding + ("01 00 00 00\n");

	ReplaceSkin(pattern, replace);
}


void tobytecfg()
{
	replasing = true;
	std::ifstream file(cfgPath + FileName);
	if (!file.is_open()) {
		return;
	}

	std::string line;
	while (std::getline(file, line)) {
		std::istringstream iss(line);
		std::string id_str, newid_str;
		if (!std::getline(iss, id_str, '=') || !std::getline(iss, newid_str)) {
			continue;
		}
		int id_value = std::stoi(id_str);
		int newid_value = std::stoi(newid_str);

		std::vector<BYTE> byte_arr;
		for (int i = 0; i < 4; i++) {
			byte_arr.push_back(static_cast<BYTE>((id_value >> (i * 8)) & 0xFF));
		}

		std::string byte_arr_with_padding;
		for (const BYTE &byte : byte_arr) {
			std::stringstream ss;
			ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(byte);
			byte_arr_with_padding += ss.str() + " ";
		}

		std::vector<BYTE> byte_array;
		for (int i = 0; i < 4; i++) {
			byte_array.push_back(static_cast<BYTE>((newid_value >> (i * 8)) & 0xFF));
		}

		std::string byte_array_with_padding;
		for (const BYTE &byte : byte_array) {
			std::stringstream ss;
			ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(byte);
			byte_array_with_padding += ss.str() + " ";
		}

		std::string pattern = byte_arr_with_padding + ("01 00 00 00\n");;
		std::string replace = byte_array_with_padding + ("01 00 00 00\n");;

		ReplaceSkin(pattern, replace);
	}
	file.close();
}

void Delete()
{
	std::string filePath = cfgPath + FileName;
	std::remove(filePath.c_str());
}
bool updates = true;
bool updatefile;
bool confedit;
std::string filecfg = cfgPath + FileName;
std::string fileContent;
std::vector<std::string> fileNames;
void ReadFileContent() {
	std::ifstream fileStream(cfgPath + FileName);
	if (fileStream.is_open())
	{
		std::stringstream strStream;
		strStream << fileStream.rdbuf();
		fileContent = strStream.str();
		fileStream.close();
	}
}
void SaveFileContent() {
	std::ofstream fileStream(cfgPath + FileName);
	if (fileStream.is_open())
	{
		fileStream << fileContent;
		fileStream.close();
	}
}

void CheckFilesInFolder() {
	fs::directory_iterator entry;
	fileNames.clear();
	for (entry = fs::directory_iterator(cfgPath); entry != fs::directory_iterator(); ++entry) {
		if (entry->path().extension() == ".ud") {
			std::string buttonLabel = entry->path().filename().string();
			fileNames.push_back(buttonLabel);
		}
	}
}

void drawfromvector()
{
	for (const auto &buttonLabel : fileNames) {
		if (ImGui::Button(buttonLabel.c_str(), ImVec2(345, 20))) {
			FileName = buttonLabel;
			updatefile = true;
		}
		if (ImGui::IsItemClicked(1)) {
			FileName = buttonLabel;
			ImGui::OpenPopup("Options");
		}
	}
	if (ImGui::BeginPopup("Options")) {
		if (ImGui::MenuItem("Edit")) {
			updatefile = true;
			confedit = true;
		}
		if (ImGui::MenuItem("Delete")) {
			Delete();
			updates = true;
			FileName.clear();
		}
		ImGui::EndPopup();
	}
	if (updatefile)
	{
		ReadFileContent();
		updatefile = false;
	}
}


std::vector<DWORD> bypassadr;
std::vector<DWORD> radaradr;
typedef unsigned char byte;

void ScanAndReplace(const std::string &pattern, const std::string &replace, std::vector<DWORD> &addresses) {
	std::vector<BYTE> patterns;
	std::vector<BYTE> replaces;
	std::istringstream issOld(pattern);
	std::istringstream issNew(replace);
	std::string byteStr;
	while (issOld >> byteStr) {
		BYTE byte = static_cast<BYTE>(std::stoi(byteStr, nullptr, 16));
		patterns.push_back(byte);
	}
	while (issNew >> byteStr) {
		BYTE byte = static_cast<BYTE>(std::stoi(byteStr, nullptr, 16));
		replaces.push_back(byte);
	}

	SYSTEM_INFO si;
	GetSystemInfo(&si);

	uintptr_t startaddress = 0x0aaaaaaa0;
	uintptr_t endaddress = 0xFFFFFFFF;

	if (startaddress == 0) {
		startaddress = (uintptr_t)(si.lpMinimumApplicationAddress);
	}
	if (endaddress == 0) {
		endaddress = (uintptr_t)(si.lpMaximumApplicationAddress);
	}

	MEMORY_BASIC_INFORMATION mbi{ 0 };
	auto protectflags = (PAGE_GUARD | PAGE_NOCACHE | PAGE_NOACCESS);

	for (uintptr_t i = startaddress; i < endaddress - patterns.size(); i++) {
		if (VirtualQuery((LPCVOID)i, &mbi, sizeof(mbi))) {
			if (mbi.Protect & protectflags || !(mbi.State & MEM_COMMIT)) {
				i += mbi.RegionSize;
				continue;
			}
			for (uintptr_t k = (uintptr_t)mbi.BaseAddress; k < (uintptr_t)mbi.BaseAddress + mbi.RegionSize - patterns.size(); k++) {
				for (uintptr_t j = 0; j < patterns.size(); j++) {
					if (patterns.at(j) != -1 && patterns.at(j) != *(byte *)(k + j))
						break;
					if (j + 1 == patterns.size()) {
						unsigned long OldProtection;
						VirtualProtect((LPVOID)k, replaces.size(), PAGE_EXECUTE_READWRITE, &OldProtection);
						memcpy((void *)k, &replaces[0], replaces.size());
						VirtualProtect((LPVOID)k, replaces.size(), OldProtection, NULL);
						addresses.push_back(k);
						bypasschams = true;
						return;
					}
				}
			}
			i = (uintptr_t)mbi.BaseAddress + mbi.RegionSize;
		}
	}
}

void Unhooking(std::vector<DWORD> &addresses, const std::string &pattern) {
	std::istringstream issOld(pattern);
	std::string byteStr;
	while (issOld >> byteStr) {
		BYTE byte = static_cast<BYTE>(std::stoi(byteStr, nullptr, 16));

		for (auto &address : addresses) {
			unsigned long OldProtection;
			VirtualProtect((LPVOID)address, 1, PAGE_EXECUTE_READWRITE, &OldProtection);
			memcpy((void *)address, &byte, 1);
			VirtualProtect((LPVOID)address, 1, OldProtection, NULL);
		}
	}
}

void bypassingchams()
{
	std::string pattern = (skCrypt("70 40 2D E9 74 50 9F E5 00 40 A0 E1 05 50 8F E0 00 00 D5 E5 00 00 50 E3 04 00 00 1A 60 00 9F E5 00 00 9F E7 C7 32 D2"));
	std::string replace = (skCrypt("1E FF 2E E1 74 50 9F E5 00 40 A0 E1 05 50 8F E0 00 00 D5 E5 00 00 50 E3 04 00 00 1A 60 00 9F E5 00 00 9F E7 4E 33 B9"));
	if (!bypasschams)
	{
		ScanAndReplace(pattern, replace, bypassadr);
	}
	else
	{
		Unhooking(bypassadr, pattern);
		bypassadr.clear();
		bypasschams = false;
	}
}
