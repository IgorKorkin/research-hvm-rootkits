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

#include "comprint.h"


VOID ComPrint(PUCHAR fmt, ...)
{
	va_list args;
	UCHAR str[128]={0};
	int i,len,j;
	va_start (args, fmt);


	if (*(PULONG32)fmt!='X20%' && *(PUSHORT)fmt!='\r\n')
		ComPrint("%02X> ",g_BpId);

	vsnprintf((PUCHAR)&str,sizeof(str)-1,(PUCHAR)fmt,args);

	len = (int)strlen (str);

	for (i=0;i<len;i++)
		PioOutByte(str[i]);

	if (*(PULONG32)fmt!='X20%' && *(PUSHORT)fmt!='\r\n')
		PioOutByte('\r');
}
