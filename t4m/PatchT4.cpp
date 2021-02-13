// ==========================================================
// T4M project
// 
// Component: clientdll
// Purpose: World at War patches
//
// Initial author: UNKNOWN
// Started: 2015-07-08
// ==========================================================

#include "StdInc.h"

void loadGameOverlay();
void PatchT4();
void PatchT4_MemoryLimits();
void PatchT4_Branding();
void PatchT4_Console();
void PatchT4_Dvars();
void PatchT4_Menus();
void PatchT4_NoBorder();
void PatchT4_PreLoad();
void PatchT4_Script();
void PatchT4_SteamDRM();
void PatchT4_FileDebug();
void PatchT4_Load();

void Sys_RunInit()
{
	PatchT4();
}

void PatchT4()
{
	//*(const char**)0x00840FF0 = "raw";
	
	// ↓ This causes server verification to fail?
	//PatchT4_SteamDRM();
	
	PatchT4_PreLoad();
	PatchT4_MemoryLimits();
	PatchT4_Branding();
	PatchT4_Console();
	PatchT4_Dvars();
	PatchT4_Menus();
	PatchT4_NoBorder();
	PatchT4_Script();
	PatchT4_Load();
	
	// check if game got started using steam
	if (!GetModuleHandle("gameoverlayrenderer.dll"))
		loadGameOverlay();
	
	//Change the title to shock     fuck it why not
	*(DWORD*)0x6D65A0 = (DWORD)"Call of Duty: World at War Campaign/Coop - T4M Swifty Editon";
}

void *MemCpyFix(void *a1, void **a2, int len)
{
	return memcpy(a1, a2, len);
}

void PatchT4_PreLoad()
{
	Detours::X86::DetourFunction((PBYTE)0x007AFFC0, (PBYTE)&MemCpyFix);
	nop(0x0059D6F4, 5); // disable Com_DvarDump from Com_Init_Try_Block_Function
	nop(0x005FF743, 5); // disable Sys_CreateSplash
	//nop(0x005FF698, 5); // disable Sys_CheckCrashOrRerun
	//nop(0x005FE685, 5); // disable Sys_HasConfigureChecksumChanged
}

void PatchT4_SteamDRM()
{
	// Replace encrypted .text segment
	DWORD size = 0x3EA000;
	std::string data = GetBinaryResource(IDB_TEXT);
	uncompress((unsigned char*)0x401000, &size, (unsigned char*)data.data(), data.size());

	// Apply new entry point
	HMODULE hModule = GetModuleHandle(NULL);
	PIMAGE_DOS_HEADER header = (PIMAGE_DOS_HEADER)hModule;
	PIMAGE_NT_HEADERS ntHeader = (PIMAGE_NT_HEADERS)((DWORD)hModule + header->e_lfanew);
	ntHeader->OptionalHeader.AddressOfEntryPoint = 0x3AF316;
}

void PatchT4_Menus()
{
	nop(0x437ACC, 5); // disable CG_CheckHudObjectiveDisplay call
	nop(0x6680D2, 2); // disable jmp for onlinegame dvar check
}

// code from https://github.com/momo5502/cod-mod/
void loadGameOverlay()
{
	try
	{
		std::string m_steamDir;
		HKEY hRegKey;

		if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, "Software\\Valve\\Steam", 0, KEY_QUERY_VALUE, &hRegKey) == ERROR_SUCCESS)
		{
			char pchSteamDir[MAX_PATH];
			DWORD dwLength = sizeof(pchSteamDir);
			RegQueryValueExA(hRegKey, "InstallPath", NULL, NULL, (BYTE*)pchSteamDir, &dwLength);
			RegCloseKey(hRegKey);

			m_steamDir = pchSteamDir;
		}
		// causes a stack overflow if left in
		//Com_Printf(0, "Loading %s\\gameoverlayrenderer.dll...\n", m_steamDir.c_str());
		HMODULE overlay = LoadLibrary(va("%s\\gameoverlayrenderer.dll", m_steamDir.c_str()));

		if (overlay)
		{
			FARPROC _SetNotificationPosition = GetProcAddress(overlay, "SetNotificationPosition");

			if (_SetNotificationPosition)
			{
				((void(*)(uint32_t))_SetNotificationPosition)(1);
			}
		}
	}
	catch (int e)
	{
		//Com_Printf(0, "Failed to inject Steam's gameoverlay: %d", e);
	}
}
