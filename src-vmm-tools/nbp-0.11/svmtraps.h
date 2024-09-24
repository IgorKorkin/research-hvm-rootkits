#pragma once

#include <ntddk.h>
#include "common.h"
#include "svm.h"
#include "traps.h"
#include "hypercalls.h"



NTSTATUS NTAPI SvmRegisterTraps(
			PCPU Cpu
);

