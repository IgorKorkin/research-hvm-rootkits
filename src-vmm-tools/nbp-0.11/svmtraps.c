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

#include "svmtraps.h"



extern ULONG	g_uPrintStuff;



static BOOLEAN NTAPI SvmDispatchVmrun(PCPU Cpu,PGUEST_REGS GuestRegs,PNBP_TRAP Trap)
{
	PVMCB	Vmcb;
	NTSTATUS	Status;
	PHYSICAL_ADDRESS	GuestMsrPmPA;


	if (!Cpu || !GuestRegs)
		return TRUE;

	Vmcb=Cpu->Svm.OriginalVmcb;
/*
	_KdPrint(("SvmDispatchVmrun(): VMRUN intercepted, VMCB PA: 0x%p\n",
		Vmcb->rax));
	_KdPrint(("SvmDispatchVmrun(): VMRUN RIP: 0x%p\n",
		Vmcb->rip));
*/

/*
	_KdPrint(("SvmDispatchVmrun(): GS_BASE: 0x%p\n",MsrRead(MSR_GS_BASE)));
	_KdPrint(("SvmDispatchVmrun(): SHADOW_GS_BASE: 0x%p\n",MsrRead(MSR_SHADOW_GS_BASE)));
	_KdPrint(("SvmDispatchVmrun(): KernGSBase: 0x%p\n",Vmcb->kerngsbase));
	_KdPrint(("SvmDispatchVmrun(): efer: 0x%p\n",Vmcb->efer));
	_KdPrint(("SvmDispatchVmrun(): fs.base: 0x%p\n",Vmcb->fs.base));
	_KdPrint(("SvmDispatchVmrun(): gs.base: 0x%p\n",Vmcb->gs.base));
	_KdPrint(("SvmDispatchVmrun(): cr2: 0x%p\n",Vmcb->cr2));
	_KdPrint(("SvmDispatchVmrun(): original ss.base: 0x%p\n",Cpu->Svm.OriginalVmcb->ss.base));
	_KdPrint(("SvmDispatchVmrun(): original rsp: 0x%p\n",Cpu->Svm.OriginalVmcb->rsp));

	_KdPrint(("SvmDispatchVmrun(): original es.sel: 0x%x, base: 0x%x, limit 0x%x\n",Cpu->Svm.OriginalVmcb->es.sel,Cpu->Svm.OriginalVmcb->es.base,Cpu->Svm.OriginalVmcb->es.limit));
	_KdPrint(("SvmDispatchVmrun(): original cs.sel: 0x%x, base: 0x%x, limit 0x%x\n",Cpu->Svm.OriginalVmcb->cs.sel,Cpu->Svm.OriginalVmcb->cs.base,Cpu->Svm.OriginalVmcb->cs.limit));
	_KdPrint(("SvmDispatchVmrun(): original ss.sel: 0x%x, base: 0x%x, limit 0x%x\n",Cpu->Svm.OriginalVmcb->ss.sel,Cpu->Svm.OriginalVmcb->ss.base,Cpu->Svm.OriginalVmcb->ss.limit));
	_KdPrint(("SvmDispatchVmrun(): original ds.sel: 0x%x, base: 0x%x, limit 0x%x\n",Cpu->Svm.OriginalVmcb->ds.sel,Cpu->Svm.OriginalVmcb->ds.base,Cpu->Svm.OriginalVmcb->ds.limit));
	_KdPrint(("SvmDispatchVmrun(): original fs.sel: 0x%x, base: 0x%x, limit 0x%x\n",Cpu->Svm.OriginalVmcb->fs.sel,Cpu->Svm.OriginalVmcb->fs.base,Cpu->Svm.OriginalVmcb->fs.limit));
	_KdPrint(("SvmDispatchVmrun(): original gs.sel: 0x%x, base: 0x%x, limit 0x%x\n",Cpu->Svm.OriginalVmcb->gs.sel,Cpu->Svm.OriginalVmcb->gs.base,Cpu->Svm.OriginalVmcb->gs.limit));

	_KdPrint(("SvmDispatchVmrun(): original gdtr.sel: 0x%x, base: 0x%x, limit 0x%x\n",Cpu->Svm.OriginalVmcb->gdtr.sel,Cpu->Svm.OriginalVmcb->gdtr.base,Cpu->Svm.OriginalVmcb->gdtr.limit));
	_KdPrint(("SvmDispatchVmrun(): original ldtr.sel: 0x%x, base: 0x%x, limit 0x%x\n",Cpu->Svm.OriginalVmcb->ldtr.sel,Cpu->Svm.OriginalVmcb->ldtr.base,Cpu->Svm.OriginalVmcb->ldtr.limit));
	_KdPrint(("SvmDispatchVmrun(): original idtr.sel: 0x%x, base: 0x%x, limit 0x%x\n",Cpu->Svm.OriginalVmcb->idtr.sel,Cpu->Svm.OriginalVmcb->idtr.base,Cpu->Svm.OriginalVmcb->idtr.limit));
	_KdPrint(("SvmDispatchVmrun(): original tr.sel: 0x%x, base: 0x%x, limit 0x%x\n",Cpu->Svm.OriginalVmcb->tr.sel,Cpu->Svm.OriginalVmcb->tr.base,Cpu->Svm.OriginalVmcb->tr.limit));
*/

	if (!Cpu->Svm.bGuestSVME) {
		_KdPrint(("SvmDispatchVmrun(): Guest hasn't turned on SVME bit, injecting #UD\n"));

		SvmInjectEvent(Vmcb,EV_INVALID_OPCODE,GE_EXCEPTION,FALSE,0);

		return FALSE;
	}

	// rax should be 4k-aligned already.
	// Save it so we can set guest hypervisor's rax to "real" VMCB PA to handle guest's #VMEXIT
	Cpu->Svm.GuestVmcbPA.QuadPart=Vmcb->rax;

	// copy & patch the guest hypervisor's guest VMCB

	if (!NT_SUCCESS(Status=HvmCopyPhysicalToVirtual(
							Cpu,
							Cpu->Svm.NestedVmcb,
							Cpu->Svm.GuestVmcbPA,
							SVM_VMCB_SIZE_IN_PAGES))) {

		_KdPrint(("SvmDispatchVmrun(): Failed to read guest VMCB, status 0x%08hX\n",Status));

		// continue the nested VM
		Cpu->Svm.VmcbToContinuePA.QuadPart=Vmcb->rax;
		return TRUE;
	}

	// Check only VMRUN interception; skip all other consistency checks mentioned in AMD man. 15.5, p. 360-361 -
	// all other bogus conditions will be checked by the CPU itself on our VMRUN, we'll just pass VMEXIT_INVALID
	// to the guest if something goes wrong.
	if (!(Cpu->Svm.NestedVmcb->general2_intercepts & 1)) {
		_KdPrint(("SvmDispatchVmrun(): Guest doesn't want to intercept VMRUN, returning VMEXIT_INVALID\n"));

		Vmcb->exitcode=VMEXIT_INVALID;
		return TRUE;
	}


	// apply all our traps to the guest vmcb
	if (!NT_SUCCESS(Status=SvmSetupGeneralInterceptions(Cpu,Cpu->Svm.NestedVmcb))) {
		_KdPrint(("SvmDispatchVmrun(): *** SvmSetupGeneralInterceptions() failed with status 0x%08hX, continuing ***\n",Status));
	}

	// copy & patch the guest MSRPM

	GuestMsrPmPA.QuadPart=Cpu->Svm.NestedVmcb->msrpm_base_pa;
//	_KdPrint(("SvmDispatchVmrun(): Guest MSRPM PA: 0x%X\n",
//		GuestMsrPmPA.QuadPart));


	if (GuestMsrPmPA.QuadPart) {
		// guest has specified a MsrPm, patch it to intercept all what we need

		if (!NT_SUCCESS(Status=HvmCopyPhysicalToVirtual(
			Cpu,
			Cpu->Svm.NestedMsrPm,
			GuestMsrPmPA,
			SVM_MSRPM_SIZE_IN_PAGES))) {

			_KdPrint(("SvmDispatchVmrun(): Failed to read guest MSRPM, status 0x%08hX\n",Status));

			// continue the nested VM
			Cpu->Svm.VmcbToContinuePA.QuadPart=Vmcb->rax;
			return TRUE;
		}

	} else {
		// guest hypervisor doesn't want to intercept any MSR rw, but we have to.
		// Indicate we want to trap nothing else but EFER and VM_HSAVE_PA rw.
		RtlZeroMemory(Cpu->Svm.NestedMsrPm,SVM_MSRPM_SIZE_IN_PAGES*PAGE_SIZE);
	}

	// apply all our traps to the guest msrpm
	if (!NT_SUCCESS(Status=SvmSetupMsrInterceptions(Cpu,Cpu->Svm.NestedMsrPm))) {
		_KdPrint(("SvmDispatchVmrun(): *** VmcbSetupMsrInterceptions() failed with status 0x%08hX, continuing ***\n",Status));
	}

	Cpu->Svm.NestedVmcb->msrpm_base_pa=Cpu->Svm.NestedMsrPmPA.QuadPart;

//	_KdPrint(("SvmDispatchVmrun(): Guest guest_asid: %d, TLB_CONTROL %d\n",Cpu->Svm.NestedVmcb->guest_asid,Cpu->Svm.NestedVmcb->tlb_control));

	Cpu->Svm.NestedVmcb->guest_asid++;
	Cpu->Svm.NestedVmcb->tlb_control=1;

	// continue the nested VM
	Cpu->Svm.VmcbToContinuePA.QuadPart=Cpu->Svm.NestedVmcbPA.QuadPart;
/*

	_KdPrint(("SvmDispatchVmrun(): Continuing nested VM at PA 0x%X\n",
		Cpu->Svm.VmcbToContinuePA.QuadPart));
	_KdPrint(("SvmDispatchVmrun(): First RIP: 0x%p\n",
		Cpu->Svm.NestedVmcb->rip));
	_KdPrint(("SvmDispatchVmrun(): Nested efer: 0x%p\n",Cpu->Svm.NestedVmcb->efer));
	_KdPrint(("SvmDispatchVmrun(): Nested kerngsbase: 0x%p\n",Cpu->Svm.NestedVmcb->kerngsbase));
	_KdPrint(("SvmDispatchVmrun(): Nested fs.base: 0x%p\n",Cpu->Svm.NestedVmcb->fs.base));
	_KdPrint(("SvmDispatchVmrun(): Nested gs.base: 0x%p\n",Cpu->Svm.NestedVmcb->gs.base));
	_KdPrint(("SvmDispatchVmrun(): Nested cr2: 0x%p\n",Cpu->Svm.NestedVmcb->cr2));
*/

	return TRUE;
}


