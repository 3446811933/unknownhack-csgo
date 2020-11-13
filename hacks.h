#include <Windows.h>
#include <iostream>
#include <vector>
#include <math.h>
#include <thread>
#include <chrono>
#include "hack_process.h"
#include "csgo.hpp"
using namespace hazedumper::netvars;
using namespace hazedumper::signatures;
struct Vector3
{
	float x, y, z;
};

struct EntityInfo
{
	float x, y, z;
}entity[20], player;

DWORD CS;
CHackProcess process_list;
/* 
BOOL IsAdmin()
{
	HANDLE hAccessToken = NULL;
	BOOL bSuccess = OpenThreadToken(GetCurrentThread(), TOKEN_QUERY, TRUE, &hAccessToken);

	if (!bSuccess)
	{
		if (GetLastError() == ERROR_NO_TOKEN)
		{
			bSuccess = OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hAccessToken);
		}
	}

	if (bSuccess)
	{
		PTOKEN_GROUPS ptgGroups = NULL;
		DWORD dwInfoBufferSize = 0;
		bSuccess = GetTokenInformation(hAccessToken, TokenGroups, NULL, dwInfoBufferSize, &dwInfoBufferSize);
		ptgGroups = (PTOKEN_GROUPS)malloc(dwInfoBufferSize);
		bSuccess = GetTokenInformation(hAccessToken, TokenGroups, (PTOKEN_GROUPS)ptgGroups, dwInfoBufferSize, &dwInfoBufferSize);

		CloseHandle(hAccessToken);

		if (bSuccess)
		{
			PSID psidAdministrators;
			SID_IDENTIFIER_AUTHORITY ia = SECURITY_NT_AUTHORITY;

			AllocateAndInitializeSid(&ia, 2,
				SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0, 0, 0, &psidAdministrators);

			for (DWORD i = 0; i < ptgGroups->GroupCount; i++)
			{
				BOOL bEqual = EqualSid(psidAdministrators, ptgGroups->Groups[i].Sid);

				if (bEqual)
				{
					FreeSid(psidAdministrators);
					free(ptgGroups);

					return TRUE;
				}
			}

			FreeSid(psidAdministrators);
		}

		free(ptgGroups);
	}

	return FALSE;
}
*/

typedef struct {
	PBYTE baseAddress;
	HMODULE(WINAPI* loadLibraryA)(PCSTR);
	FARPROC(WINAPI* getProcAddress)(HMODULE, PCSTR);
	VOID(WINAPI* rtlZeroMemory)(PVOID, SIZE_T); 

	DWORD imageBase;
	DWORD relocVirtualAddress;
	DWORD importVirtualAddress;
	DWORD addressOfEntryPoint;
} LoaderData;

namespace config
{

	bool anti_flash_enable = false;
	bool radar_hack_enable = false;
	bool chams_enable = false;
	bool triggerbot_enable = false;
	bool checkdefuse_enable = false;
	bool bWall_enable = false;
	bool bhop_enable = false;
	bool sound_esp_enable = false;

}

struct myPlayer_T
{
	DWORD dwLocalP = 0x0;
	int flash = 0;
	int iTeam = 0;
	int iHealth = 0;
	int iCrossID = 0;

	void _read_info()
	{
		//Read this all the time..
		ReadProcessMemory(process_list.__HandleProcess, (PBYTE*)(process_list.__dwordClient + dwLocalPlayer), &dwLocalP, sizeof(DWORD), 0);
		ReadProcessMemory(process_list.__HandleProcess, (PBYTE*)(dwLocalP + m_flFlashMaxAlpha), &flash, sizeof(int), 0);
		ReadProcessMemory(process_list.__HandleProcess, (PBYTE*)(dwLocalP + m_iTeamNum), &iTeam, sizeof(int), 0);
		ReadProcessMemory(process_list.__HandleProcess, (PBYTE*)(dwLocalP + m_iCrosshairId), &iCrossID, sizeof(int), 0);
		ReadProcessMemory(process_list.__HandleProcess, (PBYTE*)(dwLocalP + m_iHealth), &iHealth, sizeof(int), 0);
	}
}_player;

