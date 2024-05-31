#include <iostream>
#include <sstream>
#include <Windows.h>
#include "Remap.h"

EXTERN_C NTSTATUS NTAPI NtSuspendProcess(HANDLE);
EXTERN_C NTSTATUS NTAPI NtResumeProcess(HANDLE);

BOOL StartProcess(std::string szFilePath, std::string szFile, std::string szCommandLine, PROCESS_INFORMATION& processInfo)
{
	STARTUPINFO info = { sizeof(info) };

	std::string szWorkingDirectory(szFilePath.begin(), szFilePath.end());
	std::string strFileName(szFile.begin(), szFile.end());

	std::ostringstream fileArgs;
	fileArgs << szWorkingDirectory.c_str() << "\\" << strFileName.c_str();

	std::string file = fileArgs.str();

	std::ostringstream cmdArgs;
	cmdArgs << "\\" << szWorkingDirectory.c_str() << "\\" << strFileName.c_str() << "\\";
	cmdArgs << " ";
	cmdArgs << szCommandLine.c_str();

	std::string cmd = cmdArgs.str();

	SECURITY_ATTRIBUTES securityInfo = { sizeof(securityInfo) };

	securityInfo.bInheritHandle = FALSE;

	BOOL result = CreateProcess(&file[0], &cmd[0], &securityInfo, NULL, FALSE, 0, NULL, &szWorkingDirectory[0], &info, &processInfo);

	if (!result)
		return FALSE;

	return TRUE;
}

inline static DWORD Read4Byte(HANDLE hProcess, DWORD dwAddress)
{
	DWORD nValue = 0;
	ReadProcessMemory(hProcess, (LPVOID)dwAddress, &nValue, 4, 0);
	return nValue;
}

int main()
{
	PROCESS_INFORMATION clientProcessInfo;

	std::ostringstream szCommandLine;

	szCommandLine << GetCurrentProcessId();

	if (!StartProcess("C:\\NTTGame\\KnightOnlineEn", "KnightOnLine.exe", szCommandLine.str(), clientProcessInfo))
	{
		return 0;
	}

	while (Read4Byte(clientProcessInfo.hProcess, 0x00E706F7) == 0)
		continue;

	NtSuspendProcess(clientProcessInfo.hProcess);

	Remap::PatchSection(clientProcessInfo.hProcess, (LPVOID*)0x00400000, 0x00B20000, PAGE_EXECUTE_READWRITE);
	Remap::PatchSection(clientProcessInfo.hProcess, (LPVOID*)0x00F20000, 0x00010000, PAGE_EXECUTE_READWRITE);
	Remap::PatchSection(clientProcessInfo.hProcess, (LPVOID*)0x00F30000, 0x00130000, PAGE_EXECUTE_READWRITE);

	BYTE byPatch[] = { 0xE9, 0x79, 0x06, 0x00, 0x00, 0x90 };
	WriteProcessMemory(clientProcessInfo.hProcess, (LPVOID*)0x00E706F7, byPatch, sizeof(byPatch), 0);

	NtResumeProcess(clientProcessInfo.hProcess);

	return 0;
}