#define TRAP_page_fault       14
#define TRAP_gp_fault         13

static BOOLEAN NTAPI SvmDispatchGP(PCPU Cpu,PGUEST_REGS GuestRegs,PNBP_TRAP Trap)
{
	PVMCB	Vmcb;


	if (!Cpu || !GuestRegs)
		return TRUE;

	Vmcb=Cpu->Svm.OriginalVmcb;

	if (g_uPrintStuff) {

		_KdPrint(("SvmDispatchGP(): GP intercepted, RIP: 0x%p\n",
			Vmcb->rip));

		_KdPrint(("SvmDispatchGP(): GS_BASE: 0x%p\n",MsrRead(MSR_GS_BASE)));
		_KdPrint(("SvmDispatchGP(): SHADOW_GS_BASE: 0x%p\n",MsrRead(MSR_SHADOW_GS_BASE)));
		_KdPrint(("SvmDispatchGP(): KernGSBase: 0x%p\n",Vmcb->kerngsbase));
		_KdPrint(("SvmDispatchGP(): efer: 0x%p\n",Vmcb->efer));
		_KdPrint(("SvmDispatchGP(): fs.base: 0x%p\n",Vmcb->fs.base));
		_KdPrint(("SvmDispatchGP(): gs.base: 0x%p\n",Vmcb->gs.base));
		_KdPrint(("SvmDispatchGP(): cr2: 0x%p\n",Vmcb->cr2));



		_KdPrint(("SvmDispatchGP(): original es.sel: 0x%x, base: 0x%x, limit 0x%x\n",Cpu->Svm.OriginalVmcb->es.sel,Cpu->Svm.OriginalVmcb->es.base,Cpu->Svm.OriginalVmcb->es.limit));
		_KdPrint(("SvmDispatchGP(): original cs.sel: 0x%x, base: 0x%x, limit 0x%x\n",Cpu->Svm.OriginalVmcb->cs.sel,Cpu->Svm.OriginalVmcb->cs.base,Cpu->Svm.OriginalVmcb->cs.limit));
		_KdPrint(("SvmDispatchGP(): original ss.sel: 0x%x, base: 0x%x, limit 0x%x\n",Cpu->Svm.OriginalVmcb->ss.sel,Cpu->Svm.OriginalVmcb->ss.base,Cpu->Svm.OriginalVmcb->ss.limit));
		_KdPrint(("SvmDispatchGP(): original ds.sel: 0x%x, base: 0x%x, limit 0x%x\n",Cpu->Svm.OriginalVmcb->ds.sel,Cpu->Svm.OriginalVmcb->ds.base,Cpu->Svm.OriginalVmcb->ds.limit));
		_KdPrint(("SvmDispatchGP(): original fs.sel: 0x%x, base: 0x%x, limit 0x%x\n",Cpu->Svm.OriginalVmcb->fs.sel,Cpu->Svm.OriginalVmcb->fs.base,Cpu->Svm.OriginalVmcb->fs.limit));
		_KdPrint(("SvmDispatchGP(): original gs.sel: 0x%x, base: 0x%x, limit 0x%x\n",Cpu->Svm.OriginalVmcb->gs.sel,Cpu->Svm.OriginalVmcb->gs.base,Cpu->Svm.OriginalVmcb->gs.limit));


		_KdPrint(("SvmDispatchGP(): original gdtr.sel: 0x%x, base: 0x%x, limit 0x%x\n",Cpu->Svm.OriginalVmcb->gdtr.sel,Cpu->Svm.OriginalVmcb->gdtr.base,Cpu->Svm.OriginalVmcb->gdtr.limit));
		_KdPrint(("SvmDispatchGP(): original ldtr.sel: 0x%x, base: 0x%x, limit 0x%x\n",Cpu->Svm.OriginalVmcb->ldtr.sel,Cpu->Svm.OriginalVmcb->ldtr.base,Cpu->Svm.OriginalVmcb->ldtr.limit));
		_KdPrint(("SvmDispatchGP(): original idtr.sel: 0x%x, base: 0x%x, limit 0x%x\n",Cpu->Svm.OriginalVmcb->idtr.sel,Cpu->Svm.OriginalVmcb->idtr.base,Cpu->Svm.OriginalVmcb->idtr.limit));
		_KdPrint(("SvmDispatchGP(): original tr.sel: 0x%x, base: 0x%x, limit 0x%x\n",Cpu->Svm.OriginalVmcb->tr.sel,Cpu->Svm.OriginalVmcb->tr.base,Cpu->Svm.OriginalVmcb->tr.limit));


		_KdPrint(("SvmDispatchGP(): original cr4: 0x%x\n",Cpu->Svm.OriginalVmcb->cr4));

	}


	SvmInjectEvent(Vmcb,TRAP_gp_fault,EVENTTYPE_EXCEPTION,1,(ULONG32)Vmcb->exitinfo1);


	return FALSE;
}


