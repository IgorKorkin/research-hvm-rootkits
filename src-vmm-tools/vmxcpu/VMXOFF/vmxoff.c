#include <ntddk.h>

VOID DriverUnload( IN PDRIVER_OBJECT DriverObject )
{
	DbgPrint( "[VMXOFF] Driver Unloaded.\n" );
}

NTSTATUS DriverEntry( IN PDRIVER_OBJECT  DriverObject, IN PUNICODE_STRING  RegistryPath )
{
	DriverObject->DriverUnload = DriverUnload;

	DbgPrint( "[VMXOFF] Driver Loaded.\n" );

	KeSetSystemAffinityThread( (KAFFINITY) 0x00000001 );
	
	DbgPrint( "[VMXOFF]   Processor 0\n" );
	
	__asm
	{
		PUSHAD
		
		_emit	0x0F
		_emit	0x01
		_emit	0xC4	// VMXOFF
		
		POPAD
	}
	

	KeSetSystemAffinityThread( (KAFFINITY) 0x00000002 );
	
	DbgPrint( "[VMXOFF]   Processor 1\n" );

	__asm
	{
		PUSHAD
		
		_emit	0x0F
		_emit	0x01
		_emit	0xC4	// VMXOFF
		
		
		POPAD
	}

	return STATUS_SUCCESS;
}

