#include "Shellcode.h"

void MyEntry() {
	APIINTERFACE ai;

	DWORD szGetProcAddr = 0x0afdf8b8;
	DWORD szLoadLibraryA = 0x0148be54;
	char szUser32[] = { 'U','s','e','r','3','2','\0' };
	DWORD szMessageBoxA = 0xdbbe8dc3;

	HMODULE hKernel32 = GetKernel32Base();
	ai.pfnLoadLibrary = (FN_LoadLibraryA)MyGetProcAddress(hKernel32, szLoadLibraryA);
	ai.pfnGetProcAddress = (FN_GetProcAddress)MyGetProcAddress(hKernel32, szGetProcAddr);

	HMODULE hUser32 = ai.pfnLoadLibrary(szUser32);
	ai.pfnMessageBoxA = (FN_MessageBoxA)MyGetProcAddress(hUser32, szMessageBoxA);

	char szHello[] = { 'H','e','l','l','o','\0' };
	char szTitle[] = { 't','i','t','\0' };
	ai.pfnMessageBoxA(NULL,szTitle,szHello,MB_OK);
}

//��ȡ��������hash
DWORD GetProcHash(char* lpProcName) {
	char* p = lpProcName;
	DWORD result = 0;
	do {
		result = (result << 7) - result;
		result += *p;
	} while (*p++ );

	return result;
}

//�Ƚ��ַ���
BOOL MyStrcmp(DWORD str1, char* str2) {

	if (str1 == GetProcHash(str2)) {
		return 0;
	}
}

//��ȡkernel32��ַ
HMODULE GetKernel32Base() {
	HMODULE hKer32 = NULL;
	_asm {
		mov eax, fs: [0x18] ;//�ҵ�teb
		mov eax, [eax + 0x30];//peb
		mov eax, [eax + 0x0c];//PEB_LDR_DATA
		mov eax, [eax + 0x0c];//LIST_ENTRY ��ģ��
		mov eax, [eax];//ntdll
		mov eax, [eax];//kernel32
		mov eax, dword ptr[eax + 0x18];//kernel32��ַ
		mov hKer32, eax
	}
	return hKer32;
}

//��ú�����ַ
DWORD MyGetProcAddress(HMODULE hModule, DWORD lpProcName) {
	DWORD dwProcAddress = 0;
	PIMAGE_DOS_HEADER pDosHdr = (PIMAGE_DOS_HEADER)hModule;
	PIMAGE_NT_HEADERS pNtHdr = (PIMAGE_NT_HEADERS)((DWORD)pDosHdr + pDosHdr->e_lfanew);
	//��ȡ������
	PIMAGE_EXPORT_DIRECTORY pExtTbl = (PIMAGE_EXPORT_DIRECTORY)((DWORD)pDosHdr + pNtHdr->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress);

	//����������
	//��ȡ������ַ����
	PDWORD pAddressOfFunc = (PDWORD)((DWORD)hModule + pExtTbl->AddressOfFunctions);
	//��ȡ��������
	PDWORD pAddressOfName = (PDWORD)((DWORD)hModule + pExtTbl->AddressOfNames);
	//��ȡ��ŵ�����
	PWORD pAddressOfNameOrdinal = (PWORD)((DWORD)hModule + pExtTbl->AddressOfNameOrdinals);

	//�ж���Ż��ַ�������
	if ((DWORD)lpProcName & 0xffff0000)
	{
		//ͨ�����ƻ�ȡ������ַ
		DWORD dwSize = pExtTbl->NumberOfNames;
		for (DWORD i = 0; i < dwSize; i++)
		{
			//��ȡ�����ַ���
			DWORD dwAddrssOfName = (DWORD)hModule + pAddressOfName[i];
			//�ж�����
			int nRet = MyStrcmp(lpProcName, (char*)dwAddrssOfName);
			if (nRet == 0)
			{
				//����һ����ͨ��������ű������
				WORD wHint = pAddressOfNameOrdinal[i];
				//������Ż�ú�����ַ
				dwProcAddress = (DWORD)hModule + pAddressOfFunc[wHint];
				return dwProcAddress;
			}
		}
		//�Ҳ������ַΪ0
		dwProcAddress = 0;
	}
	else
	{
		//ͨ����Ż�ȡ������ַ
		DWORD nId = (DWORD)lpProcName - pExtTbl->Base;
		dwProcAddress = (DWORD)hModule + pAddressOfFunc[nId];
	}
	return dwProcAddress;
}