static BOOLEAN NTAPI SvmDispatchDB(PCPU Cpu,PGUEST_REGS GuestRegs,PNBP_TRAP Trap)
{
	PVMCB	Vmcb;
	if (!Cpu || !GuestRegs)
		return TRUE;

	Vmcb=Cpu->Svm.OriginalVmcb;
#ifdef INTERCEPT_RDTSCs
	
	if (Vmcb->dr6 & 0x40) {
	/*
	 	_KdPrint(("SvmDispatchGP(): DB intercepted, RIP: 0x%p\n",
			Vmcb->rip));

	*/
		Cpu->EmulatedCycles += 6;	// TODO: replace with f(Opcode)
		if (Cpu->Tracing-- <= 0) Vmcb->rflags ^= 0x100;	// disable TF

		Cpu->NoOfRecordedInstructions++;
		//TODO: add instruction opcode to Cpu->RecordedInstructions[]
		
	}	

#endif
	/*
	if (g_uPrintStuff) {

		_KdPrint(("SvmDispatchGP(): DB intercepted, RIP: 0x%p\n",
			Vmcb->rip));
	
		_KdPrint(("SvmDispatchGP(): GS_BASE: 0x%p\n",MsrRead(MSR_GS_BASE)));
		_KdPrint(("SvmDispatchGP(): SHADOW_GS_BASE: 0x%p\n",MsrRead(MSR_SHADOW_GS_BASE)));
		_KdPrint(("SvmDispatchGP(): KernGSBase: 0x%p\n",Vmcb->kerngsbase));
		_KdPrint(("SvmDispatchGP(): efer: 0x%p\n",Vmcb->efer));
		_KdPrint(("SvmDispatchGP(): fs.base: 0x%p\n",Vmcb->fs.base));
		_KdPrint(("SvmDispatchGP(): gs.base: 0x%p\n",Vmcb->gs.base));
		_KdPrint(("SvmDispatchGP(): cr2: 0x%p\n",Vmcb->cr2));
	}
	*/

	return FALSE;
}


