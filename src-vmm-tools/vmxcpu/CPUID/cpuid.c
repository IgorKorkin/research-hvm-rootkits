#include <ntddk.h>

VOID DriverUnload( IN PDRIVER_OBJECT DriverObject )
{
	DbgPrint( "[CPUID] Driver Unloaded.\n" );
}

NTSTATUS DriverEntry( IN PDRIVER_OBJECT  DriverObject, IN PUNICODE_STRING  RegistryPath )
{
	ULONG		NumberActiveProcessors;

	ULONG		A;
	ULONG		C;
	ULONG		D;
	ULONG		B;

	char		VendorID[20] = {0};

	DriverObject->DriverUnload = DriverUnload;

	DbgPrint( "[CPUID] Driver Loaded.\n" );

	KeSetSystemAffinityThread( (KAFFINITY) 0x00000001 );
	
	DbgPrint( "[CPUID]   Processor 0\n" );
	
	__asm
	{
		PUSHAD
		
		MOV		EAX, 0
		CPUID
		
		MOV		A, EAX
		MOV		C, ECX
		MOV		D, EDX
		MOV		B, EBX
		
		POPAD
	}

	DbgPrint( "[CPUID]     EAX : %08X\n", A );
	DbgPrint( "[CPUID]     ECX : %08X\n", C );
	DbgPrint( "[CPUID]     EDX : %08X\n", D );
	DbgPrint( "[CPUID]     EBX : %08X\n", B );

	RtlCopyBytes( (VendorID + 0), &B, 4 );
	RtlCopyBytes( (VendorID + 4), &D, 4 );
	RtlCopyBytes( (VendorID + 8), &C, 4 );

	DbgPrint( "[CPUID]     Vendor ID : %s\n", VendorID );
	
	KeSetSystemAffinityThread( (KAFFINITY) 0x00000002 );
	
	DbgPrint( "[CPUID]   Processor 1\n" );

	__asm
	{
		PUSHAD
		
		MOV		EAX, 0
		CPUID
		
		MOV		A, EAX
		MOV		C, ECX
		MOV		D, EDX
		MOV		B, EBX
		
		POPAD
	}

	DbgPrint( "[CPUID]     EAX : %08X\n", A );
	DbgPrint( "[CPUID]     ECX : %08X\n", C );
	DbgPrint( "[CPUID]     EDX : %08X\n", D );
	DbgPrint( "[CPUID]     EBX : %08X\n", B );

	RtlCopyBytes( (VendorID + 0), &B, 4 );
	RtlCopyBytes( (VendorID + 4), &D, 4 );
	RtlCopyBytes( (VendorID + 8), &C, 4 );

	DbgPrint( "[CPUID]     Vendor ID : %s\n", VendorID );

	return STATUS_SUCCESS;
}

