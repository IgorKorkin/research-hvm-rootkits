EXTERN	HvmSubvertCpu:PROC
EXTERN	HvmResumeGuest:PROC


.CODE

cm_clgi MACRO
	BYTE	0Fh, 01h, 0DDh
ENDM

cm_stgi MACRO
	BYTE	0Fh, 01h, 0DCh
ENDM


CmClgi PROC
	cm_clgi
	ret
CmClgi ENDP

CmStgi PROC
	cm_stgi
	ret
CmStgi ENDP

CmCli PROC
	cli
	ret
CmCli  ENDP

CmSti PROC
	sti
	ret
CmSti ENDP

CmDebugBreak PROC
	int	3
	ret
CmDebugBreak ENDP

; CmReloadGdtr (PVOID GdtBase (rcx), ULONG GdtLimit (rdx) );

CmReloadGdtr PROC
	push	rcx
	shl		rdx, 48
	push	rdx
	lgdt	fword ptr [rsp+6]	; do not try to modify stack selector with this ;)
	pop		rax
	pop		rax
	ret
CmReloadGdtr ENDP

; CmReloadIdtr (PVOID IdtBase (rcx), ULONG IdtLimit (rdx) );

CmReloadIdtr PROC
	push	rcx
	shl		rdx, 48
	push	rdx
	lidt	fword ptr [rsp+6]
	pop		rax
	pop		rax
	ret
CmReloadIdtr ENDP

; CmSetBluepillESDS ();

CmSetBluepillESDS PROC
	mov		rax, 18h			; this must be equal to BP_GDT64_DATA (bluepill.h)
	mov		ds, rax
	mov		es, rax
	ret
CmSetBluepillESDS ENDP

; CmSetBluepillGS ();

CmSetBluepillGS PROC
	mov		rax, 18h			; this must be equal to BP_GDT64_PCR (bluepill.h)
	mov		gs, rax
	ret
CmSetBluepillGS ENDP

; CmSetDS (ULONG Selector (rcx) );

CmSetDS PROC
	mov		ds, rcx
	ret
CmSetDS ENDP

; CmSetES (ULONG Selector (rcx) );

CmSetES PROC
	mov		es, rcx
	ret
CmSetES ENDP

; CmInvalidatePage (PVOID PageVA (rcx) );

CmInvalidatePage PROC
	invlpg	[rcx]
	ret
CmInvalidatePage ENDP



CmSubvert PROC


	push	rax
	push	rcx
	push	rdx
	push	rbx
	push	rbp
	push	rsi
	push	rdi
	push	r8
	push	r9
	push	r10
	push	r11
	push	r12
	push	r13
	push	r14
	push	r15

	sub	rsp, 28h

	mov	rcx, rsp
	call	HvmSubvertCpu

CmSubvert ENDP

CmSlipIntoMatrix PROC

	call	HvmResumeGuest

	add	rsp, 28h

	pop	r15
	pop	r14
	pop	r13
	pop	r12
	pop	r11
	pop	r10
	pop	r9
	pop	r8
	pop	rdi
	pop	rsi
	pop	rbp
	pop	rbx
	pop	rdx
	pop	rcx
	pop	rax

	ret

CmSlipIntoMatrix ENDP


END