static BOOLEAN NTAPI SvmDispatchPF(PCPU Cpu,PGUEST_REGS GuestRegs,PNBP_TRAP Trap)
{
	PVMCB	Vmcb;


	if (!Cpu || !GuestRegs)
		return TRUE;

	Vmcb=Cpu->Svm.OriginalVmcb;

	if (g_uPrintStuff) {

		_KdPrint(("SvmDispatchPF(): PF intercepted, RIP: 0x%p\n",
			Vmcb->rip));

		_KdPrint(("SvmDispatchPF(): GS_BASE: 0x%p\n",MsrRead(MSR_GS_BASE)));
		_KdPrint(("SvmDispatchPF(): SHADOW_GS_BASE: 0x%p\n",MsrRead(MSR_SHADOW_GS_BASE)));
		_KdPrint(("SvmDispatchPF(): KernGSBase: 0x%p\n",Vmcb->kerngsbase));
		_KdPrint(("SvmDispatchPF(): efer: 0x%p\n",Vmcb->efer));
		_KdPrint(("SvmDispatchPF(): fs.base: 0x%p\n",Vmcb->fs.base));
		_KdPrint(("SvmDispatchPF(): gs.base: 0x%p\n",Vmcb->gs.base));
		_KdPrint(("SvmDispatchPF(): cr2: 0x%p\n",Vmcb->cr2));

	}

	Vmcb->cr2=Vmcb->exitinfo2;
	SvmInjectEvent(Vmcb,TRAP_page_fault,EVENTTYPE_EXCEPTION,1,(ULONG32)Vmcb->exitinfo1);


	return FALSE;
}


static BOOLEAN NTAPI SvmDispatchBP(PCPU Cpu,PGUEST_REGS GuestRegs,PNBP_TRAP Trap)
{
	PVMCB	Vmcb;


	if (!Cpu || !GuestRegs)
		return TRUE;

	Vmcb=Cpu->Svm.OriginalVmcb;

	if (g_uPrintStuff) {

		_KdPrint(("SvmDispatchBP(): BP intercepted, RIP: 0x%p\n",
			Vmcb->rip));


		_KdPrint(("SvmDispatchBP(): GS_BASE: 0x%p\n",MsrRead(MSR_GS_BASE)));
		_KdPrint(("SvmDispatchBP(): SHADOW_GS_BASE: 0x%p\n",MsrRead(MSR_SHADOW_GS_BASE)));
		_KdPrint(("SvmDispatchBP(): KernGSBase: 0x%p\n",Vmcb->kerngsbase));
		_KdPrint(("SvmDispatchBP(): efer: 0x%p\n",Vmcb->efer));
		_KdPrint(("SvmDispatchBP(): fs.base: 0x%p\n",Vmcb->fs.base));
		_KdPrint(("SvmDispatchBP(): gs.base: 0x%p\n",Vmcb->gs.base));
		_KdPrint(("SvmDispatchBP(): cr2: 0x%p\n",Vmcb->cr2));

		_KdPrint(("SvmDispatchBP(): original es.sel: 0x%x, base: 0x%x, limit 0x%x\n",Cpu->Svm.OriginalVmcb->es.sel,Cpu->Svm.OriginalVmcb->es.base,Cpu->Svm.OriginalVmcb->es.limit));
		_KdPrint(("SvmDispatchBP(): original cs.sel: 0x%x, base: 0x%x, limit 0x%x\n",Cpu->Svm.OriginalVmcb->cs.sel,Cpu->Svm.OriginalVmcb->cs.base,Cpu->Svm.OriginalVmcb->cs.limit));
		_KdPrint(("SvmDispatchBP(): original ss.sel: 0x%x, base: 0x%x, limit 0x%x\n",Cpu->Svm.OriginalVmcb->ss.sel,Cpu->Svm.OriginalVmcb->ss.base,Cpu->Svm.OriginalVmcb->ss.limit));
		_KdPrint(("SvmDispatchBP(): original ds.sel: 0x%x, base: 0x%x, limit 0x%x\n",Cpu->Svm.OriginalVmcb->ds.sel,Cpu->Svm.OriginalVmcb->ds.base,Cpu->Svm.OriginalVmcb->ds.limit));
		_KdPrint(("SvmDispatchBP(): original fs.sel: 0x%x, base: 0x%x, limit 0x%x\n",Cpu->Svm.OriginalVmcb->fs.sel,Cpu->Svm.OriginalVmcb->fs.base,Cpu->Svm.OriginalVmcb->fs.limit));
		_KdPrint(("SvmDispatchBP(): original gs.sel: 0x%x, base: 0x%x, limit 0x%x\n",Cpu->Svm.OriginalVmcb->gs.sel,Cpu->Svm.OriginalVmcb->gs.base,Cpu->Svm.OriginalVmcb->gs.limit));


		_KdPrint(("SvmDispatchBP(): original gdtr.sel: 0x%x, base: 0x%x, limit 0x%x\n",Cpu->Svm.OriginalVmcb->gdtr.sel,Cpu->Svm.OriginalVmcb->gdtr.base,Cpu->Svm.OriginalVmcb->gdtr.limit));
		_KdPrint(("SvmDispatchBP(): original ldtr.sel: 0x%x, base: 0x%x, limit 0x%x\n",Cpu->Svm.OriginalVmcb->ldtr.sel,Cpu->Svm.OriginalVmcb->ldtr.base,Cpu->Svm.OriginalVmcb->ldtr.limit));
		_KdPrint(("SvmDispatchBP(): original idtr.sel: 0x%x, base: 0x%x, limit 0x%x\n",Cpu->Svm.OriginalVmcb->idtr.sel,Cpu->Svm.OriginalVmcb->idtr.base,Cpu->Svm.OriginalVmcb->idtr.limit));
		_KdPrint(("SvmDispatchBP(): original tr.sel: 0x%x, base: 0x%x, limit 0x%x\n",Cpu->Svm.OriginalVmcb->tr.sel,Cpu->Svm.OriginalVmcb->tr.base,Cpu->Svm.OriginalVmcb->tr.limit));


		_KdPrint(("SvmDispatchBP(): original cr4: 0x%x\n",Cpu->Svm.OriginalVmcb->cr4));

	}


	return FALSE;
}

#ifdef VPC_STUFF
static BOOLEAN NTAPI SvmDispatchClgi(PCPU Cpu,PGUEST_REGS GuestRegs,PNBP_TRAP Trap)
{
	PVMCB	Vmcb;


	if (!Cpu || !GuestRegs)
		return TRUE;

	Vmcb=Cpu->Svm.OriginalVmcb;

	if (Vmcb->idtr.base!=(ULONG64)Cpu->GuestIdtArea) {

		Cpu->OldGuestIdtBase=Vmcb->idtr.base;
		Cpu->OldGuestIdtLimit=Vmcb->idtr.limit;


		Vmcb->idtr.base=(ULONG64)Cpu->GuestIdtArea;
		Vmcb->idtr.limit=BP_IDT_LIMIT;

		_KdPrint(("CLGI: dummy IDTR set\n"));
	} else {
		_KdPrint(("CLGI: dummy IDTR NOT set\n"));
	}

	return TRUE;
}


