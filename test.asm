; function prime
prime:
	irmov $1, %r8
	irmov $2, %rcx
.L1:
	rrmov %rcx, %rax
	mul %rcx, %rax
	cmp %rax, %rdi
	jl .L3
	rrmov %rdi, %rax
	cqto
	idiv %rcx
	test %rdx, %rdx
	jne .L2
	xor %rax, %rax
	ret
.L2:
	add %r8, %rcx
	jmp .L1
.L3:
	irmov $1, %rax
	ret

; function main
main:
	irmov $1, %r10
	irmov $100, %r11
	irmov $2, %rdi
.L4:
	cmp %rdi, %r11
	jl .L6
	call prime
	test %rax, %rax
	je .L5
	echo %rdi
.L5:
	add %r10, %rdi
	jmp .L4
.L6:
	halt
