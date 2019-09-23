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
        if (GetModuleBaseName(hProcess, hMods[i], szModName, sizeof(szModName) / sizeof(char)))
        {
            if (strcmp(szModuleName, szModName) == 0)
            {
                return hMods[i];
            }
        }
    }
    return NULL;
}

void readDword(DWORD* dst, char* prompt)
{
    *dst = 0;
    do
    {
        printf(prompt);
        scanf("%x", dst);
    } while (!*dst);
    printf("You entered: 0x%p\n", *dst);
}

#include "csgo.h"

int main()
{
    DWORD dwForceJump = 0x51AD5A8;
    //readDword(&dwForceJump, "Enter the ForceJump offset (in hex): client_panorama.dll+0x");

	DWORD dwLocalPlayer = 0x4D09EF4;
    //readDword(&dwLocalPlayer, "Enter the LocalPlayer offset (in hex): client_panorama.dll+0x");

    DWORD dw_m_fFlags = 0x104;
    //readDword(&dw_m_fFlags, "Enter the m_fFlags offset (in hex): LocalPlayer+0x");

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

    // Calculate the addresses.
    DWORD dwClientBase = (DWORD)hClient;
    LPVOID pForceJump = (LPVOID)(dwClientBase + dwForceJump);
    LPVOID pLocalPlayer = (LPVOID)(dwClientBase + dwLocalPlayer);
    printf("client_panorama.dll = 0x%p\n", dwClientBase);
    printf("ForceJump =   0x%p\n", pForceJump);
    printf("LocalPlayer = 0x%p\n", pLocalPlayer);

    printf("If your offset was correct, holding Left Alt while in-game will make your player bunnyhop.\n");
    printf("For best results, use sv_enablebunnyhopping 1 and sv_airaccelerate 100 :)\n");
    printf("Press F6 to exit.\n");

    // Our main hack loop.
    while (!GetAsyncKeyState(VK_F6)) // F6 quits our hack program.
    {
        // Don't do anything if the CS:GO window isn't focused.
        if (GetForegroundWindow() != hWnd)
        {
            Sleep(100);
            continue;
        }

        // GetAsyncKeyState returns nonzero if the key is pressed.
        if (GetAsyncKeyState(VK_LMENU)) // Left Alt
        {
            DWORD dwLocalPlayer;
            ReadProcessMemory(hProcess, pLocalPlayer, &dwLocalPlayer, sizeof(dwLocalPlayer), NULL);
            if (dwLocalPlayer)
            {
                int flags;
                ReadProcessMemory(hProcess, (LPVOID)(dwLocalPlayer + dw_m_fFlags), &flags, sizeof(flags), NULL);
				printf("%x\n", flags);
                if (flags & FL_ONGROUND)
                {
                    BYTE data;
                    ReadProcessMemory(hProcess, pForceJump, &data, sizeof(data), NULL);
                    data |= 1;
                    WriteProcessMemory(hProcess, pForceJump, &data, sizeof(data), NULL);
                }
                else
                {
                    BYTE data;
                    ReadProcessMemory(hProcess, pForceJump, &data, sizeof(data), NULL);
                    data &= ~1;
                    WriteProcessMemory(hProcess, pForceJump, &data, sizeof(data), NULL);
                }
            }
            else
            {
                // LocalPlayer is null, we're probably not in-game.
                Sleep(100);
            }
        }
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