static BOOLEAN NTAPI SvmDispatchStgi(PCPU Cpu,PGUEST_REGS GuestRegs,PNBP_TRAP Trap)
{
	PVMCB	Vmcb;


	if (!Cpu || !GuestRegs)
		return TRUE;

	Vmcb=Cpu->Svm.OriginalVmcb;

	if (Vmcb->idtr.base==(ULONG64)Cpu->GuestIdtArea) {

		Vmcb->idtr.base=Cpu->OldGuestIdtBase;
		Vmcb->idtr.limit=Cpu->OldGuestIdtLimit;

		_KdPrint(("STGI: IDTR restored\n"));
		_KdPrint(("STGI: rflags 0x%x\n",Vmcb->rflags));

	} else {
		_KdPrint(("STGI: IDTR NOT restored\n"));
	}


	return TRUE;
}
#endif


#ifdef BP_KNOCK
static BOOLEAN NTAPI SvmDispatchCpuid(PCPU Cpu,PGUEST_REGS GuestRegs,PNBP_TRAP Trap) {
	PVMCB	Vmcb;
	ULONG32 fn;
	if (!Cpu || !GuestRegs)
		return TRUE;

	Vmcb=Cpu->Svm.OriginalVmcb;
//	_KdPrint(("SvmDispatchCpuid(): CPUID intercepted, RIP: 0x%p, RAX: 0x%p\n",
//		Vmcb->rip, Vmcb->rax));

	if ((Vmcb->rax & 0xffffffff) == BP_KNOCK_EAX) {
		_KdPrint (("Magic knock received: %p\n", BP_KNOCK_EAX));
		Vmcb->rax = BP_KNOCK_EAX_ANSWER;
	} else {
		fn = (ULONG32)Vmcb->rax;
		GetCpuIdInfo (fn, 
				&(ULONG32)Vmcb->rax, 
				&(ULONG32)GuestRegs->rbx,
				&(ULONG32)GuestRegs->rcx,
				&(ULONG32)GuestRegs->rdx);
	}

	return TRUE;
}
#endif

static BOOLEAN NTAPI SvmDispatchRdtsc(PCPU Cpu,PGUEST_REGS GuestRegs,PNBP_TRAP Trap)
{
	PVMCB	Vmcb;
	ULONG64 Tsc;


	if (!Cpu || !GuestRegs)
		return TRUE;

	Vmcb=Cpu->Svm.OriginalVmcb;
	// WARNING: Do not uncomment KdPrint's -- it will freeze the system due to interference with OS secheduling!
#ifdef INTERCEPT_RDTSCs
/*
	_KdPrint(("SvmDispatchRdtscp(): RDTSCP intercepted, RIP: 0x%p\n",
		Vmcb->rip));
*/	
	if (Cpu->Tracing > 0) {
		Cpu->Tsc = Cpu->EmulatedCycles + Cpu->LastTsc;
	} else {
		Cpu->Tsc = RegGetTSC();
	}
/*
	_KdPrint((" Tracing = %d, LastTsc = %p, EmulatedCycles = %p, Tsc = %p\n",
				Cpu->Tracing, Cpu->LastTsc, Cpu->EmulatedCycles, Cpu->Tsc));
*/
	Cpu->LastTsc = Cpu->Tsc;
	Cpu->EmulatedCycles = 0;
	Cpu->NoOfRecordedInstructions = 0;
	Cpu->Tracing = INSTR_TRACE_MAX;

	GuestRegs->rdx = (Cpu->Tsc >> 32);
	Vmcb->rax = (Cpu->Tsc & 0xffffffff);
	Vmcb->rflags |= 0x100;	// set TF

#else
	/*
	_KdPrint(("SvmDispatchRdtsc(): RDTSC intercepted, RIP: 0x%p\n",
		Vmcb->rip));
*/

	Tsc = RegGetTSC();
	GuestRegs->rdx = (Tsc >> 32);
	Vmcb->rax = (Tsc & 0xffffffff);
#endif
	return TRUE;
}


static BOOLEAN NTAPI SvmDispatchRdtscp(PCPU Cpu,PGUEST_REGS GuestRegs,PNBP_TRAP Trap)
{
	PVMCB	Vmcb;
	if (!Cpu || !GuestRegs)
		return TRUE;

	Vmcb=Cpu->Svm.OriginalVmcb;

#ifdef INTERCEPT_RDTSCs

	/*
	_KdPrint(("SvmDispatchRdtscp(): RDTSCP intercepted, RIP: 0x%p\n",
		Vmcb->rip));
	*/
	if (Cpu->Tracing > 0) {
		Cpu->Tsc = Cpu->EmulatedCycles + Cpu->LastTsc;
	} else {
		Cpu->Tsc = RegGetTSC();
	}
	/*
	_KdPrint((" Tracing = %d, LastTsc = %p, EmulatedCycles = %p, Tsc = %p\n",
				Cpu->Tracing, Cpu->LastTsc, Cpu->EmulatedCycles, Cpu->Tsc));
	*/
	Cpu->LastTsc = Cpu->Tsc;
	Cpu->EmulatedCycles = 0;
	Cpu->NoOfRecordedInstructions = 0;
	Cpu->Tracing = INSTR_TRACE_MAX;

	GuestRegs->rdx = (Cpu->Tsc >> 32);
	Vmcb->rax = (Cpu->Tsc & 0xffffffff);
	Vmcb->rflags |= 0x100;	// set TF
	// FIXME: load guests's ECX with TSC_AUX!
#endif



	return TRUE;
}