bool read_internal_memory()
{
	for (int i = 1; i < 20; i++)
	{
		DWORD Entity;
		ReadProcessMemory(process_list.__HandleProcess, (LPCVOID)(process_list.__dwordClient + dwEntityList + (i * 0x10)), &Entity, sizeof(DWORD), NULL);
		if (!Entity)
			continue;

		int IClientNetworkable, GetClientClass, pClientClass, classID, health, Entityteam, Localteam = 0;

		ReadProcessMemory(process_list.__HandleProcess, (LPCVOID)((DWORD)Entity + 0x8), &IClientNetworkable, sizeof(IClientNetworkable), NULL);
		ReadProcessMemory(process_list.__HandleProcess, (LPCVOID)(IClientNetworkable + 0x8), &GetClientClass, sizeof(GetClientClass), NULL);
		ReadProcessMemory(process_list.__HandleProcess, (LPCVOID)(GetClientClass + 0x1), &pClientClass, sizeof(pClientClass), NULL);
		ReadProcessMemory(process_list.__HandleProcess, (LPCVOID)(pClientClass + 0x14), &classID, sizeof(classID), NULL);
		if (classID != 40)
			continue;

		ReadProcessMemory(process_list.__HandleProcess, (LPCVOID)((DWORD)Entity + m_iHealth), &health, sizeof(health), NULL);
		if (health <= 0)
			continue;

		ReadProcessMemory(process_list.__HandleProcess, (LPCVOID)((DWORD)Entity + m_iTeamNum), &Entityteam, sizeof(Entityteam), NULL);
		ReadProcessMemory(process_list.__HandleProcess, (LPCVOID)((DWORD)_player.dwLocalP + m_iTeamNum), &Localteam, sizeof(Localteam), NULL);
		if (Entityteam == Localteam)
			continue;

		ReadProcessMemory(process_list.__HandleProcess, (LPCVOID)((DWORD)Entity + m_vecOrigin), &entity[i].x, sizeof(entity[i].x), NULL);
		ReadProcessMemory(process_list.__HandleProcess, (LPCVOID)((DWORD)Entity + m_vecOrigin + 4), &entity[i].y, sizeof(entity[i].y), NULL);
		ReadProcessMemory(process_list.__HandleProcess, (LPCVOID)((DWORD)Entity + m_vecOrigin + 8), &entity[i].z, sizeof(entity[i].z), NULL);

		Vector3 distance;
		distance.x = entity[i].x - player.x;
		distance.y = entity[i].y - player.y;
		distance.z = entity[i].z - player.z;
		float distance_XY_Plane = sqrt(pow(distance.x, 2) + pow(distance.y, 2));
		if ((distance.x / distance_XY_Plane) > 1 || (distance.x / distance_XY_Plane) < -1)
			continue;

		Vector3 Target;
		Target.y = std::acos(distance.x / distance_XY_Plane) * 180 / 3.141592; //pi
		Target.y *= (entity[i].y < player.y) ? -1 : 1;
		Target.x = -1 * std::atan(distance.z / distance_XY_Plane) * 180 / 3.141592; //pi

		Vector3 ViewAngles;
		ReadProcessMemory(process_list.__HandleProcess, (LPCVOID)(CS + dwClientState_ViewAngles), &ViewAngles, sizeof(ViewAngles), NULL);
		Vector3 distanceFromCross;

		distanceFromCross.x = abs(ViewAngles.x - Target.x);
		distanceFromCross.y = abs(ViewAngles.y - Target.y);
		distanceFromCross.z = sqrt(pow(distanceFromCross.x, 2) + pow(distanceFromCross.y, 2)); //actual distance

		if (abs(distanceFromCross.x <= 10.f) && abs(distanceFromCross.y <= 2.f))
			return true;
	}
	return false;
}

// hacks

namespace bhop
{
	int Flag;
	int jump = 5;
	int lock = 4;

	void enable()
	{
		if (config::bhop_enable)
		{
			if (GetAsyncKeyState(VK_SPACE)) 
			{
				ReadProcessMemory(process_list.__HandleProcess, (LPVOID)(_player.dwLocalP + m_fFlags), &Flag, sizeof(int), 0);
				if (Flag == 257 || Flag == 263) 
					WriteProcessMemory(process_list.__HandleProcess, (LPVOID)(process_list.__dwordClient + dwForceJump), (LPCVOID)&jump, sizeof(int), 0);
				else
					WriteProcessMemory(process_list.__HandleProcess, (LPVOID)(process_list.__dwordClient + dwForceJump), (LPCVOID)&lock, sizeof(int), 0);
			}
		}
	}
}

