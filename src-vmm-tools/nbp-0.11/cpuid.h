#pragma once

#include <ntddk.h>

VOID NTAPI GetCpuIdInfo(
			ULONG32 fn, 
			OUT PULONG32 ret_eax, 
			OUT PULONG32 ret_ebx, 
			OUT PULONG32 ret_ecx,
			OUT PULONG32 ret_edx
);