static BOOLEAN NTAPI SvmDispatchMsrTscRead(PCPU Cpu,PGUEST_REGS GuestRegs,PNBP_TRAP Trap)
{
	PVMCB	Vmcb;
	ULONG32	eax,edx;

	if (!Cpu || !GuestRegs)
		return TRUE;
#ifdef INTERCEPT_RDTSCs
	Vmcb=Cpu->Svm.OriginalVmcb;
	/*
	_KdPrint(("SvmDispatchMsrTscRead(): RDMSR 10h intercepted, RIP: 0x%p\n",
		Vmcb->rip));
	*/
	if (Cpu->Tracing > 0) {
		Cpu->Tsc = Cpu->EmulatedCycles + Cpu->LastTsc;
	} else {
		Cpu->Tsc = RegGetTSC();
	}
	/*
	_KdPrint((" Tracing = %d, LastTsc = %p, EmulatedCycles = %p, Tsc = %p\n",
				Cpu->Tracing, Cpu->LastTsc, Cpu->EmulatedCycles, Cpu->Tsc));
	*/
	Cpu->LastTsc = Cpu->Tsc;
	Cpu->EmulatedCycles = 0;
	Cpu->NoOfRecordedInstructions = 0;
	Cpu->Tracing = INSTR_TRACE_MAX;

	GuestRegs->rdx = (Cpu->Tsc >> 32);
	Vmcb->rax = (Cpu->Tsc & 0xffffffff);
	Vmcb->rflags |= 0x100;	// set TF
#endif
	return TRUE;
}


static BOOLEAN NTAPI SvmDispatchVmsave(PCPU Cpu,PGUEST_REGS GuestRegs,PNBP_TRAP Trap)
{
	PVMCB	Vmcb;


	if (!Cpu || !GuestRegs)
		return TRUE;

	Vmcb=Cpu->Svm.OriginalVmcb;



	_KdPrint(("SvmDispatchVmsave(): VMSAVE intercepted, RIP: 0x%p\n",
		Vmcb->rip));

	_KdPrint(("SvmDispatchVmsave(): GS_BASE: 0x%p\n",MsrRead(MSR_GS_BASE)));
	_KdPrint(("SvmDispatchVmsave(): SHADOW_GS_BASE: 0x%p\n",MsrRead(MSR_SHADOW_GS_BASE)));
	_KdPrint(("SvmDispatchVmsave(): KernGSBase: 0x%p\n",Vmcb->kerngsbase));
	_KdPrint(("SvmDispatchVmsave(): efer: 0x%p\n",Vmcb->efer));
	_KdPrint(("SvmDispatchVmsave(): fs.base: 0x%p\n",Vmcb->fs.base));
	_KdPrint(("SvmDispatchVmsave(): gs.base: 0x%p\n",Vmcb->gs.base));
	_KdPrint(("SvmDispatchVmsave(): cr2: 0x%p\n",Vmcb->cr2));


	TrDeregisterTrap(Trap);

	return FALSE;
}

static BOOLEAN NTAPI SvmDispatchVmload(PCPU Cpu,PGUEST_REGS GuestRegs,PNBP_TRAP Trap)
{
	PVMCB	Vmcb;
	PULONG64	p1,p2;


	if (!Cpu || !GuestRegs)
		return TRUE;

	Vmcb=Cpu->Svm.OriginalVmcb;



	_KdPrint(("SvmDispatchVmload(): VMLOAD intercepted, RIP: 0x%p\n",
		Vmcb->rip));

//	_KdPrint(("SvmDispatchVmload(): GS_BASE: 0x%p\n",MsrRead(MSR_GS_BASE)));
//	_KdPrint(("SvmDispatchVmload(): SHADOW_GS_BASE: 0x%p\n",MsrRead(MSR_SHADOW_GS_BASE)));
	_KdPrint(("SvmDispatchVmload(): KernGSBase: 0x%p\n",Vmcb->kerngsbase));
	_KdPrint(("SvmDispatchVmload(): efer: 0x%p\n",Vmcb->efer));
	_KdPrint(("SvmDispatchVmload(): fs.base: 0x%p\n",Vmcb->fs.base));
	_KdPrint(("SvmDispatchVmload(): gs.base: 0x%p\n",Vmcb->gs.base));
	_KdPrint(("SvmDispatchVmload(): cr2: 0x%p\n",Vmcb->cr2));
	_KdPrint(("SvmDispatchVmload(): rax: 0x%p\n",Vmcb->rax));
	_KdPrint(("SvmDispatchVmload(): VINTR: 0x%p\n",Vmcb->vintr.UCHARs));
	_KdPrint(("SvmDispatchVmload(): cpl: 0x%p\n",Vmcb->cpl));



	_KdPrint(("SvmDispatchVmload(): original es.sel: 0x%x, base: 0x%x, limit 0x%x\n",Cpu->Svm.OriginalVmcb->es.sel,Cpu->Svm.OriginalVmcb->es.base,Cpu->Svm.OriginalVmcb->es.limit));
	_KdPrint(("SvmDispatchVmload(): original cs.sel: 0x%x, base: 0x%x, limit 0x%x\n",Cpu->Svm.OriginalVmcb->cs.sel,Cpu->Svm.OriginalVmcb->cs.base,Cpu->Svm.OriginalVmcb->cs.limit));
	_KdPrint(("SvmDispatchVmload(): original ss.sel: 0x%x, base: 0x%x, limit 0x%x\n",Cpu->Svm.OriginalVmcb->ss.sel,Cpu->Svm.OriginalVmcb->ss.base,Cpu->Svm.OriginalVmcb->ss.limit));
	_KdPrint(("SvmDispatchVmload(): original ds.sel: 0x%x, base: 0x%x, limit 0x%x\n",Cpu->Svm.OriginalVmcb->ds.sel,Cpu->Svm.OriginalVmcb->ds.base,Cpu->Svm.OriginalVmcb->ds.limit));
	_KdPrint(("SvmDispatchVmload(): original fs.sel: 0x%x, base: 0x%x, limit 0x%x\n",Cpu->Svm.OriginalVmcb->fs.sel,Cpu->Svm.OriginalVmcb->fs.base,Cpu->Svm.OriginalVmcb->fs.limit));
	_KdPrint(("SvmDispatchVmload(): original gs.sel: 0x%x, base: 0x%x, limit 0x%x\n",Cpu->Svm.OriginalVmcb->gs.sel,Cpu->Svm.OriginalVmcb->gs.base,Cpu->Svm.OriginalVmcb->gs.limit));


	_KdPrint(("SvmDispatchVmload(): original gdtr.sel: 0x%x, base: 0x%x, limit 0x%x\n",Cpu->Svm.OriginalVmcb->gdtr.sel,Cpu->Svm.OriginalVmcb->gdtr.base,Cpu->Svm.OriginalVmcb->gdtr.limit));
	_KdPrint(("SvmDispatchVmload(): original ldtr.sel: 0x%x, base: 0x%x, limit 0x%x\n",Cpu->Svm.OriginalVmcb->ldtr.sel,Cpu->Svm.OriginalVmcb->ldtr.base,Cpu->Svm.OriginalVmcb->ldtr.limit));
	_KdPrint(("SvmDispatchVmload(): original idtr.sel: 0x%x, base: 0x%x, limit 0x%x\n",Cpu->Svm.OriginalVmcb->idtr.sel,Cpu->Svm.OriginalVmcb->idtr.base,Cpu->Svm.OriginalVmcb->idtr.limit));
	_KdPrint(("SvmDispatchVmload(): original tr.sel: 0x%x, base: 0x%x, limit 0x%x\n",Cpu->Svm.OriginalVmcb->tr.sel,Cpu->Svm.OriginalVmcb->tr.base,Cpu->Svm.OriginalVmcb->tr.limit));


	_KdPrint(("SvmDispatchVmload(): original cr4: 0x%x\n",Cpu->Svm.OriginalVmcb->cr4));

	g_uPrintStuff=10;


	return FALSE;
}