namespace triggerbot
{
	void enable()
	{
		DWORD aimedEntity = 0x0;
		int entityTeam = 0;
		if (config::triggerbot_enable)
		{
			int AimedAt = _player.iCrossID;
			if (AimedAt > 0 && AimedAt < 64)
			{
				ReadProcessMemory(process_list.__HandleProcess, (PBYTE*)(process_list.__dwordClient + dwEntityList + (AimedAt - 1) * (0x10)), &aimedEntity, sizeof(DWORD), 0);
			}
			for (int i = 0; i < 64; i++)
			{
				ReadProcessMemory(process_list.__HandleProcess, (PBYTE*)(aimedEntity + m_iTeamNum), &entityTeam, sizeof(int), 0);
			}
			if (_player.iCrossID > 0 && entityTeam != _player.iTeam)
			{
				Sleep(14);
				mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
				Sleep(10);
				mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
			}
		}
	}

}

namespace sound_esp
{
	void hook()
	{
		ReadProcessMemory(process_list.__HandleProcess, (LPCVOID)((DWORD)_player.dwLocalP + m_vecOrigin), &player.x, sizeof(player.x), NULL);
		ReadProcessMemory(process_list.__HandleProcess, (LPCVOID)((DWORD)_player.dwLocalP + m_vecOrigin + 4), &player.y, sizeof(player.y), NULL);
		ReadProcessMemory(process_list.__HandleProcess, (LPCVOID)((DWORD)_player.dwLocalP + m_vecOrigin + 8), &player.z, sizeof(player.z), NULL);

		if (read_internal_memory())
			Beep(550, 70);
	}

	void target()
	{
		ReadProcessMemory(process_list.__HandleProcess, (LPCVOID)(process_list.__dwordEngine + dwClientState), &CS, sizeof(CS), NULL);

		ReadProcessMemory(process_list.__HandleProcess, (LPCVOID)(process_list.__dwordClient + dwLocalPlayer), &_player.dwLocalP, sizeof(DWORD), NULL);
		int health = 0;
		ReadProcessMemory(process_list.__HandleProcess, (LPCVOID)((DWORD)_player.dwLocalP + m_iHealth), &health, sizeof(health), NULL);
		if (health > 0)
			sound_esp::hook();
		std::this_thread::sleep_for(std::chrono::milliseconds(10));

	}
}

namespace CSGOutils
{
	void glow_in_wall()
	{
		DWORD glowObj = 0x0;
		DWORD entity = 0x0;

		int glowIndex = 0;
		int entityTeam = 0; //actually EntityTeam

		bool bOccluded = true;
		bool bUnoccluded = false;

		float full = 1.f; //255
		float alpha = .7f;
		//int glowStyle = 2;

		ReadProcessMemory(process_list.__HandleProcess, (PBYTE*)(process_list.__dwordClient + dwGlowObjectManager), &glowObj, sizeof(DWORD), 0);

		for (int i = 0; i < 10; i++)
		{
			ReadProcessMemory(process_list.__HandleProcess, (PBYTE*)(process_list.__dwordClient + dwEntityList + (i * 0x10)), &entity, sizeof(DWORD), 0);
			ReadProcessMemory(process_list.__HandleProcess, (PBYTE*)(entity + m_iTeamNum), &entityTeam, sizeof(int), 0);
			ReadProcessMemory(process_list.__HandleProcess, (PBYTE*)(entity + m_iGlowIndex), &glowIndex, sizeof(int), 0);

			if (entity != NULL && entityTeam != _player.iTeam) //Find Enemy
			{
				//Show outlines
				WriteProcessMemory(process_list.__HandleProcess, (PBYTE*)(glowObj + (glowIndex * 0x38) + 0x4), &full, sizeof(float), 0); //red
				//WriteProcessMemory(process_list.__HandleProcess, (PBYTE*)(glowObj + (glowIndex * 0x38) + 0x8), 0, sizeof(float), 0); //green
				//WriteProcessMemory(process_list.__HandleProcess, (PBYTE*)(glowObj + (glowIndex * 0x38) + 0xC), 0, sizeof(float), 0); //blue
				WriteProcessMemory(process_list.__HandleProcess, (PBYTE*)(glowObj + (glowIndex * 0x38) + 0x10), &alpha, sizeof(float), 0); //alpha
			}
			/*
			else if (entity != NULL && entityTeam == myPlayer.iTeam)
			{
				//Show outlines
				WriteProcessMemory(process_list.__HandleProcess, (PBYTE*)(glowObj + (glowIndex * 0x38) + 0x4), 0, sizeof(float), 0); //red
				WriteProcessMemory(process_list.__HandleProcess, (PBYTE*)(glowObj + (glowIndex * 0x38) + 0x8), 0, sizeof(float), 0); //green
				WriteProcessMemory(process_list.__HandleProcess, (PBYTE*)(glowObj + (glowIndex * 0x38) + 0xC), &two, sizeof(float), 0); //blue
				WriteProcessMemory(process_list.__HandleProcess, (PBYTE*)(glowObj + (glowIndex * 0x38) + 0x10), &alpha, sizeof(float), 0); //alpha
			}
			*/
			WriteProcessMemory(process_list.__HandleProcess, (PBYTE*)(glowObj + (glowIndex * 0x38) + 0x24), &bOccluded, sizeof(bool), 0);
			WriteProcessMemory(process_list.__HandleProcess, (PBYTE*)(glowObj + (glowIndex * 0x38) + 0x25), &bUnoccluded, sizeof(bool), 0);
			//need to find style offset somewhere near 0x44, 0x48
			//WriteProcessMemory(process_list.__HandleProcess, (PBYTE*)(glowObj + (glowIndex * 0x38) + 0x48), &glowStyle, sizeof(int), 0); 
		}
	}

