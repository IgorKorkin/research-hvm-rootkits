#pragma once

#include <ntddk.h>
#include "paging.h"
#include "comprint.h"


#define	ENABLE_HYPERCALLS
#define	ENABLE_DEBUG_PRINTS
//#define	USE_2MB_PAGES
//#define	SET_PCD_BIT	// Set PCD for BP's pages (Non Cached)

// RDTSC cheating via instruction tracing and cycles emulation (only prototype)
//#define INTERCEPT_RDTSCs
#ifdef INTERCEPT_RDTSCs
#define INSTR_TRACE_MAX 128	// max no of instruction to trace
#endif

// cheat via VMCB.TSC_OFFSET (disable when intercepting RDTSCs)
//#define SIMPLE_RDTSC_CHEATING	

// Enable Blue Chicken strategy to survice external based timing
//#define BLUE_CHICKEN
#ifdef BLUE_CHICKEN
// BP will uninstall if CHICKEN_QUEUE_SZ intercepts
// occure within a period of time < CHICKEN_TSC_THRESHOLD
#define CHICKEN_QUEUE_SZ 1000
#define CHICKEN_TSC_THRESHOLD  100*1000000 //(100ms on a 1GHz processor)

#define	TIMEBOMB_COUNTDOWN	2000

#endif


//#define	ENABLE_MEMCLOAK


//#define	VPC_STUFF

#define	MAX_INTERRUPT_QUEUE_SIZE	128

// This is only for testing 
#define BP_KNOCK
#ifdef BP_KNOCK	
#define BP_KNOCK_EAX	0xbabecafe	
#define BP_KNOCK_EAX_ANSWER 0x69696969
#endif


#undef _KdPrint

#ifdef ENABLE_DEBUG_PRINTS
#define _KdPrint(x) ComPrint x
#else
#define _KdPrint(x) {}
#endif

#define ABSOLUTE(wait) (wait)

#define RELATIVE(wait) (-(wait))

#define NANOSECONDS(nanos)   \
	(((signed __int64)(nanos)) / 100L)

#define MICROSECONDS(micros) \
	(((signed __int64)(micros)) * NANOSECONDS(1000L))

#define MILLISECONDS(milli)  \
	(((signed __int64)(milli)) * MICROSECONDS(1000L))

#define SECONDS(seconds)	 \
	(((signed __int64)(seconds)) * MILLISECONDS(1000L))

#define MINUTES(minutes)	 \
	(((signed __int64)(minutes)) * SECONDS(60L))

#define HOURS(hours)		 \
	(((signed __int64)(hours)) * MINUTES(60L))


#define ALIGN(x,y)	(((x)+(y)-1)&(~((y)-1)))

#define	PML4_BASE	0xFFFFF6FB7DBED000
#define	PDP_BASE	0xFFFFF6FB7DA00000
#define	PD_BASE		0xFFFFF6FB40000000
#define	PT_BASE		0xFFFFF68000000000


typedef NTSTATUS (NTAPI *PCALLBACK_PROC)(PVOID Param);


/* 
* Attribute for segment selector. This is a copy of bit 40:47 & 52:55 of the
* segment descriptor. 
*/
typedef union
{
	USHORT UCHARs;
	struct
	{
		USHORT type:4;    /* 0;  Bit 40-43 */
		USHORT s:   1;    /* 4;  Bit 44 */
		USHORT dpl: 2;    /* 5;  Bit 45-46 */
		USHORT p:   1;    /* 7;  Bit 47 */
		// gap!       
		USHORT avl: 1;    /* 8;  Bit 52 */
		USHORT l:   1;    /* 9;  Bit 53 */
		USHORT db:  1;    /* 10; Bit 54 */
		USHORT g:   1;    /* 11; Bit 55 */
	} fields;
} SEGMENT_ATTRIBUTES;

#pragma pack (push, 1)

typedef struct _TSS64 {
	ULONG	Reserved0;
	PVOID	RSP0;
	PVOID	RSP1;
	PVOID	RSP2;
	ULONG64	Reserved1;
	PVOID	IST1;
	PVOID	IST2;
	PVOID	IST3;
	PVOID	IST4;
	PVOID	IST5;
	PVOID	IST6;
	PVOID	IST7;
	ULONG64	Reserved2;
	USHORT	Reserved3;
	USHORT	IOMapBaseAddress;
} TSS64, *PTSS64;


typedef struct 
{
	USHORT        sel;
	SEGMENT_ATTRIBUTES attributes;
	ULONG32        limit;
	ULONG64        base;
} SEGMENT_SELECTOR;


typedef struct {
	USHORT limit0;
	USHORT base0;
	UCHAR base1;
	UCHAR attr0;
	UCHAR limit1attr1;
	UCHAR base2;
} SEGMENT_DESCRIPTOR, *PSEGMENT_DESCRIPTOR;