static BOOLEAN NTAPI SvmDispatchEFERAccess(PCPU Cpu,PGUEST_REGS GuestRegs,PNBP_TRAP Trap)
{
	PVMCB	Vmcb;
	ULONG32	eax,edx;
	LARGE_INTEGER	Efer;
	BOOLEAN	bWriteAccess,bIsSVMEOn,bTimeAttack;


	if (!Cpu || !GuestRegs)
		return TRUE;


	Vmcb=Cpu->Svm.OriginalVmcb;
	eax=(ULONG32)(Vmcb->rax & 0xffffffff);
	edx=(ULONG32)(GuestRegs->rdx & 0xffffffff);

	bWriteAccess=(BOOLEAN)(Vmcb->exitinfo1==(MSR_INTERCEPT_WRITE>>1));

	switch (bWriteAccess) {
	case FALSE:

		// it's a RDMSR(MSR_EFER)

		Efer.QuadPart=Vmcb->efer;

#ifdef ENABLE_HYPERCALLS
		if ((GuestRegs->rdx & 0xffff0000)==(NBP_MAGIC & 0xffff0000)) {
			HcDispatchHypercall(Cpu,GuestRegs);

			GuestRegs->rdx &= 0xffffffff00000000;;
			GuestRegs->rdx |= Efer.HighPart;
			break;
		}
#endif
		Vmcb->rax = Efer.LowPart;

		// clear SVME if it has not been set by the guest hypervisor
		if (!Cpu->Svm.bGuestSVME)
			Vmcb->rax&=~EFER_SVME;

		GuestRegs->rdx = Efer.HighPart;

		break;

	default:

		// it's a WRMSR(MSR_EFER)

		bIsSVMEOn=(BOOLEAN)((eax & EFER_SVME)!=0);

		Vmcb->efer=(((ULONG64)edx)<<32)+eax;
		Vmcb->efer|=EFER_SVME;

		if (Cpu->Svm.bGuestSVME!=bIsSVMEOn) {
			_KdPrint(("SvmDispatchEFERAccess(): EFER = 0x%X, turning guest SVME %s\n",
				Vmcb->rax & 0xffffffff,
				bIsSVMEOn ? "on" : "off"));
			Cpu->Svm.bGuestSVME=bIsSVMEOn;
		}


		break;
	}


	return TRUE;
}


static BOOLEAN NTAPI SvmDispatchVM_HSAVE_PAAccess(PCPU Cpu,PGUEST_REGS GuestRegs,PNBP_TRAP Trap)
{
	PVMCB	Vmcb;
	ULONG32	eax,edx;
	BOOLEAN	bWriteAccess;


	if (!Cpu || !GuestRegs)
		return TRUE;

	Vmcb=Cpu->Svm.OriginalVmcb;
	eax=(ULONG32)(Vmcb->rax & 0xffffffff);
	edx=(ULONG32)(GuestRegs->rdx & 0xffffffff);

	bWriteAccess=(BOOLEAN)(Vmcb->exitinfo1==(MSR_INTERCEPT_WRITE>>1));

	switch (bWriteAccess) {
	case FALSE:

		// it's a RDMSR(MSR_VM_HSAVE_PA)

		Vmcb->rax = Cpu->Svm.GuestHsaPA.LowPart;
		GuestRegs->rdx = Cpu->Svm.GuestHsaPA.HighPart;

		break;

	default:

		// it's a WRMSR(MSR_VM_HSAVE_PA)

		Cpu->Svm.GuestHsaPA.QuadPart=((ULONG64)edx<<32)+eax;

		_KdPrint(("SvmDispatchVM_HSAVE_PAAccess(): VM_HSAVE_PA = 0x%X, virtualizing it\n",
			Cpu->Svm.GuestHsaPA.QuadPart));

		// don't allow guest to modify real VM_HSAVE_PA
		break;
	}

	return TRUE;
}



