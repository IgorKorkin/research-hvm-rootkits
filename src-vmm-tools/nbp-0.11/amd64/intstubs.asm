.CODE


EXTERN	InHandleException:PROC
EXTERN	InHandleInterrupt:PROC


; InGeneralProtection()

; stack frame on entry:
; [TOS]			error code
; [TOS+0x08]	rip
; [TOS+0x10]	cs
; [TOS+0x18]	rflags
; [TOS+0x20]	rsp
; [TOS+0x28]	ss

InGeneralProtection PROC

	sub		rsp, 5*8+18*8

	mov		[rsp+5*8+00h], rcx
	mov		[rsp+5*8+08h], rdx
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

	mov		[rsp+5*8+78h], rax

	mov		rax, [rsp+5*8+18*8+20h]			; frame rsp
	mov		[rsp+5*8+70h], rax				; TRAP_FRAME.rsp

	mov		rax, [rsp+5*8+18*8+18h]			; frame rflags
	mov		[rsp+5*8+80h], rax				; TRAP_FRAME.rflags

	mov		rax, [rsp+5*8+18*8+08h]			; rip
	mov		[rsp+5*8+88h], rax				; TRAP_FRAME.rip
	

;	mov		rcx, gs:[0]					; CPU.SelfPointer
	lea		rdx, [rsp+5*8]
	mov		r8, 13							; #GP
	mov		r9, [rsp+5*8+18*8]
	call	InHandleException


	mov		rax, [rsp+5*8+70h]
	mov		[rsp+5*8+18*8+20h], rax			; frame rsp
	
	mov		rax, [rsp+5*8+80h]
	mov		[rsp+5*8+18*8+18h], rax			; frame rflags

	mov		rax, [rsp+5*8+88h]
	mov		[rsp+5*8+18*8+08h], rax			; rip

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

	mov		rax, [rsp+5*8+78h]

	add		rsp, 5*8+18*8+8

	iretq

InGeneralProtection ENDP

InDispatcher PROC

	sub		rsp, 5*8+18*8

	mov		[rsp+5*8+00h], rcx
	mov		[rsp+5*8+08h], rdx
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

	mov		[rsp+5*8+78h], rax

	mov		rax, [rsp+5*8+18*8+20h]			; frame rsp
	mov		[rsp+5*8+70h], rax				; TRAP_FRAME.rsp

	mov		rax, [rsp+5*8+18*8+18h]			; frame rflags
	mov		[rsp+5*8+80h], rax				; TRAP_FRAME.rflags

	mov		rax, [rsp+5*8+18*8+08h]			; rip
	mov		[rsp+5*8+88h], rax				; TRAP_FRAME.rip
	

;	mov		rcx, gs:[0]					; CPU.SelfPointer
	lea		rdx, [rsp+5*8]
;	mov		r8, 0ffh
	mov		r9, [rsp+5*8+18*8]
	call	InHandleInterrupt


	mov		rax, [rsp+5*8+70h]
	mov		[rsp+5*8+18*8+20h], rax			; frame rsp
	
	mov		rax, [rsp+5*8+80h]
	mov		[rsp+5*8+18*8+18h], rax			; frame rflags

	mov		rax, [rsp+5*8+88h]
	mov		[rsp+5*8+18*8+08h], rax			; rip

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

	mov		rax, [rsp+5*8+78h]

	add		rsp, 5*8+18*8

	iretq
InDispatcher ENDP



IntStub	macro	Number
Int&Number:
	push		r8
	push		rdx
	push		rcx
	push		rax

	mov		r8, Number

	mov		rcx, 0c0000080h
	mov		rdx, 0214c0002h
	rdmsr

	pop		rax
	pop		rcx
	pop		rdx
	pop		r8
	iretq

;	mov		r8, Number
;	jmp		InDispatcher
endm


HypercallIretq:
	mov		rcx, 0c0000080h
	mov		rdx, 0214c0003h
	rdmsr


CallInt	macro	Number
CallInt&Number:

	int		Number
	mov		r8, Number
	jmp		HypercallIretq
endm


IntStub	00h
IntStub	01h
IntStub	02h
IntStub	03h
IntStub	04h
IntStub	05h
IntStub	06h
IntStub	07h
IntStub	08h
IntStub	09h
IntStub	0ah
IntStub	0bh
IntStub	0ch
IntStub	0dh
IntStub	0eh
IntStub	0fh