typedef struct _INTERRUPT_GATE_DESCRIPTOR {
	USHORT	TargetOffset1500;
	USHORT	TargetSelector;
	UCHAR	InterruptStackTable;
	UCHAR	Attributes;
	USHORT	TargetOffset3116;
	ULONG32 TargetOffset6332;
	ULONG32	Reserved;
} INTERRUPT_GATE_DESCRIPTOR, *PINTERRUPT_GATE_DESCRIPTOR;

#pragma pack (pop)

#define LA_ACCESSED		0x01
#define LA_READABLE		0x02	// for code segments
#define LA_WRITABLE		0x02	// for data segments
#define LA_CONFORMING	0x04	// for code segments
#define LA_EXPANDDOWN	0x04	// for data segments
#define LA_CODE			0x08
#define LA_STANDARD		0x10
#define LA_DPL_0		0x00
#define LA_DPL_1		0x20
#define LA_DPL_2		0x40
#define LA_DPL_3		0x60
#define LA_PRESENT		0x80

#define LA_LDT64		0x02
#define LA_ATSS64		0x09
#define LA_BTSS64		0x0b
#define LA_CALLGATE64	0x0c
#define LA_INTGATE64	0x0e
#define LA_TRAPGATE64	0x0f

#define HA_AVAILABLE	0x01
#define HA_LONG			0x02
#define HA_DB			0x04
#define HA_GRANULARITY	0x08

#define P_PRESENT			0x01
#define P_WRITABLE			0x02
#define P_USERMODE			0x04
#define P_WRITETHROUGH		0x08
#define P_CACHE_DISABLED	0x10
#define P_ACCESSED			0x20
#define P_DIRTY				0x40
#define P_LARGE				0x80
#define P_GLOBAL			0x100


#define REG_MASK			0x07

#define REG_GP				0x08
#define REG_GP_ADDITIONAL	0x10
#define REG_CONTROL			0x20
#define REG_DEBUG			0x40
#define REG_RFLAGS			0x80


#define	REG_RAX	REG_GP | 0
#define REG_RCX	REG_GP | 1
#define REG_RDX	REG_GP | 2
#define REG_RBX	REG_GP | 3
#define REG_RSP	REG_GP | 4
#define REG_RBP	REG_GP | 5
#define REG_RSI	REG_GP | 6
#define REG_RDI	REG_GP | 7

#define	REG_R8	REG_GP_ADDITIONAL | 0
#define	REG_R9	REG_GP_ADDITIONAL | 1
#define	REG_R10	REG_GP_ADDITIONAL | 2
#define	REG_R11	REG_GP_ADDITIONAL | 3
#define	REG_R12	REG_GP_ADDITIONAL | 4
#define	REG_R13	REG_GP_ADDITIONAL | 5
#define	REG_R14	REG_GP_ADDITIONAL | 6
#define	REG_R15	REG_GP_ADDITIONAL | 7

#define REG_CR0	REG_CONTROL | 0
#define REG_CR2	REG_CONTROL | 2
#define REG_CR3	REG_CONTROL | 3
#define REG_CR4	REG_CONTROL | 4

#define REG_DR0	REG_DEBUG | 0
#define REG_DR1	REG_DEBUG | 1
#define REG_DR2	REG_DEBUG | 2
#define REG_DR3	REG_DEBUG | 3
#define REG_DR6	REG_DEBUG | 6
#define REG_DR7	REG_DEBUG | 7




typedef struct _CPU *PCPU;


typedef struct _GUEST_REGS {
	ULONG64	rcx;	// 0x00
	ULONG64	rdx;
	ULONG64	r8;		// 0x10
	ULONG64	r9;
	ULONG64	r10;	// 0x20
	ULONG64	r11;
	ULONG64	r12;	// 0x30
	ULONG64	r13;
	ULONG64	r14;	// 0x40
	ULONG64	r15;
	ULONG64	rdi;	// 0x50
	ULONG64	rsi;
	ULONG64	rbx;	// 0x60
	ULONG64	rbp;
} GUEST_REGS, *PGUEST_REGS;


typedef BOOLEAN (NTAPI *ARCH_IS_HVM_IMPLEMENTED)();

typedef NTSTATUS (NTAPI *ARCH_INITIALIZE)(PCPU Cpu,PVOID GuestRip,PVOID GuestRsp);
typedef NTSTATUS (NTAPI *ARCH_VIRTUALIZE)(PCPU Cpu);
typedef NTSTATUS (NTAPI *ARCH_SHUTDOWN)(PCPU Cpu,PGUEST_REGS GuestRegs,BOOLEAN bSetupTimeBomb);

