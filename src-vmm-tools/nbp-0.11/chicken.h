#pragma once
#include "common.h"
#include "hvm.h"

VOID NTAPI ChickenAddInterceptTsc(PCPU Cpu) ;
BOOLEAN NTAPI ChickenShouldUninstall(PCPU Cpu) ;


