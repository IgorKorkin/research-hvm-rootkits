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

#include "hypercalls.h"





VOID NTAPI HcDispatchHypercall(PCPU Cpu,PGUEST_REGS GuestRegs)
{

#ifndef ENABLE_HYPERCALLS

	return;

#else

	ULONG32	HypercallNumber;
	ULONG32	HypercallResult=0;


	if (!Cpu || !GuestRegs)
		return;

	HypercallNumber=(ULONG32)(GuestRegs->rdx & 0xffff);

	switch (HypercallNumber) {
	case NBP_HYPERCALL_UNLOAD:

		_KdPrint(("HcDispatchHypercall(): NBP_HYPERCALL_UNLOAD\n"));

		GuestRegs->rcx=NBP_MAGIC;
		GuestRegs->rdx=HypercallResult;

		Hvm->ArchAdjustRip(Cpu,GuestRegs,2);

		// disable virtualization, resume guest, don't setup time bomb
		Hvm->ArchShutdown(Cpu,GuestRegs,FALSE);

		// never returns

		_KdPrint(("HcDispatchHypercall(): ArchShutdown() returned\n"));

		break;

#ifdef VPC_STUFF
	case NBP_HYPERCALL_QUEUE_INTERRUPT:

		_KdPrint(("HcDispatchHypercall(): NBP_HYPERCALL_QUEUE_INTERRUPT, number 0x%x\n",GuestRegs->r8));

		if (Cpu->uInterruptQueueSize<MAX_INTERRUPT_QUEUE_SIZE) {
			Cpu->InterruptQueue[Cpu->uInterruptQueueSize]=(UCHAR)GuestRegs->r8;
			Cpu->uInterruptQueueSize++;
		}

		break;

	case NBP_HYPERCALL_EOI:

		_KdPrint(("HcDispatchHypercall(): NBP_HYPERCALL_QUEUE_EOI, number 0x%x\n",GuestRegs->r8));

		memcpy(&((PUCHAR)Cpu->Svm.OriginalVmcb)[0x400],&Cpu->Svm.GuestStateBeforeInterrupt,0xc00);
		// it will be adjusted by two by efer trap caller
		Cpu->Svm.OriginalVmcb->rip-=2;

		return;
#endif

	default:

		_KdPrint(("HcDispatchHypercall(): Unsupported hypercall 0x%04X\n",HypercallNumber));
		break;
	}

	GuestRegs->rcx=NBP_MAGIC;
	GuestRegs->rdx=HypercallResult;

#endif
}

NTSTATUS NTAPI HcMakeHypercall(
					ULONG32 HypercallNumber,
					ULONG32 HypercallParameter,
					PULONG32 pHypercallResult)
{

#ifndef ENABLE_HYPERCALLS

	return STATUS_NOT_SUPPORTED;

#else

	ULONG32	eax,edx=HypercallParameter,ecx=MSR_EFER;



	// low part contains a hypercall number
	edx=HypercallNumber | (NBP_MAGIC & 0xffff0000);

	MsrReadWithEaxEdx(&ecx,&eax,&edx);

	if (ecx!=NBP_MAGIC) {
		_KdPrint(("HcMakeHypercall(): No NewBluePill detected on the processor #%d\n",KeGetCurrentProcessorNumber()));
		return STATUS_NOT_SUPPORTED;
	}

	if (pHypercallResult)
		*pHypercallResult=edx;

	return STATUS_SUCCESS;
#endif
}