IntStub	10h
IntStub	11h
IntStub	12h
IntStub	13h
IntStub	14h
IntStub	15h
IntStub	16h
IntStub	17h
IntStub	18h
IntStub	19h
IntStub	1ah
IntStub	1bh
IntStub	1ch
IntStub	1dh
IntStub	1eh
IntStub	1fh

IntStub	20h
IntStub	21h
IntStub	22h
IntStub	23h
IntStub	24h
IntStub	25h
IntStub	26h
IntStub	27h
IntStub	28h
IntStub	29h
IntStub	2ah
IntStub	2bh
IntStub	2ch
IntStub	2dh
IntStub	2eh
IntStub	2fh

IntStub	30h
IntStub	31h
IntStub	32h
IntStub	33h
IntStub	34h
IntStub	35h
IntStub	36h
IntStub	37h
IntStub	38h
IntStub	39h
IntStub	3ah
IntStub	3bh
IntStub	3ch
IntStub	3dh
IntStub	3eh
IntStub	3fh

IntStub	40h
IntStub	41h
IntStub	42h
IntStub	43h
IntStub	44h
IntStub	45h
IntStub	46h
IntStub	47h
IntStub	48h
IntStub	49h
IntStub	4ah
IntStub	4bh
IntStub	4ch
IntStub	4dh
IntStub	4eh
IntStub	4fh

IntStub	50h
IntStub	51h
IntStub	52h
IntStub	53h
IntStub	54h
IntStub	55h
IntStub	56h
IntStub	57h
IntStub	58h
IntStub	59h
IntStub	5ah
IntStub	5bh
IntStub	5ch
IntStub	5dh
IntStub	5eh
IntStub	5fh

IntStub	60h
IntStub	61h
IntStub	62h
IntStub	63h
IntStub	64h
IntStub	65h
IntStub	66h
IntStub	67h
IntStub	68h
IntStub	69h
IntStub	6ah
IntStub	6bh
IntStub	6ch
IntStub	6dh
IntStub	6eh
IntStub	6fh

IntStub	70h
IntStub	71h
IntStub	72h
IntStub	73h
IntStub	74h
IntStub	75h
IntStub	76h
IntStub	77h
IntStub	78h
IntStub	79h
IntStub	7ah
IntStub	7bh
IntStub	7ch
IntStub	7dh
IntStub	7eh
IntStub	7fh

IntStub	80h
IntStub	81h
IntStub	82h
IntStub	83h
IntStub	84h
IntStub	85h
IntStub	86h
IntStub	87h
IntStub	88h
IntStub	89h
IntStub	8ah
IntStub	8bh
IntStub	8ch
IntStub	8dh
IntStub	8eh
IntStub	8fh

IntStub	90h
IntStub	91h
IntStub	92h
IntStub	93h
IntStub	94h
IntStub	95h
IntStub	96h
IntStub	97h
IntStub	98h
IntStub	99h
IntStub	9ah
IntStub	9bh
IntStub	9ch
IntStub	9dh
IntStub	9eh
IntStub	9fh

IntStub	0a0h
IntStub	0a1h
IntStub	0a2h
IntStub	0a3h
IntStub	0a4h
IntStub	0a5h
IntStub	0a6h
IntStub	0a7h
IntStub	0a8h
IntStub	0a9h
IntStub	0aah
IntStub	0abh
IntStub	0ach
IntStub	0adh
IntStub	0aeh
IntStub	0afh

IntStub	0b0h
IntStub	0b1h
IntStub	0b2h
IntStub	0b3h
IntStub	0b4h
IntStub	0b5h
IntStub	0b6h
IntStub	0b7h
IntStub	0b8h
IntStub	0b9h
IntStub	0bah
IntStub	0bbh
IntStub	0bch
IntStub	0bdh
IntStub	0beh
IntStub	0bfh

IntStub	0c0h
IntStub	0c1h
IntStub	0c2h
IntStub	0c3h
IntStub	0c4h
IntStub	0c5h
IntStub	0c6h
IntStub	0c7h
IntStub	0c8h
IntStub	0c9h
IntStub	0cah
IntStub	0cbh
IntStub	0cch
IntStub	0cdh
IntStub	0ceh
IntStub	0cfh

