#pragma once

#include <ntddk.h>
#include "common.h"
#include "cpuid.h"
#include "vmcb.h"
#include "msr.h"
#include "regs.h"
#include "paging.h"
#include "svmtraps.h"


#define MSR_TSC				0x10
#define	MSR_EFER			0xc0000080
#define	MSR_FS_BASE			0xc0000100
#define	MSR_GS_BASE			0xc0000101
#define MSR_LSTAR			0xC0000082
#define	MSR_SHADOW_GS_BASE	0xc0000102
#define	MSR_VM_HSAVE_PA		0xC0010117

#define	SVM_VMCB_SIZE_IN_PAGES	1
#define	SVM_HSA_SIZE_IN_PAGES	1
#define	SVM_MSRPM_SIZE_IN_PAGES	2

#define	EFER_SVME	(1<<12)


typedef struct _SVM {
	PHYSICAL_ADDRESS	VmcbToContinuePA;	// MUST go first in the structure; refer to SvmVmrun() for details
	PVMCB		_2mbVmcbMap;


	BOOLEAN	bGuestSVME;						// contains the virtualized value of guest's SVME bit
	PHYSICAL_ADDRESS	GuestHsaPA;			// contains the virtualized value of guest's MSR VM_HSAVE_PA
	PHYSICAL_ADDRESS	GuestVmcbPA;		// PA of VMCB of the guest hypervisor's guest (last one)



	PHYSICAL_ADDRESS	OriginalVmcbPA;
	PVMCB	OriginalVmcb;					// VMCB which was originally built by the BP for the guest OS


	PUCHAR	OriginalMsrPm;
	PHYSICAL_ADDRESS	OriginalMsrPmPA;

	PVMCB	NestedVmcb;						// points to the patched copy of the VMCB which was built by the 
											// guest hypervisor. We patch general{1|2}_intercepts to trap VMRUNs 
											// as well as EFER and VM_HSAVE_PA rw (by patching msrpm_base_pa).
	PHYSICAL_ADDRESS	NestedVmcbPA;

	PUCHAR	NestedMsrPm;					// any nested MsrPm must be patched to intercept EFER and VM_HSAVE_PA rw.
	PHYSICAL_ADDRESS	NestedMsrPmPA;		// We copy guest's MsrPm here and patch it.
	// we don't have to have NestedIoPm because we don't intercept any io

	PVOID	Hsa;
	PHYSICAL_ADDRESS	HsaPA;

	UCHAR	GuestStateBeforeInterrupt[0xc00];

} SVM, *PSVM;

#include "hvm.h"

extern ULONG	g_uSubvertedCPUs;

NTSTATUS NTAPI SvmEnable(
);

NTSTATUS NTAPI SvmDisable(
);

VOID NTAPI SvmSetHsa(
			PHYSICAL_ADDRESS HsaPA
);

// pure asm stuff

VOID NTAPI SvmWbinvd(
);


VOID NTAPI SvmVmsave(
			PHYSICAL_ADDRESS VmcbPA
);

VOID NTAPI SvmVmload(
			PHYSICAL_ADDRESS VmcbPA
);

VOID NTAPI SvmVmrun(
			PVOID HostStackBottom
);

NTSTATUS NTAPI SvmInterceptMsr(
			PUCHAR MsrPm,
			ULONG32 Msr,
			UCHAR bHowToIntercept,
			PUCHAR pOldInterceptType
);


NTSTATUS SvmInterceptEvent(
			PVMCB Vmcb,
			ULONG uVmExitNumber,
			BOOLEAN bInterceptState,
			PBOOLEAN pOldInterceptState
);

NTSTATUS NTAPI SvmSetupGeneralInterceptions(
			PCPU Cpu,
			PVMCB Vmcb
);

NTSTATUS NTAPI SvmSetupMsrInterceptions(
			PCPU Cpu,
			PUCHAR MsrPm
);

NTSTATUS NTAPI SvmInjectEvent(
			PVMCB Vmcb,
			UCHAR bVector,
			UCHAR bType,
			UCHAR bEv,
			ULONG32 uErrCode
);


static BOOLEAN NTAPI SvmIsNestedEvent(
			PCPU Cpu,
			PGUEST_REGS GuestRegs
);

static VOID NTAPI SvmDispatchEvent(
			PCPU Cpu,
			PGUEST_REGS GuestRegs
);

static VOID NTAPI SvmDispatchNestedEvent(
			PCPU Cpu,
			PGUEST_REGS GuestRegs
);

static BOOLEAN NTAPI SvmIsImplemented(
);

static VOID NTAPI SvmAdjustRip(
			PCPU Cpu,
			PGUEST_REGS GuestRegs,
			ULONG64 Delta
);

static NTSTATUS NTAPI SvmInitialize(
			PCPU Cpu,
			PVOID GuestRip,
			PVOID GuestRsp
);

static NTSTATUS NTAPI SvmShutdown(
			PCPU Cpu,
			PGUEST_REGS GuestRegs,
			BOOLEAN bSetupTimeBomb
);

static NTSTATUS NTAPI SvmVirtualize(
			PCPU Cpu
);