	void checker_defuse()
	{
		DWORD entity = 0x0;
		int entityTeam = 0; //actually EntityTeam
		bool defusing = 0;
		bool hasKit = 0;

		for (int i = 0; i < 64; i++)
		{
			ReadProcessMemory(process_list.__HandleProcess, (PBYTE*)(process_list.__dwordClient + dwEntityList + (i * 0x10)), &entity, sizeof(DWORD), 0);
			ReadProcessMemory(process_list.__HandleProcess, (PBYTE*)(entity + m_iTeamNum), &entityTeam, sizeof(int), 0);
			ReadProcessMemory(process_list.__HandleProcess, (PBYTE*)(entity + m_bIsDefusing), &defusing, sizeof(bool), 0);
			ReadProcessMemory(process_list.__HandleProcess, (PBYTE*)(entity + m_bHasDefuser), &hasKit, sizeof(bool), 0);
			if (entity != NULL && entityTeam != _player.iTeam)
			{
				if (defusing && hasKit)
				{
					Beep(550, 70); //lower interval if has defuserkit and higher freq
				}
				if (defusing && !hasKit)
				{
					Beep(350, 120);
				}
			}
		}
	}

	void flash_hack()
	{
		float newAlphaFlash = .2f; //old val .2f

		if (config::anti_flash_enable)
		{
			if (_player.flash > .5f)
			{
				WriteProcessMemory(process_list.__HandleProcess, (PBYTE*)(_player.dwLocalP + m_flFlashMaxAlpha), &newAlphaFlash, sizeof(newAlphaFlash), 0);
			}
		}
	}

	void radar_hack()
	{
		if (config::radar_hack_enable)
		{
			DWORD entity = 0x0;
			bool bSpot = true;
			for (int i = 0; i < 64; i++)
			{
				//Radar
				ReadProcessMemory(process_list.__HandleProcess, (PBYTE*)(process_list.__dwordClient + dwEntityList + (i * 0x10)), &entity, sizeof(DWORD), 0);
				//Spot
				if (entity != NULL)
				{
					WriteProcessMemory(process_list.__HandleProcess, (PBYTE*)(entity + m_bSpotted), &bSpot, sizeof(bool), 0);
				}
			}
		}
	}
}