NTSTATUS NTAPI SvmRegisterTraps(PCPU Cpu)
{
	NTSTATUS	Status;
	PNBP_TRAP	Trap;


	if (!NT_SUCCESS(Status=TrInitializeGeneralTrap(
							Cpu,
							VMEXIT_VMRUN,
							3,					// length of the VMRUN instruction
							SvmDispatchVmrun,
							&Trap))) {
		_KdPrint(("SvmRegisterTraps(): Failed to register SvmDispatchVmrun with status 0x%08hX\n",Status));
		return Status;
	}
	TrRegisterTrap(Cpu,Trap);


/*

	if (!NT_SUCCESS(Status=TrInitializeGeneralTrap(
							Cpu,
							VMEXIT_EXCEPTION_BP,
							0,
							SvmDispatchBP,
							&Trap))) {
		_KdPrint(("SvmRegisterTraps(): Failed to register SvmDispatchBP with status 0x%08hX\n",Status));
		return Status;
	}
	TrRegisterTrap(Cpu,Trap);

	if (!NT_SUCCESS(Status=TrInitializeGeneralTrap(
							Cpu,
							VMEXIT_EXCEPTION_PF,
							0,
							SvmDispatchPF,
							&Trap))) {
		_KdPrint(("SvmRegisterTraps(): Failed to register SvmDispatchPF with status 0x%08hX\n",Status));
		return Status;
	}
	TrRegisterTrap(Cpu,Trap);


	if (!NT_SUCCESS(Status=TrInitializeGeneralTrap(
							Cpu,
							VMEXIT_EXCEPTION_GP,
							0,					// length of the VMRUN instruction
							SvmDispatchGP,
							&Trap))) {
		_KdPrint(("SvmRegisterTraps(): Failed to register SvmDispatchGP with status 0x%08hX\n",Status));
		return Status;
	}
	TrRegisterTrap(Cpu,Trap);

*/
/*
	if (!NT_SUCCESS(Status=TrInitializeGeneralTrap(
							Cpu,
							VMEXIT_VMSAVE,
							3,
							SvmDispatchVmsave,
							&Trap))) {
		_KdPrint(("SvmRegisterTraps(): Failed to register SvmDispatchVmsave with status 0x%08hX\n",Status));
		return Status;
	}
	TrRegisterTrap(Cpu,Trap);
*/

/*
	if (!NT_SUCCESS(Status=TrInitializeGeneralTrap(
							Cpu,
							VMEXIT_VMLOAD,
							0,
							SvmDispatchVmload,
							&Trap))) {
		_KdPrint(("SvmRegisterTraps(): Failed to register SvmDispatchVmload with status 0x%08hX\n",Status));
		return Status;
	}
	TrRegisterTrap(Cpu,Trap);
*/

#ifdef BP_KNOCK
	if (!NT_SUCCESS(Status=TrInitializeGeneralTrap(
							Cpu,
							VMEXIT_CPUID,
							2,
							SvmDispatchCpuid,
							&Trap))) {
		_KdPrint(("SvmRegisterTraps(): Failed to register SvmDispatchCpuid with status 0x%08hX\n",Status));
		return Status;
	}
	TrRegisterTrap(Cpu,Trap);
#endif

#ifdef INTERCEPT_RDTSCs 
	if (!NT_SUCCESS(Status=TrInitializeGeneralTrap(
							Cpu,
							VMEXIT_EXCEPTION_DB,
							0,
							SvmDispatchDB,
							&Trap))) {
		_KdPrint(("SvmRegisterTraps(): Failed to register SvmDispatchDB with status 0x%08hX\n",Status));
		return Status;
	}
	TrRegisterTrap(Cpu,Trap);

	if (!NT_SUCCESS(Status=TrInitializeGeneralTrap(
							Cpu,
							VMEXIT_RDTSC,
							2,					// length of the RDTSC instruction
							SvmDispatchRdtsc,
							&Trap))) {
		_KdPrint(("SvmRegisterTraps(): Failed to register SvmDispatchRdtsc with status 0x%08hX\n",Status));
		return Status;
	}
	TrRegisterTrap(Cpu,Trap);

	if (!NT_SUCCESS(Status=TrInitializeGeneralTrap(
							Cpu,
							VMEXIT_RDTSCP,
							3,					// length of the RDTSCP instruction
							SvmDispatchRdtscp,
							&Trap))) {
		_KdPrint(("SvmRegisterTraps(): Failed to register SvmDispatchRdtscp with status 0x%08hX\n",Status));
		return Status;
	}
	TrRegisterTrap(Cpu,Trap);

	if (!NT_SUCCESS(Status=TrInitializeMsrTrap(
							Cpu,
							MSR_TSC,
							MSR_INTERCEPT_READ,
							SvmDispatchMsrTscRead,
							&Trap))) {
		_KdPrint(("SvmRegisterTraps(): Failed to register SvmDispatchMsrTscRead with status 0x%08hX\n",Status));
		return Status;
	}
	TrRegisterTrap(Cpu,Trap);

#endif

#ifdef VPC_STUFF
	if (!NT_SUCCESS(Status=TrInitializeGeneralTrap(
							Cpu,
							VMEXIT_CLGI,
							3,					// length of the VMRUN instruction
							SvmDispatchClgi,
							&Trap))) {
		_KdPrint(("SvmRegisterTraps(): Failed to register SvmDispatchClgi with status 0x%08hX\n",Status));
		return Status;
	}
	TrRegisterTrap(Cpu,Trap);


	if (!NT_SUCCESS(Status=TrInitializeGeneralTrap(
							Cpu,
							VMEXIT_STGI,
							3,					// length of the VMRUN instruction
							SvmDispatchStgi,
							&Trap))) {
		_KdPrint(("SvmRegisterTraps(): Failed to register SvmDispatchStgi with status 0x%08hX\n",Status));
		return Status;
	}
	TrRegisterTrap(Cpu,Trap);
#endif


	if (!NT_SUCCESS(Status=TrInitializeMsrTrap(
							Cpu,
							MSR_EFER,
							MSR_INTERCEPT_READ | MSR_INTERCEPT_WRITE,
							SvmDispatchEFERAccess,
							&Trap))) {
		_KdPrint(("SvmRegisterTraps(): Failed to register SvmDispatchEFERAccess with status 0x%08hX\n",Status));
		return Status;
	}
	TrRegisterTrap(Cpu,Trap);


	if (!NT_SUCCESS(Status=TrInitializeMsrTrap(
							Cpu,
							MSR_VM_HSAVE_PA,
							MSR_INTERCEPT_READ | MSR_INTERCEPT_WRITE,
							SvmDispatchVM_HSAVE_PAAccess,
							&Trap))) {
		_KdPrint(("SvmRegisterTraps(): Failed to register SvmDispatchVM_HSAVE_PAAccess with status 0x%08hX\n",Status));
		return Status;
	}
	TrRegisterTrap(Cpu,Trap);

	return STATUS_SUCCESS;
}


