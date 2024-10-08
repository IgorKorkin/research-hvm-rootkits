#pragma once

#include <ntddk.h>
#include "common.h"



#define AP_PAGETABLE	1	// used to mark allocations of host pagetables
#define AP_PT		2
#define AP_PD		4
#define AP_PDP		8
#define AP_PML4		16


typedef enum {
	PAT_DONT_FREE=0,
	PAT_POOL,
	PAT_CONTIGUOUS
} PAGE_ALLOCATION_TYPE;


typedef struct _ALLOCATED_PAGE {
	
	LIST_ENTRY	le;

	ULONG	Flags;

	PAGE_ALLOCATION_TYPE	AllocationType;
	ULONG	uNumberOfPages;			// for PAT_CONTIGUOUS only

	PHYSICAL_ADDRESS	PhysicalAddress;
	PVOID	HostAddress;
	PVOID	GuestAddress;

} ALLOCATED_PAGE, *PALLOCATED_PAGE;


NTSTATUS NTAPI MmCreateMapping(
					PHYSICAL_ADDRESS PhysicalAddress,
					PVOID VirtualAddress,
					BOOLEAN bLargePage
);

PVOID NTAPI MmAllocateContiguousPages(
					ULONG uNumberOfPages,
					PPHYSICAL_ADDRESS pFirstPagePA
);

PVOID NTAPI MmAllocatePages(
					ULONG uNumberOfPages,
					PPHYSICAL_ADDRESS pFirstPagePA
);


NTSTATUS NTAPI MmMapGuestPages(
					PVOID FirstPage,
					ULONG uNumberOfPages
);

NTSTATUS NTAPI MmMapGuestKernelPages(
);

NTSTATUS NTAPI MmMapGuestTSS64(
					PVOID Tss64,
					USHORT Tss64Limit
);

NTSTATUS NTAPI MmInitManager(
);

NTSTATUS NTAPI MmShutdownManager(
);