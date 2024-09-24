#pragma once

#include <ntddk.h>

#include "portio.h"
#include "snprintf.h"
#include <stdarg.h>



VOID ComPrint(
			PUCHAR fmt,
			...
);
