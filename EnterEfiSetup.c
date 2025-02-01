#define PHNT_VERSION PHNT_WIN8
#include <phnt_windows.h>
#include <evntrace.h>
#include <phnt.h>

#include <stdio.h>

#pragma comment(lib, "NTDLL.lib")


/* https://uefi.org/specs/UEFI/2.10/ */

#define EFI_GLOBAL_VARIABLE \
 {0x8BE4DF61,0x93CA,0x11D2,\
   {0xAA,0x0D,0x00,0xE0,0x98,0x03,0x2B,0x8C}}
#define EFI_OS_INDICATIONS_BOOT_TO_FW_UI 0x0000000000000001


#define NtPrintA(h, iosb, s) \
NtWriteFile(h, NULL, NULL, NULL, iosb, s, (ULONG)strlen(s), 0, NULL)

int main() {
	GUID globalvar = EFI_GLOBAL_VARIABLE;
	UNICODE_STRING varname1 = RTL_CONSTANT_STRING(L"OsIndications");
	UNICODE_STRING varname2 = RTL_CONSTANT_STRING(L"OsIndicationsSupported");

	HANDLE nt_stdout = NtCurrentPeb()->ProcessParameters->StandardOutput;
	NTSTATUS status;
	UINT64 indic = 0;
	ULONG varlen = sizeof(UINT64);
	BOOLEAN r;
	ULONG attr;
	IO_STATUS_BLOCK iosb;

	LARGE_INTEGER li = {
		.QuadPart = -20'000'000
	};

	status = RtlAdjustPrivilege(SE_SYSTEM_ENVIRONMENT_PRIVILEGE, TRUE, FALSE, &r);
	if (status) {
		printf("Cannot enable SeSystemEnvironmentPrivilege: 0x%08X\n", status);
		goto exit;
	}

	status = NtQuerySystemEnvironmentValueEx(&varname2, &globalvar, &indic, &varlen, NULL);
	if (status) {
		printf("Cannot query OsIndicationsSupported: 0x%08X\n", status);
		goto exit;
	}

	if (indic & EFI_OS_INDICATIONS_BOOT_TO_FW_UI) {
	} else {
		NtPrintA(nt_stdout, &iosb, "The system does not support booting to EFI settings.\n");
		goto exit;
	}

	varlen = sizeof(UINT64);
	status = NtQuerySystemEnvironmentValueEx(&varname1, &globalvar, &indic, &varlen, NULL);
	if (status) {
		indic = 0;
	}
	indic |= EFI_OS_INDICATIONS_BOOT_TO_FW_UI;
	attr = EFI_VARIABLE_NON_VOLATILE |
		EFI_VARIABLE_BOOTSERVICE_ACCESS |
		EFI_VARIABLE_RUNTIME_ACCESS;
	status = NtSetSystemEnvironmentValueEx(&varname1, &globalvar, &indic, sizeof(UINT64), attr);
	if (status) {
		printf("Cannot set OsIndications: 0x%08X\n", status);
		goto exit;
	}

	NtPrintA(nt_stdout, &iosb, "The system will enter EFI setup on next boot.\n");

exit:
	NtDelayExecution(FALSE, &li);
	return 0;
}
