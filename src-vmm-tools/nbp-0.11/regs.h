#pragma once

#include <ntddk.h>

USHORT NTAPI RegGetCs();
USHORT NTAPI RegGetDs();
USHORT NTAPI RegGetEs();
USHORT NTAPI RegGetSs();

ULONG64 NTAPI RegGetCr0();
ULONG64 NTAPI RegGetCr2();
ULONG64 NTAPI RegGetCr3();
ULONG64 NTAPI RegGetCr4();
ULONG64 NTAPI RegGetRflags();
ULONG64 NTAPI RegGetRsp();

ULONG64 NTAPI GetIdtBase();
USHORT NTAPI GetIdtLimit();
ULONG64 NTAPI GetGdtBase();
USHORT NTAPI GetGdtLimit();

ULONG64 NTAPI GetTrSelector();


ULONG64 NTAPI RegGetRbx();
ULONG64 NTAPI RegGetTSC();


ULONG64 NTAPI RegGetDr0();
ULONG64 NTAPI RegGetDr1();
ULONG64 NTAPI RegGetDr2();
ULONG64 NTAPI RegGetDr3();
ULONG64 NTAPI RegSetDr0();
ULONG64 NTAPI RegSetDr1();
ULONG64 NTAPI RegSetDr2();
ULONG64 NTAPI RegSetDr3();