namespace chams
{
	void draw()
	{
		DWORD dwCham = 0x70;

		if (config::chams_enable)
		{
			DWORD entity = 0x0;
			bool hasKit = 0;
			int entityTeam = 0;
			int health = 0;

			byte rgbColor[3] = { 33, 103, 255 };
			byte rgbColorEnemy[3] = { 255, 25, 155 };
			byte rgbColorEnemyLow[3] = { 255,140,0 };
			byte rgbColorDefuser[3] = { 0, 0, 255 };
			byte rgbColorEnemyDefuser[3] = { 255, 0, 0 };
			float brightness = 15.f;
			DWORD thisPtr = (int)(process_list.__dwordEngine + model_ambient_min - 0x2c);
			DWORD xored = *(DWORD*)&brightness ^ thisPtr;

			for (int i = 0; i < 64; i++)
			{
				ReadProcessMemory(process_list.__HandleProcess, (PBYTE*)(process_list.__dwordClient + dwEntityList + (i * 0x10)), &entity, sizeof(DWORD), 0);
				ReadProcessMemory(process_list.__HandleProcess, (PBYTE*)(entity + m_iTeamNum), &entityTeam, sizeof(int), 0);
				ReadProcessMemory(process_list.__HandleProcess, (PBYTE*)(entity + m_bHasDefuser), &hasKit, sizeof(bool), 0);
				ReadProcessMemory(process_list.__HandleProcess, (PBYTE*)(entity + m_iHealth), &health, sizeof(bool), 0);
				if (entity != NULL && entityTeam != _player.iTeam)
				{
					//Defuse Boi red
					if (hasKit && health > 40)
					{
						WriteProcessMemory(process_list.__HandleProcess, (PBYTE*)(entity + dwCham), &rgbColorEnemyDefuser, sizeof(rgbColorEnemyDefuser), 0);
					}
					else if (hasKit && health <= 40)
					{
						WriteProcessMemory(process_list.__HandleProcess, (PBYTE*)(entity + dwCham), &rgbColorEnemyDefuser, sizeof(rgbColorEnemyDefuser), 0);
						Sleep(2);
						WriteProcessMemory(process_list.__HandleProcess, (PBYTE*)(entity + dwCham), &rgbColorEnemyLow, sizeof(rgbColorEnemyLow), 0);
						Sleep(2);
					}
					//Normal Boi pink
					else
					{
						if (health > 40)
						{
							WriteProcessMemory(process_list.__HandleProcess, (PBYTE*)(entity + dwCham), &rgbColorEnemy, sizeof(rgbColorEnemy), 0);
						}
						else
						{
							WriteProcessMemory(process_list.__HandleProcess, (PBYTE*)(entity + dwCham), &rgbColorEnemyLow, sizeof(rgbColorEnemyLow), 0);
						}
					}
				}
				else if (entity != NULL && entityTeam == _player.iTeam)
				{
					if (hasKit)
					{
						WriteProcessMemory(process_list.__HandleProcess, (PBYTE*)(entity + dwCham), &rgbColorDefuser, sizeof(rgbColorDefuser), 0);
					}
					else
					{
						WriteProcessMemory(process_list.__HandleProcess, (PBYTE*)(entity + dwCham), &rgbColor, sizeof(rgbColor), 0);
					}
				}
			}
			WriteProcessMemory(process_list.__HandleProcess, (PBYTE*)(process_list.__dwordEngine + model_ambient_min), &xored, sizeof(int), 0);
		}
		else
		{
			//reset
			DWORD entity = 0x0;

			int entityTeam = 0;

			byte rgb_color[3] = { 255, 255, 255 };

			byte rgb_color_enemy[3] = { 255, 255, 255 };

			float resetBrightness = 0.f;

			DWORD thisPtr = (int)(process_list.__dwordEngine + model_ambient_min - 0x2c);
			DWORD resetXored = *(DWORD*)&resetBrightness ^ thisPtr;
			for (int i = 0; i < 64; i++)
			{
				ReadProcessMemory(process_list.__HandleProcess, (PBYTE*)(process_list.__dwordClient + dwEntityList + (i * 0x10)), &entity, sizeof(DWORD), 0);
				ReadProcessMemory(process_list.__HandleProcess, (PBYTE*)(entity + m_iTeamNum), &entityTeam, sizeof(int), 0);
				if (entity != NULL && entityTeam != _player.iTeam)
				{
					WriteProcessMemory(process_list.__HandleProcess, (PBYTE*)(entity + dwCham), &rgb_color_enemy, sizeof(rgb_color_enemy), 0);
				}
				else if (entity != NULL && entityTeam == _player.iTeam)
				{
					WriteProcessMemory(process_list.__HandleProcess, (PBYTE*)(entity + dwCham), &rgb_color, sizeof(rgb_color), 0);
				}
			}
			WriteProcessMemory(process_list.__HandleProcess, (PBYTE*)(process_list.__dwordEngine + model_ambient_min), &resetXored, sizeof(int), 0);
		}
	}

}