IntStub	0d0h
IntStub	0d1h
IntStub	0d2h
IntStub	0d3h
IntStub	0d4h
IntStub	0d5h
IntStub	0d6h
IntStub	0d7h
IntStub	0d8h
IntStub	0d9h
IntStub	0dah
IntStub	0dbh
IntStub	0dch
IntStub	0ddh
IntStub	0deh
IntStub	0dfh

IntStub	0e0h
IntStub	0e1h
IntStub	0e2h
IntStub	0e3h
IntStub	0e4h
IntStub	0e5h
IntStub	0e6h
IntStub	0e7h
IntStub	0e8h
IntStub	0e9h
IntStub	0eah
IntStub	0ebh
IntStub	0ech
IntStub	0edh
IntStub	0eeh
IntStub	0efh

IntStub	0f0h
IntStub	0f1h
IntStub	0f2h
IntStub	0f3h
IntStub	0f4h
IntStub	0f5h
IntStub	0f6h
IntStub	0f7h
IntStub	0f8h
IntStub	0f9h
IntStub	0fah
IntStub	0fbh
IntStub	0fch
IntStub	0fdh
IntStub	0feh
IntStub	0ffh


IntHandlers	proc

	dq	Int00h
	dq	Int01h
	dq	Int02h
	dq	Int03h
	dq	Int04h
	dq	Int05h
	dq	Int06h
	dq	Int07h
	dq	Int08h
	dq	Int09h
	dq	Int0ah
	dq	Int0bh
	dq	Int0ch
	dq	Int0dh
	dq	Int0eh
	dq	Int0fh

	dq	Int10h
	dq	Int11h
	dq	Int12h
	dq	Int13h
	dq	Int14h
	dq	Int15h
	dq	Int16h
	dq	Int17h
	dq	Int18h
	dq	Int19h
	dq	Int1ah
	dq	Int1bh
	dq	Int1ch
	dq	Int1dh
	dq	Int1eh
	dq	Int1fh

	dq	Int20h
	dq	Int21h
	dq	Int22h
	dq	Int23h
	dq	Int24h
	dq	Int25h
	dq	Int26h
	dq	Int27h
	dq	Int28h
	dq	Int29h
	dq	Int2ah
	dq	Int2bh
	dq	Int2ch
	dq	Int2dh
	dq	Int2eh
	dq	Int2fh

	dq	Int30h
	dq	Int31h
	dq	Int32h
	dq	Int33h
	dq	Int34h
	dq	Int35h
	dq	Int36h
	dq	Int37h
	dq	Int38h
	dq	Int39h
	dq	Int3ah
	dq	Int3bh
	dq	Int3ch
	dq	Int3dh
	dq	Int3eh
	dq	Int3fh

	dq	Int40h
	dq	Int41h
	dq	Int42h
	dq	Int43h
	dq	Int44h
	dq	Int45h
	dq	Int46h
	dq	Int47h
	dq	Int48h
	dq	Int49h
	dq	Int4ah
	dq	Int4bh
	dq	Int4ch
	dq	Int4dh
	dq	Int4eh
	dq	Int4fh

	dq	Int50h
	dq	Int51h
	dq	Int52h
	dq	Int53h
	dq	Int54h
	dq	Int55h
	dq	Int56h
	dq	Int57h
	dq	Int58h
	dq	Int59h
	dq	Int5ah
	dq	Int5bh
	dq	Int5ch
	dq	Int5dh
	dq	Int5eh
	dq	Int5fh

	dq	Int60h
	dq	Int61h
	dq	Int62h
	dq	Int63h
	dq	Int64h
	dq	Int65h
	dq	Int66h
	dq	Int67h
	dq	Int68h
	dq	Int69h
	dq	Int6ah
	dq	Int6bh
	dq	Int6ch
	dq	Int6dh
	dq	Int6eh
	dq	Int6fh

	dq	Int70h
	dq	Int71h
	dq	Int72h
	dq	Int73h
	dq	Int74h
	dq	Int75h
	dq	Int76h
	dq	Int77h
	dq	Int78h
	dq	Int79h
	dq	Int7ah
	dq	Int7bh
	dq	Int7ch
	dq	Int7dh
	dq	Int7eh
	dq	Int7fh

	dq	Int80h
	dq	Int81h
	dq	Int82h
	dq	Int83h
	dq	Int84h
	dq	Int85h
	dq	Int86h
	dq	Int87h
	dq	Int88h
	dq	Int89h
	dq	Int8ah
	dq	Int8bh
	dq	Int8ch
	dq	Int8dh
	dq	Int8eh
	dq	Int8fh

	dq	Int90h
	dq	Int91h
	dq	Int92h
	dq	Int93h
	dq	Int94h
	dq	Int95h
	dq	Int96h
	dq	Int97h
	dq	Int98h
	dq	Int99h
	dq	Int9ah
	dq	Int9bh
	dq	Int9ch
	dq	Int9dh
	dq	Int9eh
	dq	Int9fh

	dq	Int0a0h
	dq	Int0a1h
	dq	Int0a2h
	dq	Int0a3h
	dq	Int0a4h
	dq	Int0a5h
	dq	Int0a6h
	dq	Int0a7h
	dq	Int0a8h
	dq	Int0a9h
	dq	Int0aah
	dq	Int0abh
	dq	Int0ach
	dq	Int0adh
	dq	Int0aeh
	dq	Int0afh

	dq	Int0b0h
	dq	Int0b1h
	dq	Int0b2h
	dq	Int0b3h
	dq	Int0b4h
	dq	Int0b5h
	dq	Int0b6h
	dq	Int0b7h
	dq	Int0b8h
	dq	Int0b9h
	dq	Int0bah
	dq	Int0bbh
	dq	Int0bch
	dq	Int0bdh
	dq	Int0beh
	dq	Int0bfh

	dq	Int0c0h
	dq	Int0c1h
	dq	Int0c2h
	dq	Int0c3h
	dq	Int0c4h
	dq	Int0c5h
	dq	Int0c6h
	dq	Int0c7h
	dq	Int0c8h
	dq	Int0c9h
	dq	Int0cah
	dq	Int0cbh
	dq	Int0cch
	dq	Int0cdh
	dq	Int0ceh
	dq	Int0cfh

	dq	Int0d0h
	dq	Int0d1h
	dq	Int0d2h
	dq	Int0d3h
	dq	Int0d4h
	dq	Int0d5h
	dq	Int0d6h
	dq	Int0d7h
	dq	Int0d8h
	dq	Int0d9h
	dq	Int0dah
	dq	Int0dbh
	dq	Int0dch
	dq	Int0ddh
	dq	Int0deh
	dq	Int0dfh

	dq	Int0e0h
	dq	Int0e1h
	dq	Int0e2h
	dq	Int0e3h
	dq	Int0e4h
	dq	Int0e5h
	dq	Int0e6h
	dq	Int0e7h
	dq	Int0e8h
	dq	Int0e9h
	dq	Int0eah
	dq	Int0ebh
	dq	Int0ech
	dq	Int0edh
	dq	Int0eeh
	dq	Int0efh

	dq	Int0f0h
	dq	Int0f1h
	dq	Int0f2h
	dq	Int0f3h
	dq	Int0f4h
	dq	Int0f5h
	dq	Int0f6h
	dq	Int0f7h
	dq	Int0f8h
	dq	Int0f9h
	dq	Int0fah
	dq	Int0fbh
	dq	Int0fch
	dq	Int0fdh
	dq	Int0feh
	dq	Int0ffh

