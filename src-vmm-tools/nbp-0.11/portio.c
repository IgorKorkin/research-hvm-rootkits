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

#include "portio.h"


// Bug: doesn't initialize port


static PUCHAR	g_DebugComPort=NULL;
static UCHAR	bDummy;

UCHAR	g_BpId=0;

VOID NTAPI PioInit(PUCHAR ComPortAddress)
{
	g_DebugComPort=ComPortAddress;
	g_BpId=(UCHAR)RegGetTSC();
}


VOID NTAPI PioOutByte(UCHAR Byte)
{
	ULONG	i;

	while (!(READ_PORT_UCHAR(g_DebugComPort+LINE_STATUS_REGISTER) & LS_THR_EMPTY)) {
		bDummy^=1;
	};
	WRITE_PORT_UCHAR(g_DebugComPort+TRANSMIT_HOLDING_REGISTER, Byte); 
}
