#include <stdio.h>
#include <windows.h>

void main( int argc, char * argv[] )
{
	DWORD		lpProcessAffinityMask;
	DWORD		lpSystemAffinityMask;
	
	printf( "[VMXOFF] Driver Loaded.\n" );

	SetProcessAffinityMask( GetCurrentProcess(), 0x00000001 );
	
	printf( "[VMXOFF]   Processor 0\n" );
	
	__asm
	{
		PUSHAD
		
		_emit	0x0F
		_emit	0x01
		_emit	0xC4	// VMXOFF
		
		POPAD
	}
	
	SetProcessAffinityMask( GetCurrentProcess(), 0x00000002 );
	
	printf( "[VMXOFF]   Processor 1\n" );

	__asm
	{
		PUSHAD
		
		_emit	0x0F
		_emit	0x01
		_emit	0xC4	// VMXOFF
		
		
		POPAD
	}

	GetProcessAffinityMask( GetCurrentProcess(), &lpProcessAffinityMask, &lpSystemAffinityMask );
	SetProcessAffinityMask( GetCurrentProcess(), lpProcessAffinityMask );

	printf("\nPress <ENTER> to continue ...\n");
	getchar();

	return;
}

