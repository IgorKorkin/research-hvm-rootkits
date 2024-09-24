; 
; Copyright holder: Invisible Things Lab
; 
; This software is protected by domestic and International
; copyright laws. Any use (including publishing and
; distribution) of this software requires a valid license
; from the copyright holder.
;
; This software is provided for the educational use only
; during the Black Hat training. This software should not
; be used on production systems.
;
;

extern	g_PageMapBasePhysicalAddress:QWORD
extern	HvmEventCallback:PROC
extern	McCloak:PROC


svm_vmload MACRO
	BYTE	0Fh, 01h, 0DAh
ENDM


svm_vmsave MACRO
	BYTE	0Fh, 01h, 0DBh
ENDM


svm_vmrun MACRO
	BYTE	0Fh, 01h, 0D8h
ENDM


svm_vmmcall MACRO
	BYTE	0Fh, 01h, 0D9h
ENDM


.CODE


; SvmVmsave (PHYSICAL_ADDRESS vmcb_pa (rcx) );

SvmVmsave PROC
	mov		rax, rcx
	svm_vmsave
	ret
SvmVmsave ENDP

; SvmVmload (PHYSICAL_ADDRESS vmcb_pa (rcx) );

SvmVmload PROC
	mov		rax, rcx
	svm_vmload
	ret
SvmVmload ENDP


; Stack layout for SvmVmrun() call:
;
; ^                              ^
; |                              |
; | lots of pages for host stack |
; |                              |
; |------------------------------|   <- HostStackBottom(rcx) points here
; |         struct CPU           |
; --------------------------------

; SvmVmrun(PVOID HostStackBottom (rcx))

SvmVmrun PROC


	lea		rsp, [rcx-14*8-5*8]		; backup 14 regs and leave space for FASTCALL call

	mov		rax, [g_PageMapBasePhysicalAddress]
	mov		cr3, rax

	call		McCloak

@loop:

;	clgi

	mov		rax, [rsp+14*8+5*8+8]	; CPU.Svm.VmcbToContinuePA

	svm_vmload
	svm_vmrun
	svm_vmsave						; save guest FS etc.

;	stgi

;	lea		rax, [rsp+14*8+5*8]		; PCPU
;	mov		rax, [rax+10h]			; vmcb va
;	add		qword ptr [rax+578h], 2		; rip
;	jmp	@loop

	; save guest state

	mov		[rsp+5*8+00h], rcx
	mov		[rsp+5*8+08h], rdx

	rdtsc
	shl		rdx, 32
	or		rax, rdx


	mov		[rsp+5*8+10h], r8
	mov		[rsp+5*8+18h], r9
	mov		[rsp+5*8+20h], r10
	mov		[rsp+5*8+28h], r11
	mov		[rsp+5*8+30h], r12
	mov		[rsp+5*8+38h], r13
	mov		[rsp+5*8+40h], r14
	mov		[rsp+5*8+48h], r15
	mov		[rsp+5*8+50h], rdi
	mov		[rsp+5*8+58h], rsi
	mov		[rsp+5*8+60h], rbx
	mov		[rsp+5*8+68h], rbp

	mov		r8, rax

	lea		rdx, [rsp+5*8]			; PGUEST_REGS
	lea		rcx, [rsp+14*8+5*8]		; PCPU
	call	HvmEventCallback

	; restore guest state (HvmEventCallback migth have alternated the guest state)

	mov		rcx, [rsp+5*8+00h]
	mov		rdx, [rsp+5*8+08h]
	mov		r8,  [rsp+5*8+10h]
	mov		r9,  [rsp+5*8+18h]
	mov		r10, [rsp+5*8+20h]
	mov		r11, [rsp+5*8+28h]
	mov		r12, [rsp+5*8+30h]
	mov		r13, [rsp+5*8+38h]
	mov		r14, [rsp+5*8+40h]
	mov		r15, [rsp+5*8+48h]
	mov		rdi, [rsp+5*8+50h]
	mov		rsi, [rsp+5*8+58h]
	mov		rbx, [rsp+5*8+60h]
	mov		rbp, [rsp+5*8+68h]
	
	jmp		@loop

SvmVmrun ENDP



END
