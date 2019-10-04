#include <stdio.h>
#include <Windows.h>

int main() {

	// ~~ Stolen from ForceAttack ~~

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

	// ~~ End Stolen Code ~~

	HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, 0, dwPid);

	printf("I got the handle it is %d\n", hProcess);

	DWORD dwForceAttack = 0x3506B60C; // Address of "Force Attack"
	DWORD dwPlayerAddress = 0x370DBD80; // Address of player pointer
	DWORD dwCrosshairOffset = 0x0000B3AC; // Offset of crosshair ID

	LPVOID pForceAttack = (LPVOID)dwForceAttack;
	DWORD dynamicPlayerAddress;
	ReadProcessMemory(hProcess, (LPVOID)dwPlayerAddress, &dynamicPlayerAddress, 4, NULL);
	
	LPVOID pCrosshairId = (LPVOID)(dynamicPlayerAddress + dwCrosshairOffset);
	
	boolean pointing = false;

	printf("All setup! Press F6 to stop. \n")

	while (!GetAsyncKeyState(VK_F6)) { // F6 quits the program

		int crosshairId;
		ReadProcessMemory(hProcess, pCrosshairId, &crosshairId, 4, NULL);

		// Only change the value of ForceAttack if your "pointing" state has changed
		// Start shooting if you weren't pointing at something and you are now pointing at something
		// Stop shooting if you were pointing at something and you are now not pointing at something
		if (crosshairId > 0 != pointing) {
			// Write 5 to start shooting, 4 to stop.
			int shootValue = crosshairId > 0 ? 5 : 4;
			WriteProcessMemory(hProcess, pForceAttack, &shootValue, 4, NULL);
		}
		pointing = crosshairId > 0; // update the pointing state 

		Sleep(1); // So you don't waste cpu time

	}

	return 0;
}