IntHandlers	endp



CallInt	00h
CallInt	01h
CallInt	02h
CallInt	03h
CallInt	04h
CallInt	05h
CallInt	06h
CallInt	07h
CallInt	08h
CallInt	09h
CallInt	0ah
CallInt	0bh
CallInt	0ch
CallInt	0dh
CallInt	0eh
CallInt	0fh

CallInt	10h
CallInt	11h
CallInt	12h
CallInt	13h
CallInt	14h
CallInt	15h
CallInt	16h
CallInt	17h
CallInt	18h
CallInt	19h
CallInt	1ah
CallInt	1bh
CallInt	1ch
CallInt	1dh
CallInt	1eh
CallInt	1fh

CallInt	20h
CallInt	21h
CallInt	22h
CallInt	23h
CallInt	24h
CallInt	25h
CallInt	26h
CallInt	27h
CallInt	28h
CallInt	29h
CallInt	2ah
CallInt	2bh
CallInt	2ch
CallInt	2dh
CallInt	2eh
CallInt	2fh

CallInt	30h
CallInt	31h
CallInt	32h
CallInt	33h
CallInt	34h
CallInt	35h
CallInt	36h
CallInt	37h
CallInt	38h
CallInt	39h
CallInt	3ah
CallInt	3bh
CallInt	3ch
CallInt	3dh
CallInt	3eh
CallInt	3fh

