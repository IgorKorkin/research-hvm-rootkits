#pragma once

#include <ntddk.h>

ULONG64 NTAPI MsrRead(
			ULONG32 reg
);

VOID NTAPI MsrWrite(
			ULONG32 reg,
			ULONG64 MsrValue
);

NTSTATUS NTAPI MsrSafeWrite(
			ULONG32 reg,
			ULONG32 eax,
			ULONG32 edx
);

VOID NTAPI MsrReadWithEaxEdx(
			PULONG32 reg,	// ecx after rdmsr will be stored there
			PULONG32 eax,
			PULONG32 edx
);


