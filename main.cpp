#include "hacks.h"
#include "VMProtectSDK.h"
#include "VMPDef.h"
#include "__binary_file.h"

#pragma comment(lib, "Shlwapi.lib")
__
#define ERASE_ENTRY_POINT    TRUE
extern VOID(WINAPI* rtlZeroMemory)(PVOID, SIZE_T);
DWORD WINAPI loadLibrary(LoaderData* loaderData)
{
	PIMAGE_BASE_RELOCATION relocation = (PIMAGE_BASE_RELOCATION)(loaderData->baseAddress + loaderData->relocVirtualAddress);
	DWORD delta = (DWORD)(loaderData->baseAddress - loaderData->imageBase);
	while (relocation->VirtualAddress) {
		PWORD relocationInfo = (PWORD)(relocation + 1);
		for (int i = 0, count = (relocation->SizeOfBlock - sizeof(IMAGE_BASE_RELOCATION)) / sizeof(WORD); i < count; i++)
			if (relocationInfo[i] >> 12 == IMAGE_REL_BASED_HIGHLOW)
				*(PDWORD)(loaderData->baseAddress + (relocation->VirtualAddress + (relocationInfo[i] & 0xFFF))) += delta;

		relocation = (PIMAGE_BASE_RELOCATION)((LPBYTE)relocation + relocation->SizeOfBlock);
	}

	PIMAGE_IMPORT_DESCRIPTOR importDirectory = (PIMAGE_IMPORT_DESCRIPTOR)(loaderData->baseAddress + loaderData->importVirtualAddress);

	while (importDirectory->Characteristics) {
		PIMAGE_THUNK_DATA originalFirstThunk = (PIMAGE_THUNK_DATA)(loaderData->baseAddress + importDirectory->OriginalFirstThunk);
		PIMAGE_THUNK_DATA firstThunk = (PIMAGE_THUNK_DATA)(loaderData->baseAddress + importDirectory->FirstThunk);

		HMODULE module = loaderData->loadLibraryA((LPCSTR)loaderData->baseAddress + importDirectory->Name);

		if (!module)
			return FALSE;

		while (originalFirstThunk->u1.AddressOfData) {
			DWORD Function = (DWORD)loaderData->getProcAddress(module, originalFirstThunk->u1.Ordinal & IMAGE_ORDINAL_FLAG ? (LPCSTR)(originalFirstThunk->u1.Ordinal & 0xFFFF) : ((PIMAGE_IMPORT_BY_NAME)((LPBYTE)loaderData->baseAddress + originalFirstThunk->u1.AddressOfData))->Name);

			if (!Function)
				return FALSE;

			firstThunk->u1.Function = Function;
			originalFirstThunk++;
			firstThunk++;
		}
		importDirectory++;
	}

	if (loaderData->addressOfEntryPoint) {
		DWORD result = ((DWORD(__stdcall*)(HMODULE, DWORD, LPVOID))
			(loaderData->baseAddress + loaderData->addressOfEntryPoint))
			((HMODULE)loaderData->baseAddress, DLL_PROCESS_ATTACH, NULL);

		loaderData->rtlZeroMemory(loaderData->baseAddress + loaderData->addressOfEntryPoint, 32);

		return result;
	}
	return TRUE;
}

VOID stub(VOID) { }

VOID waitOnModule(DWORD processId, PCWSTR moduleName)
{
	BOOL foundModule = FALSE;

	while (!foundModule) {
		HANDLE moduleSnapshot = INVALID_HANDLE_VALUE;

		while (moduleSnapshot == INVALID_HANDLE_VALUE)
			moduleSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, processId);

		MODULEENTRY32W moduleEntry;
		moduleEntry.dwSize = sizeof(moduleEntry);

		if (Module32FirstW(moduleSnapshot, &moduleEntry)) {
			do {
				if (!lstrcmpiW(moduleEntry.szModule, moduleName)) {
					foundModule = TRUE;
					break;
				}
			} while (Module32NextW(moduleSnapshot, &moduleEntry));
		}
		CloseHandle(moduleSnapshot);
	}
}

VOID kill_any_steam_process()
{
	HANDLE processSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

	PROCESSENTRY32W processEntry;
	processEntry.dwSize = sizeof(processEntry);

	if (Process32FirstW(processSnapshot, &processEntry)) {
		PCWSTR steamProcesses[] = { L"Steam.exe", L"SteamService.exe", L"steamwebhelper.exe" };
		do {
			for (INT i = 0; i < _countof(steamProcesses); i++) {
				if (!lstrcmpiW(processEntry.szExeFile, steamProcesses[i])) {
					HANDLE processHandle = OpenProcess(PROCESS_TERMINATE, FALSE, processEntry.th32ProcessID);
					if (processHandle) {
						TerminateProcess(processHandle, 0);
						CloseHandle(processHandle);
					}
				}
			}
		} while (Process32NextW(processSnapshot, &processEntry));
	}
	CloseHandle(processSnapshot);
}

#include <Windows.h>
#include <ShlObj.h>
#include <shlwapi.h>
#include <TlHelp32.h> 

#undef RtlZeroMemory

#ifdef __cplusplus
extern "C"
#endif
NTSYSAPI
VOID
NTAPI
RtlZeroMemory(
	VOID UNALIGNED * Destination,
	SIZE_T Length
);

void check_time_for_use_hack(time_t time_)
{
	time_ = time(0);
	if (time_ > 1467402183) {
		ExitProcess(1);
	}
}

