#include "stdafx.h"

#include <stdio.h>
#include <Windows.h>
#include <Psapi.h>

// Finds the base address of a module in another process' address space by it's name.
// (for example, client_panorama.dll)
HMODULE FindModule(HANDLE hProcess, const char* szModuleName)
{
	HMODULE hMods[1024];
	DWORD cbNeeded;
	if (!EnumProcessModules(hProcess, hMods, sizeof(hMods), &cbNeeded))
	{
		printf("Failed to enumerate process modules\n");
		return NULL;
	}
	for (unsigned int i = 0; i < cbNeeded / sizeof(HMODULE); i++)
	{
		char szModName[MAX_PATH];
		//printf("%d \n", i);
		if (GetModuleBaseName(hProcess, hMods[i], szModName, sizeof(szModName) / sizeof(char)))
		{
			//printf("%s \n",szModName);
			if (strcmp(szModuleName, szModName) == 0)
			{
				return hMods[i];
			}
		}
	}
	return NULL;
}

int main()
{
	DWORD dwForceAttack = 0x313B5B0;
	DWORD dwPlayerAddress = 0x51ABD30;
	DWORD dwCrosshairId = 0x0000B3AC;
	/*
	do
	{
		printf("Enter the ForceAttack offset (in hex): client_panorama.dll+0x");
		scanf("%x", &dwForceAttack);
	}
	while (!dwForceAttack);
	*/
	printf("You entered: 0x%p\n", dwForceAttack);
	
	// Find the CS:GO game window to get the game's process ID.
	// This is a hacky way to do this, but it works for most programs.
	printf("Waiting for CS:GO to start...\n");
	HWND hWnd = FindWindow(NULL, "Counter-Strike: Global Offensive");
	DWORD dwPid = 0;
	while (GetWindowThreadProcessId(hWnd, &dwPid), !dwPid)
	{
		Sleep(1000);
	}
	printf("CS:GO PID = %d\n", dwPid);

	// Open a handle to the game process.
	// This is necessary to interact with game's memory like in Cheat Engine.
	HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ | PROCESS_VM_WRITE, FALSE, dwPid);
	if (!hProcess)
	{
		printf("Failed to open handle to process, run as Admin!\n");
		goto exit;
	}

	// Find the base address of client_panorama.dll
	HMODULE hClient = FindModule(hProcess, "client_panorama.dll");
	if (!hClient)
	{
		printf("Failed to find client_panorama.dll!\n");
		goto exit;
	}

	// Calculate the address of ForceAttack relative to the base address.
	DWORD dwClientBase = (DWORD) hClient;
	LPVOID pForceAttack = (LPVOID) (dwClientBase + dwForceAttack);

	LPVOID pPlayerAddress = (LPVOID)(dwClientBase + dwPlayerAddress);
	DWORD dynamicPlayerAddress;
	ReadProcessMemory(hProcess, pPlayerAddress, &dynamicPlayerAddress, sizeof(dynamicPlayerAddress), NULL);

	LPVOID pCrossHairId = (LPVOID)(dynamicPlayerAddress + dwCrosshairId);

	printf("client_panorama.dll = 0x%p\n", dwClientBase);
	printf("ForceAttack = 0x%p\n", pForceAttack);

	printf("If your offset was correct, pressing Spacebar while in-game will make your player shoot.\n");
	printf("Press F6 to exit.\n");

	// Our main hack loop.
	// We keep track of whether the spacebar was held and
	// write to the game's memory when the spacebar is held or released.
	int wasDown = 0;
	while (!GetAsyncKeyState(VK_F6)) // F6 quits our hack program.
	{
		// Don't do anything if the CS:GO window isn't focused.
		if (GetForegroundWindow() != hWnd)
		{
			Sleep(100);
			continue;
		}
		BYTE crosshairId;
		ReadProcessMemory(hProcess, pCrossHairId, &crosshairId, sizeof(crosshairId), NULL);
		//printf("%d\n",crosshairId);
		if (crosshairId > 0 != wasDown) {
			// Set the first bit of ForceAttack's address to 1 if spacebar was pressed, and 0 if spacebar was released.
			// To do this, we first read the original value from the game's memory, set the bit, then write it back.
			BYTE data;
			// Read 1 byte from the game at ForceAttack's address into `data`
			ReadProcessMemory(hProcess, pForceAttack, &data, sizeof(data), NULL);
			// Set the bit
			data ^= (data & 1) ^ !!(crosshairId > 0);
			// Write 1 byte of `data` into the game at ForceAttack's address
			WriteProcessMemory(hProcess, pForceAttack, &data, sizeof(data), NULL);
			wasDown = true;
		}
		wasDown = crosshairId > 0;


		/*
		// GetAsyncKeyState returns nonzero if the key is pressed.
		int spaceDown = GetAsyncKeyState(VK_SPACE);
		if (spaceDown != wasDown)
		{
			// Set the first bit of ForceAttack's address to 1 if spacebar was pressed, and 0 if spacebar was released.
			// To do this, we first read the original value from the game's memory, set the bit, then write it back.
			BYTE data;
			// Read 1 byte from the game at ForceAttack's address into `data`
			ReadProcessMemory (hProcess, pForceAttack, &data, sizeof(data), NULL);
			// Set the bit
			data ^= (data & 1) ^ !!spaceDown;
			// Write 1 byte of `data` into the game at ForceAttack's address
			WriteProcessMemory(hProcess, pForceAttack, &data, sizeof(data), NULL);
		}
		wasDown = spaceDown;
		*/
		// When we have an update loop like this, it's important to Sleep each time to avoid wasting cpu time.
		Sleep(1);
	}
	printf("Exiting\n");

exit:
	// Close the handle to the game process.
	// In general, when you open a handle, you should close it when you're done.
	// This is not strictly necessary here, as all handles are closed by the operating system when our program exits.
	if (hProcess)
	{
		CloseHandle(hProcess);
	}
	_getch(); // read a character so the window doesn't close immediately
    return 0;
}

