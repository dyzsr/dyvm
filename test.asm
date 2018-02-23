; function prime
prime:
	irmovq $1, %r8
	irmovq $2, %rcx
.L1:
	rrmovq %rcx, %rax
	imulq %rcx
	cmpq %rax, %rdi
	jl .L3
	rrmovq %rdi, %rax
	cqto
	idivq %rcx
	testq %rdx, %rdx
	jns .L2
	xorq %rax, %rax
	ret
.L2:
	addq %r8, %rcx
	jmp .L1
.L3:
	irmovq $1, %rax
	ret

; function main
main:
	irmovq $1, %r8
	irmovq $100, %r9
	irmovq $2, %rdi
.L4:
	cmpq %rdi, %r9
	jl .L6
	call prime
	testq %rax, %rax
	js .L5
	echo %rdi
.L5:
	addq %r8, %rdi
	jmp .L4
.L6:
	ret
