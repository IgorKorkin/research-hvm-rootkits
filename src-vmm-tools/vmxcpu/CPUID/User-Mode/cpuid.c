#include <stdio.h>
#include <windows.h>

void main(int argc, char * args[])
{
	DWORD		lpProcessAffinityMask;
	DWORD		lpSystemAffinityMask;

	ULONG		A;
	ULONG		C;
	ULONG		D;
	ULONG		B;

	char		VendorID[20] = {0};

	SetProcessAffinityMask( GetCurrentProcess(), 0x00000001 );
	
	printf( "[CPUID]   Processor 0\n" );
	
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

	printf( "[CPUID]     EAX : %08X\n", A );
	printf( "[CPUID]     ECX : %08X\n", C );
	printf( "[CPUID]     EDX : %08X\n", D );
	printf( "[CPUID]     EBX : %08X\n", B );

	memmove( (VendorID + 0), &B, 4 );
	memmove( (VendorID + 4), &D, 4 );
	memmove( (VendorID + 8), &C, 4 );

	printf( "[CPUID]     Vendor ID : %s\n", VendorID );
	
	SetProcessAffinityMask( GetCurrentProcess(), 0x00000002 );
	
	printf( "[CPUID]   Processor 1\n" );

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

	printf( "[CPUID]     EAX : %08X\n", A );
	printf( "[CPUID]     ECX : %08X\n", C );
	printf( "[CPUID]     EDX : %08X\n", D );
	printf( "[CPUID]     EBX : %08X\n", B );

	memmove( (VendorID + 0), &B, 4 );
	memmove( (VendorID + 4), &D, 4 );
	memmove( (VendorID + 8), &C, 4 );

	printf( "[CPUID]     Vendor ID : %s\n", VendorID );

	GetProcessAffinityMask( GetCurrentProcess(), &lpProcessAffinityMask, &lpSystemAffinityMask );
	SetProcessAffinityMask( GetCurrentProcess(), lpProcessAffinityMask );

	printf("\nPress <ENTER> to continue ...\n");
	getchar();

	return;
}

