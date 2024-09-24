#pragma once

#include <ntddk.h>
#include "common.h"



typedef struct _VMX {
	PVOID	None;
} VMX, *PVMX;

#include "hvm.h"


static BOOLEAN NTAPI VmxIsImplemented(
);


static BOOLEAN NTAPI VmxIsNestedEvent(
			PCPU Cpu,
			PGUEST_REGS GuestRegs
);

static VOID NTAPI VmxDispatchEvent(
			PCPU Cpu,
			PGUEST_REGS GuestRegs
);

static VOID NTAPI VmxDispatchNestedEvent(
			PCPU Cpu,
			PGUEST_REGS GuestRegs
);

static VOID NTAPI VmxAdjustRip(
			PCPU Cpu,
			PGUEST_REGS GuestRegs,
			ULONG64 Delta
);

static NTSTATUS NTAPI VmxRegisterTraps(
			PCPU Cpu
);

static NTSTATUS NTAPI VmxInitialize(
			PCPU Cpu,
			PVOID GuestRip,
			PVOID GuestRsp
);

static NTSTATUS NTAPI VmxShutdown(
			PCPU Cpu,
			PGUEST_REGS GuestRegs,
			BOOLEAN bSetupTimeBomb
);

static NTSTATUS NTAPI VmxVirtualize(
			PCPU Cpu
);