CallInt	40h
CallInt	41h
CallInt	42h
CallInt	43h
CallInt	44h
CallInt	45h
CallInt	46h
CallInt	47h
CallInt	48h
CallInt	49h
CallInt	4ah
CallInt	4bh
CallInt	4ch
CallInt	4dh
CallInt	4eh
CallInt	4fh

CallInt	50h
CallInt	51h
CallInt	52h
CallInt	53h
CallInt	54h
CallInt	55h
CallInt	56h
CallInt	57h
CallInt	58h
CallInt	59h
CallInt	5ah
CallInt	5bh
CallInt	5ch
CallInt	5dh
CallInt	5eh
CallInt	5fh

CallInt	60h
CallInt	61h
CallInt	62h
CallInt	63h
CallInt	64h
CallInt	65h
CallInt	66h
CallInt	67h
CallInt	68h
CallInt	69h
CallInt	6ah
CallInt	6bh
CallInt	6ch
CallInt	6dh
CallInt	6eh
CallInt	6fh

CallInt	70h
CallInt	71h
CallInt	72h
CallInt	73h
CallInt	74h
CallInt	75h
CallInt	76h
CallInt	77h
CallInt	78h
CallInt	79h
CallInt	7ah
CallInt	7bh
CallInt	7ch
CallInt	7dh
CallInt	7eh
CallInt	7fh

CallInt	80h
CallInt	81h
CallInt	82h
CallInt	83h
CallInt	84h
CallInt	85h
CallInt	86h
CallInt	87h
CallInt	88h
CallInt	89h
CallInt	8ah
CallInt	8bh
CallInt	8ch
CallInt	8dh
CallInt	8eh
CallInt	8fh

CallInt	90h
CallInt	91h
CallInt	92h
CallInt	93h
CallInt	94h
CallInt	95h
CallInt	96h
CallInt	97h
CallInt	98h
CallInt	99h
CallInt	9ah
CallInt	9bh
CallInt	9ch
CallInt	9dh
CallInt	9eh
CallInt	9fh

CallInt	0a0h
CallInt	0a1h
CallInt	0a2h
CallInt	0a3h
CallInt	0a4h
CallInt	0a5h
CallInt	0a6h
CallInt	0a7h
CallInt	0a8h
CallInt	0a9h
CallInt	0aah
CallInt	0abh
CallInt	0ach
CallInt	0adh
CallInt	0aeh
CallInt	0afh

CallInt	0b0h
CallInt	0b1h
CallInt	0b2h
CallInt	0b3h
CallInt	0b4h
CallInt	0b5h
CallInt	0b6h
CallInt	0b7h
CallInt	0b8h
CallInt	0b9h
CallInt	0bah
CallInt	0bbh
CallInt	0bch
CallInt	0bdh
CallInt	0beh
CallInt	0bfh

CallInt	0c0h
CallInt	0c1h
CallInt	0c2h
CallInt	0c3h
CallInt	0c4h
CallInt	0c5h
CallInt	0c6h
CallInt	0c7h
CallInt	0c8h
CallInt	0c9h
CallInt	0cah
CallInt	0cbh
CallInt	0cch
CallInt	0cdh
CallInt	0ceh
CallInt	0cfh

CallInt	0d0h
CallInt	0d1h
CallInt	0d2h
CallInt	0d3h
CallInt	0d4h
CallInt	0d5h
CallInt	0d6h
CallInt	0d7h
CallInt	0d8h
CallInt	0d9h
CallInt	0dah
CallInt	0dbh
CallInt	0dch
CallInt	0ddh
CallInt	0deh
CallInt	0dfh

