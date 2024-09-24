#pragma once

#include <ntddk.h>
#include "common.h"
#include "cpuid.h"


NTSTATUS NTAPI McInitCloak(
			PVOID ImageStart,
			ULONG ImageSize
);

NTSTATUS NTAPI McCloak(
);

NTSTATUS NTAPI McShutdownCloak(
);