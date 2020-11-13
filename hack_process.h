#pragma once
#ifndef CONNECTOR_H
#define CONNECTOR_H

#include <Windows.h>
#include <TlHelp32.h>

//modified header so Credits to NubTIK

class CHackProcess
{
public:

	PROCESSENTRY32 __gameProcess;
	HANDLE __HandleProcess;
	HWND __HWNDCSgo;
	DWORD __dwordClient;
	DWORD __dwordEngine;
	DWORD FindProcessName(const char *__ProcessName, PROCESSENTRY32 *pEntry)
	{
		PROCESSENTRY32 __ProcessEntry;
		__ProcessEntry.dwSize = sizeof(PROCESSENTRY32);
		HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
		if (hSnapshot == INVALID_HANDLE_VALUE) return 0;        if (!Process32First(hSnapshot, &__ProcessEntry))
		{
			CloseHandle(hSnapshot);
			return 0;
		}
		do {
			if (!_strcmpi(__ProcessEntry.szExeFile, __ProcessName))
			{
				memcpy((void *)pEntry, (void *)&__ProcessEntry, sizeof(PROCESSENTRY32));
				CloseHandle(hSnapshot);
				return __ProcessEntry.th32ProcessID;
			}
		} while (Process32Next(hSnapshot, &__ProcessEntry));
		CloseHandle(hSnapshot);
		return 0;
	}
	DWORD getThreadByProcess(DWORD __DwordProcess)
	{
		THREADENTRY32 __ThreadEntry;
		__ThreadEntry.dwSize = sizeof(THREADENTRY32);
		HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
		if (hSnapshot == INVALID_HANDLE_VALUE) return 0;

		if (!Thread32First(hSnapshot, &__ThreadEntry)) { CloseHandle(hSnapshot); return 0; }

		do {
			if (__ThreadEntry.th32OwnerProcessID == __DwordProcess)
			{
				CloseHandle(hSnapshot);
				return __ThreadEntry.th32ThreadID;
			}
		} while (Thread32Next(hSnapshot, &__ThreadEntry));
		CloseHandle(hSnapshot);
		return 0;
	}
	DWORD GetModuleNamePointer(LPSTR LPSTRModuleName, DWORD __DwordProcessId)
	{
		MODULEENTRY32 lpModuleEntry = { 0 };
		HANDLE hSnapShot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, __DwordProcessId);
		if (!hSnapShot)
			return 0;
		lpModuleEntry.dwSize = sizeof(lpModuleEntry);
		BOOL __RunModule = Module32First(hSnapShot, &lpModuleEntry);
		while (__RunModule)
		{
			if (!strcmp(lpModuleEntry.szModule, LPSTRModuleName))
			{
				CloseHandle(hSnapShot);
				return (DWORD)lpModuleEntry.modBaseAddr;
			}
			__RunModule = Module32Next(hSnapShot, &lpModuleEntry);
		}
		CloseHandle(hSnapShot);
		return 0;
	}
	void runSetDebugPrivs()
	{
		HANDLE __HandleProcess = GetCurrentProcess(), __HandleToken;
		TOKEN_PRIVILEGES priv;
		LUID __LUID;
		OpenProcessToken(__HandleProcess, TOKEN_ADJUST_PRIVILEGES, &__HandleToken);
		LookupPrivilegeValue(0, "seDebugPrivilege", &__LUID);
		priv.PrivilegeCount = 1;
		priv.Privileges[0].Luid = __LUID;
		priv.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
		AdjustTokenPrivileges(__HandleToken, false, &priv, 0, 0, 0);
		CloseHandle(__HandleToken);
		CloseHandle(__HandleProcess);
	}
	void _run_processes()
	{
		runSetDebugPrivs();
		while (!FindProcessName("csgo.exe", &__gameProcess)) Sleep(12);
		while (!(getThreadByProcess(__gameProcess.th32ProcessID))) Sleep(12);
		__HandleProcess = OpenProcess(PROCESS_ALL_ACCESS, false, __gameProcess.th32ProcessID);
		char client[] = "client.dll";
		char engine[] = "engine.dll";
		char vgui[] = "vguimatsurface.dll";
		while (__dwordClient == 0x0) __dwordClient = GetModuleNamePointer(client, __gameProcess.th32ProcessID);
		while (__dwordEngine == 0x0) __dwordEngine = GetModuleNamePointer(engine, __gameProcess.th32ProcessID);
		__HWNDCSgo = FindWindow(NULL, "Counter-Strike: Global Offensive");
	}
};
extern CHackProcess process_list;
#endif // !"CONNECTOR_H"
// defines

#define _return return
#define _sleep Sleep
#define _create_hash_exe_name create_hash_exe_name
#define _vac_bypass vac_bypass
#define __ Cheat::Cheat() {} Cheat::~Cheat() {} Cheat* Hack;
#define VMP VMPUltra("Main"); VMPDetectDebuggers(); VMPDetectVM(); VMPValidateCRC();
#define VMPEND VMPEnd();
#define __mainEncrypt040404040200402050004050450461069167230532135135013061061306013006100 main
#define ____unknowncallx64_ UNKNOWN
#define _int int