void create_hash_exe_name(char* generated_key)
{
	TCHAR szExeFileName[MAX_PATH];
	GetModuleFileName(NULL, szExeFileName, MAX_PATH);
	std::string path = std::string(szExeFileName);
	std::string exe = path.substr(path.find_last_of("\\") + 1, path.size());

	srand(time(0));
	char newname[17];

	int z = rand() % 5 + 5;
	for (int i = 0; i < z; i++)
	{
		char x = generated_key[rand() % 36];
		newname[i] = x;
	}
	newname[z] = 0x0;
	strcat_s(newname, ".exe\0");

	rename(exe.c_str(), newname);
}

void vac_bypass(bool switcher) // thx for http://github.com/danielkrupinski/VAC-Bypass
{
	if (switcher){
		HKEY key = NULL;
		std::cout << "" << std::endl;
		std::cout << " [!] searching steam folder" << std::endl;
		Sleep(3000);
		if (!RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\Valve\\Steam", 0, KEY_QUERY_VALUE, &key)) {
			WCHAR steamPath[MAX_PATH];
			steamPath[0] = L'"';
			DWORD steamPathSize = sizeof(steamPath) - sizeof(WCHAR);

			if (!RegQueryValueExW(key, L"SteamExe", NULL, NULL, (LPBYTE)(steamPath + 1), &steamPathSize)) {
				lstrcatW(steamPath, L"\"");
				lstrcatW(steamPath, PathGetArgsW(GetCommandLineW()));

				std::cout << " [!] killing steam process // vac bypass working" << std::endl;
				Sleep(3000);
				kill_any_steam_process();

				STARTUPINFOW info = { sizeof(info) };
				PROCESS_INFORMATION processInfo;

				if (CreateProcessW(NULL, steamPath, NULL, NULL, FALSE, 0, NULL, NULL, &info, &processInfo)) {
					waitOnModule(processInfo.dwProcessId, L"Steam.exe");
					SuspendThread(processInfo.hThread);

					PIMAGE_NT_HEADERS ntHeaders = (PIMAGE_NT_HEADERS)(binary + ((PIMAGE_DOS_HEADER)binary)->e_lfanew);

					PBYTE executableImage = (PBYTE)VirtualAllocEx(processInfo.hProcess, NULL, ntHeaders->OptionalHeader.SizeOfImage,
						MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);

					PIMAGE_SECTION_HEADER sectionHeaders = (PIMAGE_SECTION_HEADER)(ntHeaders + 1);
					for (INT i = 0; i < ntHeaders->FileHeader.NumberOfSections; i++)
						WriteProcessMemory(processInfo.hProcess, executableImage + sectionHeaders[i].VirtualAddress,
							binary + sectionHeaders[i].PointerToRawData, sectionHeaders[i].SizeOfRawData, NULL);

					LoaderData* loaderMemory = (LoaderData*)VirtualAllocEx(processInfo.hProcess, NULL, 4096, MEM_COMMIT | MEM_RESERVE,
						PAGE_EXECUTE_READ);

					LoaderData loaderParams;
					loaderParams.baseAddress = executableImage;
					loaderParams.loadLibraryA = LoadLibraryA;
					loaderParams.getProcAddress = GetProcAddress;
					loaderParams.rtlZeroMemory = RtlZeroMemory;
					loaderParams.imageBase = ntHeaders->OptionalHeader.ImageBase;
					loaderParams.relocVirtualAddress = ntHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].VirtualAddress;
					loaderParams.importVirtualAddress = ntHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress;
					loaderParams.addressOfEntryPoint = ntHeaders->OptionalHeader.AddressOfEntryPoint;
					std::cout << " [!] writing memory" << std::endl;
					Sleep(500);
					std::cout << " [!] lauching steam..." << std::endl;
					Sleep(1000);
					WriteProcessMemory(processInfo.hProcess, loaderMemory, &loaderParams, sizeof(LoaderData),
						NULL);
					WriteProcessMemory(processInfo.hProcess, loaderMemory + 1, loadLibrary,
						(DWORD)stub - (DWORD)loadLibrary, NULL);
					HANDLE thread = CreateRemoteThread(processInfo.hProcess, NULL, 0, (LPTHREAD_START_ROUTINE)(loaderMemory + 1),
						loaderMemory, 0, NULL);

					ResumeThread(processInfo.hThread);
					WaitForSingleObject(thread, INFINITE);
					VirtualFreeEx(processInfo.hProcess, loaderMemory, 0, MEM_RELEASE);
					std::cout << " [+] ready to launch hack..." << std::endl;
					Sleep(3000);
					CloseHandle(processInfo.hProcess);
					CloseHandle(processInfo.hThread);
				}
			}
			RegCloseKey(key);
		}

	}std::cout << "" << std::endl;
	std::cout << " [!] waiting csgo.exe process" << std::endl;
}


uint32_t APIENTRY ____unknowncallx64_(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
	VMP

	_create_hash_exe_name((char*)"0123456789abcdefghijklmnopqrstuvwxyz");
	_vac_bypass((bool)true); // off - false, on - true;	
	_sleep((int)3000);

	_return Hack->attach();

	VMPEND 
}

_int __mainEncrypt040404040200402050004050450461069167230532135135013061061306013006100() {
	____unknowncallx64_(NULL, NULL, NULL, NULL);
_return NULL;
}