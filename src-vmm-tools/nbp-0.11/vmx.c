/* 
 * Copyright holder: Invisible Things Lab
 * 
 * This software is protected by domestic and International
 * copyright laws. Any use (including publishing and
 * distribution) of this software requires a valid license
 * from the copyright holder.
 *
 * This software is provided for the educational use only
 * during the Black Hat training. This software should not
 * be used on production systems.
 *
 */

#include "vmx.h"


HVM_DEPENDENT	Vmx={
	ARCH_VMX,
	VmxIsImplemented,
	VmxInitialize,
	VmxVirtualize,
	VmxShutdown,
	VmxIsNestedEvent,
	VmxDispatchNestedEvent,
	VmxDispatchEvent,
	VmxAdjustRip,
	VmxRegisterTraps
};


static BOOLEAN NTAPI VmxIsImplemented()
{
	return FALSE;
}


static BOOLEAN NTAPI VmxIsNestedEvent(PCPU Cpu,PGUEST_REGS GuestRegs)
{
	return FALSE;
}


static VOID NTAPI VmxDispatchEvent(PCPU Cpu,PGUEST_REGS GuestRegs) 
{
	return;
}


static VOID NTAPI VmxDispatchNestedEvent(PCPU Cpu,PGUEST_REGS GuestRegs)
{
	return;
}

static VOID NTAPI VmxAdjustRip(PCPU Cpu,PGUEST_REGS GuestRegs,ULONG64 Delta)
{
	return;
}

static NTSTATUS NTAPI VmxRegisterTraps(PCPU Cpu)
{
	return STATUS_NOT_IMPLEMENTED;
}

static NTSTATUS NTAPI VmxInitialize(PCPU Cpu,PVOID GuestRip,PVOID GuestRsp)
{
	return STATUS_NOT_IMPLEMENTED;
}

static NTSTATUS NTAPI VmxShutdown(PCPU Cpu,PGUEST_REGS GuestRegs,BOOLEAN bSetupTimeBomb)
{
	return STATUS_NOT_IMPLEMENTED;
}

static NTSTATUS NTAPI VmxVirtualize(PCPU Cpu)
{
	return STATUS_NOT_IMPLEMENTED;
}
