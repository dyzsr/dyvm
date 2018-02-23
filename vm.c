#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>


/* ****************************************************************************
 * 
 *     DY86-64 ISA (Based on Y86-64 described in CS:APP)
 *
 * ****************************************************************************
 *
 * 00 halt
 * 01 nop
 *
 * **** transmit **************************************************************
 *
 * 10 rrmovq rA, rB
 *
 * 11 cmove rA, rB
 * 12 cmovne rA, rB
 *
 * 13 cmovs rA, rB
 * 14 cmovns rA, rB
 *
 * 15 cmovg rA, rB
 * 16 cmovge rA, rB
 * 17 cmovl rA, rB
 * 18 cmovle rA, rB
 *
 * 19 cmova rA, rB
 * 1A cmovae rA, rB
 * 1B cmovb rA, rB
 * 1C cmovbe rA, rB
 *
 * 20 irmovq $, rA
 *
 * 30 rmmovq rA, $(rB, rI, C)
 * 31 mrmovq $(rB, rI, C), rA
 *
 * **** arithmetic & logic ****************************************************
 *
 * 40 addq rA, rB
 * 41 subq rA, rB
 * 42 andq rA, rB
 * 43 orq rA, rB
 * 44 xorq rA, rB
 * 45 notq rA
 * 46 salq rA, rB [rA is the offset]
 * 47 sarq rA, rB
 * 48 shrq rA, rB
 *
 * 50 imulq rA [%rax multiplied by rA]
 * 51 mulq rA [unsigned]
 * 52 idivq rA [(%rdx, %rax) divided by rA]
 * 53 divq rA [unsigned]
 * 60 cqto
 *
 * 70 leaq $(rB, rI, C), rA
 *
 * 80 cmpq rA, rB
 * 81 testq rA, rB
 *
 * **** jump ******************************************************************
 *
 * 90 jmp Label
 *
 * 91 je Label
 * 92 jne Label
 *
 * 93 js Label
 * 94 jns Label
 *
 * 95 jg Label
 * 96 jge Label
 * 97 jl Label
 * 98 jle Label
*
*  99 ja Label
*  9A jae Label
*  9B jb Label
*  9C jbe Label
* 
*  9D call Label
* 
*  A0 ret
* 
*  **** stack *****************************************************************
* 
*  B0 pushq rA
*  B1 popq rA
* 
*  **** io ********************************************************************
* 
*  C0 echo rA
* 
*  ***************************************************************************
* 
* */


long reg[15]; // register id from 0 to E, F stands for none.

#define MEMORY_SIZE 1048576
long mem[MEMORY_SIZE];


int main() {
	return 0;
}
