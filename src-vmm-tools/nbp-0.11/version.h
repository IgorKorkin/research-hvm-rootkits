#pragma once

#include <ntddk.h>


#define NBP_BUILD_NUMBER		11


#define	MAKE_VERSION(Major,Minor,BuildNumber,Reserved)	(ULONG64)((((ULONG64)Major)<<48)+(((ULONG64)Minor)<<32)+(((ULONG64)BuildNumber)<<16)+(Reserved))
#define NBP_VERSION	MAKE_VERSION(1,0,NBP_BUILD_NUMBER,0)