CallInt	0e0h
CallInt	0e1h
CallInt	0e2h
CallInt	0e3h
CallInt	0e4h
CallInt	0e5h
CallInt	0e6h
CallInt	0e7h
CallInt	0e8h
CallInt	0e9h
CallInt	0eah
CallInt	0ebh
CallInt	0ech
CallInt	0edh
CallInt	0eeh
CallInt	0efh

CallInt	0f0h
CallInt	0f1h
CallInt	0f2h
CallInt	0f3h
CallInt	0f4h
CallInt	0f5h
CallInt	0f6h
CallInt	0f7h
CallInt	0f8h
CallInt	0f9h
CallInt	0fah
CallInt	0fbh
CallInt	0fch
CallInt	0fdh
CallInt	0feh
CallInt	0ffh


CallIntHandlers	proc

	dq	CallInt00h
	dq	CallInt01h
	dq	CallInt02h
	dq	CallInt03h
	dq	CallInt04h
	dq	CallInt05h
	dq	CallInt06h
	dq	CallInt07h
	dq	CallInt08h
	dq	CallInt09h
	dq	CallInt0ah
	dq	CallInt0bh
	dq	CallInt0ch
	dq	CallInt0dh
	dq	CallInt0eh
	dq	CallInt0fh

	dq	CallInt10h
	dq	CallInt11h
	dq	CallInt12h
	dq	CallInt13h
	dq	CallInt14h
	dq	CallInt15h
	dq	CallInt16h
	dq	CallInt17h
	dq	CallInt18h
	dq	CallInt19h
	dq	CallInt1ah
	dq	CallInt1bh
	dq	CallInt1ch
	dq	CallInt1dh
	dq	CallInt1eh
	dq	CallInt1fh

	dq	CallInt20h
	dq	CallInt21h
	dq	CallInt22h
	dq	CallInt23h
	dq	CallInt24h
	dq	CallInt25h
	dq	CallInt26h
	dq	CallInt27h
	dq	CallInt28h
	dq	CallInt29h
	dq	CallInt2ah
	dq	CallInt2bh
	dq	CallInt2ch
	dq	CallInt2dh
	dq	CallInt2eh
	dq	CallInt2fh

	dq	CallInt30h
	dq	CallInt31h
	dq	CallInt32h
	dq	CallInt33h
	dq	CallInt34h
	dq	CallInt35h
	dq	CallInt36h
	dq	CallInt37h
	dq	CallInt38h
	dq	CallInt39h
	dq	CallInt3ah
	dq	CallInt3bh
	dq	CallInt3ch
	dq	CallInt3dh
	dq	CallInt3eh
	dq	CallInt3fh

	dq	CallInt40h
	dq	CallInt41h
	dq	CallInt42h
	dq	CallInt43h
	dq	CallInt44h
	dq	CallInt45h
	dq	CallInt46h
	dq	CallInt47h
	dq	CallInt48h
	dq	CallInt49h
	dq	CallInt4ah
	dq	CallInt4bh
	dq	CallInt4ch
	dq	CallInt4dh
	dq	CallInt4eh
	dq	CallInt4fh

	dq	CallInt50h
	dq	CallInt51h
	dq	CallInt52h
	dq	CallInt53h
	dq	CallInt54h
	dq	CallInt55h
	dq	CallInt56h
	dq	CallInt57h
	dq	CallInt58h
	dq	CallInt59h
	dq	CallInt5ah
	dq	CallInt5bh
	dq	CallInt5ch
	dq	CallInt5dh
	dq	CallInt5eh
	dq	CallInt5fh

	dq	CallInt60h
	dq	CallInt61h
	dq	CallInt62h
	dq	CallInt63h
	dq	CallInt64h
	dq	CallInt65h
	dq	CallInt66h
	dq	CallInt67h
	dq	CallInt68h
	dq	CallInt69h
	dq	CallInt6ah
	dq	CallInt6bh
	dq	CallInt6ch
	dq	CallInt6dh
	dq	CallInt6eh
	dq	CallInt6fh

	dq	CallInt70h
	dq	CallInt71h
	dq	CallInt72h
	dq	CallInt73h
	dq	CallInt74h
	dq	CallInt75h
	dq	CallInt76h
	dq	CallInt77h
	dq	CallInt78h
	dq	CallInt79h
	dq	CallInt7ah
	dq	CallInt7bh
	dq	CallInt7ch
	dq	CallInt7dh
	dq	CallInt7eh
	dq	CallInt7fh

	dq	CallInt80h
	dq	CallInt81h
	dq	CallInt82h
	dq	CallInt83h
	dq	CallInt84h
	dq	CallInt85h
	dq	CallInt86h
	dq	CallInt87h
	dq	CallInt88h
	dq	CallInt89h
	dq	CallInt8ah
	dq	CallInt8bh
	dq	CallInt8ch
	dq	CallInt8dh
	dq	CallInt8eh
	dq	CallInt8fh

	dq	CallInt90h
	dq	CallInt91h
	dq	CallInt92h
	dq	CallInt93h
	dq	CallInt94h
	dq	CallInt95h
	dq	CallInt96h
	dq	CallInt97h
	dq	CallInt98h
	dq	CallInt99h
	dq	CallInt9ah
	dq	CallInt9bh
	dq	CallInt9ch
	dq	CallInt9dh
	dq	CallInt9eh
	dq	CallInt9fh

	dq	CallInt0a0h
	dq	CallInt0a1h
	dq	CallInt0a2h
	dq	CallInt0a3h
	dq	CallInt0a4h
	dq	CallInt0a5h
	dq	CallInt0a6h
	dq	CallInt0a7h
	dq	CallInt0a8h
	dq	CallInt0a9h
	dq	CallInt0aah
	dq	CallInt0abh
	dq	CallInt0ach
	dq	CallInt0adh
	dq	CallInt0aeh
	dq	CallInt0afh

	dq	CallInt0b0h
	dq	CallInt0b1h
	dq	CallInt0b2h
	dq	CallInt0b3h
	dq	CallInt0b4h
	dq	CallInt0b5h
	dq	CallInt0b6h
	dq	CallInt0b7h
	dq	CallInt0b8h
	dq	CallInt0b9h
	dq	CallInt0bah
	dq	CallInt0bbh
	dq	CallInt0bch
	dq	CallInt0bdh
	dq	CallInt0beh
	dq	CallInt0bfh

	dq	CallInt0c0h
	dq	CallInt0c1h
	dq	CallInt0c2h
	dq	CallInt0c3h
	dq	CallInt0c4h
	dq	CallInt0c5h
	dq	CallInt0c6h
	dq	CallInt0c7h
	dq	CallInt0c8h
	dq	CallInt0c9h
	dq	CallInt0cah
	dq	CallInt0cbh
	dq	CallInt0cch
	dq	CallInt0cdh
	dq	CallInt0ceh
	dq	CallInt0cfh

	dq	CallInt0d0h
	dq	CallInt0d1h
	dq	CallInt0d2h
	dq	CallInt0d3h
	dq	CallInt0d4h
	dq	CallInt0d5h
	dq	CallInt0d6h
	dq	CallInt0d7h
	dq	CallInt0d8h
	dq	CallInt0d9h
	dq	CallInt0dah
	dq	CallInt0dbh
	dq	CallInt0dch
	dq	CallInt0ddh
	dq	CallInt0deh
	dq	CallInt0dfh

	dq	CallInt0e0h
	dq	CallInt0e1h
	dq	CallInt0e2h
	dq	CallInt0e3h
	dq	CallInt0e4h
	dq	CallInt0e5h
	dq	CallInt0e6h
	dq	CallInt0e7h
	dq	CallInt0e8h
	dq	CallInt0e9h
	dq	CallInt0eah
	dq	CallInt0ebh
	dq	CallInt0ech
	dq	CallInt0edh
	dq	CallInt0eeh
	dq	CallInt0efh

	dq	CallInt0f0h
	dq	CallInt0f1h
	dq	CallInt0f2h
	dq	CallInt0f3h
	dq	CallInt0f4h
	dq	CallInt0f5h
	dq	CallInt0f6h
	dq	CallInt0f7h
	dq	CallInt0f8h
	dq	CallInt0f9h
	dq	CallInt0fah
	dq	CallInt0fbh
	dq	CallInt0fch
	dq	CallInt0fdh
	dq	CallInt0feh
	dq	CallInt0ffh

CallIntHandlers	endp



END