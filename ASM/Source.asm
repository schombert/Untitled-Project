_TEXT segment

asm_div PROC
	mov rax, rcx
	idiv r8
	ret
asm_div ENDP

_TEXT ends

END