// attach hacks

class Cheat
{
public:
	int attach()
	{

		process_list._run_processes();

		std::cout << "" << std::endl;
		std::cout << " [NUM1] Anti-Flash" << std::endl;
		std::cout << " [NUM2] Radar Hack" << std::endl;
		std::cout << " [NUM3] Draw Chams" << std::endl;
		std::cout << " [NUM4] TriggerBot // key l_alt holding to work trigger" << std::endl;
		std::cout << " [NUM5] Check Defusing // when the bomb is defusing, a sound is made" << std::endl;
		std::cout << " [NUM6] Glow" << std::endl;
		std::cout << " [NUM7] Bunnyhop" << std::endl;
		std::cout << " [NUM8] Sound esp // when you hover over the model, a sound is made" << std::endl;

		std::cout << "" << std::endl;
		std::cout << " [F11] Panic key // unload cheat" << std::endl;

		while (!GetAsyncKeyState(VK_F11))
		{

			_player._read_info(); // read all strings.

			if (GetAsyncKeyState(VK_NUMPAD1))
			{
				config::anti_flash_enable = !config::anti_flash_enable;
				if (config::anti_flash_enable == false) {
					Beep(250, 200); }
				else {
					Beep(400, 200);
				}
			}
			if (GetAsyncKeyState(VK_NUMPAD2))
			{
				config::radar_hack_enable = !config::radar_hack_enable;
				if (config::radar_hack_enable == false) {
					Beep(250, 200); }
				else {
					Beep(400, 200);
				}
			}
			if (GetAsyncKeyState(VK_NUMPAD3))
			{
				config::chams_enable = !config::chams_enable;
				if (config::chams_enable == false) {
					Beep(250, 200); }
				else {
					Beep(400, 200);
				}
			}
			if (GetAsyncKeyState(VK_NUMPAD4))
			{
				config::triggerbot_enable = !config::triggerbot_enable;
				if (config::triggerbot_enable == false) {
					Beep(250, 200); }
				else {
					Beep(400, 200);
				}
			}
			if (GetAsyncKeyState(VK_NUMPAD5))
			{
				config::checkdefuse_enable = !config::checkdefuse_enable;
				if (config::checkdefuse_enable == false) {
					Beep(250, 200); }
				else {
					Beep(400, 200);
				}
			}
			if (GetAsyncKeyState(VK_NUMPAD6))
			{
				config::bWall_enable = !config::bWall_enable;
				if (config::bWall_enable == false) {
					Beep(250, 200); }
				else {
					Beep(400, 200);
				}
			}
			if (GetAsyncKeyState(VK_NUMPAD7))
			{
				config::bhop_enable = !config::bhop_enable;
				if (config::bhop_enable == false) {
					Beep(250, 200); }
				else {
					Beep(400, 200);
				}
			}
			if (GetAsyncKeyState(VK_NUMPAD8))
			{
				config::sound_esp_enable = !config::sound_esp_enable;
				if (config::sound_esp_enable == false) {
					Beep(250, 200);
				}
				else {
					Beep(400, 200);
				}
			}

			if (config::triggerbot_enable == true) {
				if (GetAsyncKeyState(VK_LMENU)) {  //l alt key
					triggerbot::enable();
				}
			}
			if (config::anti_flash_enable == true) {
				CSGOutils::flash_hack();
			}
			if (config::radar_hack_enable == true) {
				CSGOutils::radar_hack();
			}
			if (config::checkdefuse_enable == true) {
				CSGOutils::checker_defuse();
			}
			if (config::bWall_enable == true) {
				CSGOutils::glow_in_wall();
			}
			if (config::bhop_enable == true) {
				bhop::enable();
			}
			if (config::sound_esp_enable == true) {
				sound_esp::target();
			}
			chams::draw();
			Sleep(1);
		} _return 0;
	} 

	Cheat();
	~Cheat();
};