typedef BOOLEAN (NTAPI *ARCH_IS_NESTED_EVENT)(PCPU Cpu,PGUEST_REGS GuestRegs);
typedef VOID (NTAPI *ARCH_DISPATCH_NESTED_EVENT)(PCPU Cpu,PGUEST_REGS GuestRegs);
typedef VOID (NTAPI *ARCH_DISPATCH_EVENT)(PCPU Cpu,PGUEST_REGS GuestRegs);
typedef VOID (NTAPI *ARCH_ADJUST_RIP)(PCPU Cpu,PGUEST_REGS GuestRegs,ULONG64 Delta);
typedef NTSTATUS (NTAPI *ARCH_REGISTER_TRAPS)(PCPU Cpu);


typedef struct {
	UCHAR	Architecture;

	ARCH_IS_HVM_IMPLEMENTED	ArchIsHvmImplemented;

	ARCH_INITIALIZE	ArchInitialize;
	ARCH_VIRTUALIZE	ArchVirtualize;
	ARCH_SHUTDOWN	ArchShutdown;

	ARCH_IS_NESTED_EVENT	ArchIsNestedEvent;
	ARCH_DISPATCH_NESTED_EVENT	ArchDispatchNestedEvent;
	ARCH_DISPATCH_EVENT		ArchDispatchEvent;
	ARCH_ADJUST_RIP		ArchAdjustRip;
	ARCH_REGISTER_TRAPS		ArchRegisterTraps;
} HVM_DEPENDENT, *PHVM_DEPENDENT;



NTSTATUS NTAPI CmPatchPTEPhysicalAddress(
			PULONG64 pPte,
			PVOID PageVA,
			PHYSICAL_ADDRESS NewPhysicalAddress
);

NTSTATUS NTAPI CmGetPagePTEAddress(
			PVOID Page,
			PULONG64* pPagePTE,
			PHYSICAL_ADDRESS* pPA
);

NTSTATUS NTAPI CmSetIdtEntry(
			PINTERRUPT_GATE_DESCRIPTOR IdtBase,
			ULONG IdtLimit,
			ULONG InterruptNumber,
			USHORT TargetSelector,
			PVOID TargetOffset,
			UCHAR InterruptStackTable,
			UCHAR Attributes
);

NTSTATUS NTAPI CmDumpGdt(
			PUCHAR GdtBase,
			USHORT GdtLimit
);

NTSTATUS CmDumpTSS64(
			PTSS64 Tss64,
			USHORT Tss64Limit
);

NTSTATUS NTAPI CmSetGdtEntry(
			PSEGMENT_DESCRIPTOR GdtBase,
			ULONG GdtLimit,
			ULONG SelectorNumber,
			PVOID SegmentBase,
			ULONG SegmentLimit,
			UCHAR LowAttributes,
			UCHAR HighAttributes
);

VOID NTAPI CmClgi(
);

VOID NTAPI CmStgi(
);

VOID NTAPI CmCli(
);

VOID NTAPI CmSti(
);

VOID NTAPI CmDebugBreak(
);

VOID NTAPI CmInvalidatePage(
			PVOID Page
);

VOID NTAPI CmReloadGdtr(
			PVOID GdtBase,
			ULONG GdtLimit
);

VOID NTAPI CmReloadIdtr(
			PVOID IdtBase,
			ULONG IdtLimit
);

VOID NTAPI CmSetBluepillESDS(
);

VOID NTAPI CmSetBluepillGS(
);

VOID NTAPI CmSetDS(
			ULONG Selector
);

VOID NTAPI CmSetES(
			ULONG Selector
);

VOID NTAPI CmFreePhysPages(
			PVOID BaseAddress,
			ULONG uNoOfPages
);

NTSTATUS NTAPI CmSubvert(
			PVOID
);

NTSTATUS NTAPI CmSlipIntoMatrix(
			PVOID
);

NTSTATUS NTAPI CmDeliverToProcessor(
			CCHAR cProcessorNumber,
			PCALLBACK_PROC CallbackProc,
			PVOID CallbackParam,
			PNTSTATUS pCallbackStatus
);

NTSTATUS NTAPI CmInitializeSegmentSelector(
			SEGMENT_SELECTOR *SegmentSelector,
			USHORT Selector,
			PUCHAR GdtBase
);

NTSTATUS NTAPI CmGenerateIretq(
			PUCHAR pCode,
			PULONG pGeneratedCodeLength
);

NTSTATUS NTAPI CmGeneratePushReg(
			PUCHAR pCode,
			PULONG pGeneratedCodeLength,
			ULONG Register
);

NTSTATUS NTAPI CmGenerateMovReg(
			PUCHAR pCode,
			PULONG pGeneratedCodeLength,
			ULONG Register,
			ULONG64 Value
);

NTSTATUS NTAPI CmGenerateCallReg(
			PUCHAR pCode,
			PULONG pGeneratedCodeLength,
			ULONG Register
);

#define ITL_TAG	'LTI'
