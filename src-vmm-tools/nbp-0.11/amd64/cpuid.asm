.CODE
GetCpuIdInfo PROC
	push	rbp
	mov		rbp, rsp
	push	rbx
	push	rsi

	mov		[rbp+18h], rdx
	mov		eax, ecx
	cpuid
	mov		rsi, [rbp+18h]
	mov		[rsi], eax
	mov		[r8], ebx
	mov		[r9], ecx
	mov		rsi, [rbp+30h]
	mov		[rsi], edx	

	pop		rsi
	pop		rbx
	mov		rsp, rbp
	pop		rbp
	ret
GetCpuIdInfo ENDP
END
