#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <inttypes.h>


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
 * 10 rrmov rA, rB
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
 * 20 irmov $, rA
 *
 * 30 rmmov rA, $(rB, rI, C)
 * 31 mrmov $(rB, rI, C), rA
 *
 * **** arithmetic & logic ****************************************************
 *
 * 40 add rA, rB
 * 41 sub rA, rB
 * 42 and rA, rB
 * 43 or rA, rB
 * 44 xor rA, rB
 * 45 sal rA, rB [rA is the offset]
 * 46 sar rA, rB
 * 47 shr rA, rB
 * 48 mul rA, rB
 * 49 inc rA
 * 4A dec rA
 * 4B neg rA
 * 4C not rA
 *
 * 50 imul rA [%rax multiplied by rA]
 * 51 umul rA [unsigned]
 * 52 idiv rA [(%rdx, %rax) divided by rA]
 * 53 div rA [unsigned]
 * 60 cqto
 *
 * 70 lea $(rB, rI, C), rA
 *
 * 80 cmp rA, rB
 * 81 test rA, rB
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
*  B0 push rA
*  B1 pop rA
* 
*  **** io ********************************************************************
* 
*  C0 echo rA
* 
*  ***************************************************************************
* 
* */

typedef __int128 int128_t;
typedef unsigned __int128 uint128_t;

int64_t reg[15]; // register id from 0 to E, F stands for none.
// 0: %rax  1: %rcx  2: %rdx  3: %rbx  4: %rsp  5: %rbp  6: %rsi  7: %rdi 
// 8: %r8  9: %r9  10: %r10  11: %r11  12: %r12  13: %r13  14: %r14 

#define MEMORY_SIZE 0x100000
#define PROGRAM_INDEX 0x40000
#define DATA_INDEX 0x80000
uint8_t mem[MEMORY_SIZE];

const int64_t ins_len[16] = { 1, 2, 10, 11, 2, 2, 1, 11, 2, 9, 1, 2, 2 };

#define OK 0
#define HLT 1 // halt
#define ADR 2 // address
#define INS 3 // instruction
#define REG 4 // register
#define LOG 5 // logic

int cond(uint8_t c, uint8_t cc) {
	uint8_t CF = cc >> 3 & 1;
	uint8_t ZF = cc >> 2 & 1;
	uint8_t SF = cc >> 1 & 1;
	uint8_t OF = cc & 1;

	int ret = 1;
	switch (c) {
		case 1: // e
			ret = ZF; break;
		case 2: // ne
			ret = !ZF; break;
		case 3: // s
			ret = SF;	break;
		case 4: // ns
			ret = !SF; break;
		case 5: // g
			ret = !(SF ^ OF) & !ZF;	break;
		case 6: // ge
			ret = !(SF ^ OF);	break;
		case 7: // l
			ret = SF ^ OF; break;
		case 8: // le
			ret = (SF ^ OF) | ZF;	break;
		case 9: // a
			ret = !CF & !ZF; break;
		case 0xA: // ae
			ret = !CF; break;
		case 0xB: // b
			ret = CF;	break;
		case 0xC: // be
			ret = CF | ZF; break;
	}

	return ret;
}

int rrmov(uint8_t ifun, int64_t pc, uint8_t cc) { // rrmovq && cmovXX
	uint8_t byte = mem[pc + 1];
	uint8_t rA = byte >> 4;
	uint8_t rB = byte & 0xf;
	if (rA == 0xf || rB == 0xf) return REG;

	if (!ifun || cond(ifun, cc)) {
		reg[rB] = reg[rA];
	}
	return OK;
}

int irmov(uint8_t ifun, int64_t pc) { // irmovq
	uint8_t rA = mem[pc + 1] >> 4;
	int64_t imme = *(int64_t *)(mem + pc + 2);
	if (rA == 0xf) return REG;
	reg[rA] = imme;
	return OK;
}

int rmmrmov(uint8_t ifun, int64_t pc) { // rmmovq & mrmovq
	uint8_t byte = mem[pc + 1];
	uint8_t rA = byte >> 4;
	uint8_t rB = byte & 0xf;
	byte = mem[pc + 2];
	uint8_t rI = byte >> 4;
	uint8_t coef = byte & 0xf;
	int64_t imme = *(int64_t *)(mem + pc + 3);

	if (rA == 0xf) return REG;
	if (coef != 1 && coef != 2 && coef != 4 && coef != 8) return INS;

	int64_t addr = imme;
	if (rB != 0xf) addr += reg[rB];
	if (rI != 0xf) addr += coef * reg[rI];

	if (addr < DATA_INDEX || addr >= MEMORY_SIZE) return ADR;

	if (ifun == 0) mem[addr] = reg[rA];
	else reg[rA] = mem[addr];

	return OK;
}

int arith(uint8_t ifun, int64_t pc, uint8_t *ptr_cc) { // arithmetic
	if (ifun < 0 || ifun > 0xC) return INS;

	uint8_t byte = mem[pc + 1];
	uint8_t rA = byte >> 4;
	uint8_t rB = byte & 0xf;

	uint8_t cc = 0;

	int64_t valA, valB, res = 0;
	
	if (ifun <= 8) {
		if (rA == 0xf || rB == 0xf) return REG;

		valA = reg[rA], valB = reg[rB];
		int8_t shift;
		uint128_t prod = 0;

		switch (ifun) {
			case 0: // addq
				res = valB + valA;
				if ((uint64_t)res < (uint64_t)valA) cc |= 8;
				if (((valB < 0) == (valA < 0)) && ((valB < 0) != (res < 0))) cc |= 1;
				break;
			case 1: // subq
				res = valB - valA;
				if ((uint64_t)valB >= (uint64_t)valA) cc |= 8;
				if (((valB < 0) != (valA < 0)) && ((valA < 0) == (res < 0))) cc |= 1;
				break;
			case 2: // andq
				res = valB & valA;
				break;
			case 3: // orq
				res = valB | valA;
				break;
			case 4: // xorq
				res = valB ^ valA;
				break;
			case 5: // salq
				shift = valA & 63;
				res = valB << shift;
				cc = shift ? (valB >> (64 - shift) & 1 << 3) : 0;
				break;
			case 6: // sarq
				shift = valA & 63;
				res = valB >> shift;
				cc |= shift ? (valB >> (shift - 1) & 1 << 3) : 0;
				break;
			case 7: // shrq
				shift = valA & 63;
				res = (uint64_t)valB >> (valA & 63);
				cc |= shift ? ((uint64_t)valB >> (shift - 1) & 1 << 3) : 0;
				break;
			case 8: // mul
				prod = (uint128_t)valA * valB;
				res = prod;
				if (prod >> 64) cc |= 9;
				break;
		}
		reg[rB] = res;

	} else {
		if (rA == 0xf) return REG;

		int64_t valA = reg[rA];
		switch (ifun) {
			case 9:
				res = ++valA;
				if (res < valA) cc |= 1;
				break;
			case 0xA:
				res = --valA;
				if (res > valA) cc |= 1;
				break;
			case 0xB:
				res = -valA;
				if (valA) cc |= 8;
				if (valA == (1ll << 63)) cc |= 1;
				break;
			case 0xC:
				res = ~valA;
				break;
		}
		reg[rA] = res;
	}

	cc |= res ? 0 : 4;
	cc |= res < 0 ? 2 : 0;
	*ptr_cc = cc;

	return OK;
}

int mul_div(uint8_t ifun, int64_t pc) { // imulq, mulq, idivq, divq
	if (ifun < 0 || ifun >= 4) return INS;

	uint8_t rA = mem[pc + 1] >> 4;
	if (rA == 0xf) return REG;
	int64_t valA = reg[rA];

	if (ifun == 0) { // imulq
		int128_t res = (int128_t)valA * reg[0]; // reg[rA] * reg[%rax]
		reg[0] = (int64_t)res;
		reg[2] = res >> 64;
	} else if (ifun == 1) { // mulq
		uint128_t res = (uint128_t)valA * reg[0]; // reg[rA] * reg[%rax]
		reg[0] = (uint64_t)res;
		reg[2] = res >> 64;
	} else { // divq & idivq
		if (valA == 0) return LOG;
		int128_t divd = reg[2];
		divd = (divd << 64) | reg[0];
		if (ifun == 2) {
			reg[0] = divd / valA;
			reg[2] = divd % valA;
		} else {
			reg[0] = (uint128_t)divd / valA;
			reg[2] = (uint128_t)divd % valA;
		}
	}
	return OK;
}

int cqto() {
	reg[2] = reg[0] < 0 ? (uint64_t)-1 : 0;
	return OK;
}

int lea(uint8_t ifun, int64_t pc) {
	uint8_t byte = mem[pc + 1];
	uint8_t rA = byte >> 4;
	uint8_t rB = byte & 0xf;
	byte = mem[pc + 2];
	uint8_t rI = byte >> 4;
	uint8_t coef = byte & 0xf;
	int64_t imme = *(int64_t *)(mem + pc + 3);
	if (rA == 0xf) return REG;

	int64_t addr = imme;
	if (rB != 0xf) addr += reg[rB];
	if (rI != 0xf) addr += reg[rI] * coef;
	reg[rA] = addr;

	return OK;
}

int cmp_test(uint8_t ifun, int64_t pc, uint8_t *ptr_cc) {
	if (ifun != 0 && ifun != 1) return INS;

	uint8_t byte = mem[pc + 1];
	uint8_t rA = byte >> 4;
	uint8_t rB = byte & 0xf;
	if (rA == 0xf || rB == 0xf) return REG;

	int64_t valA = reg[rA], valB = reg[rB];
	int64_t res;

	uint8_t cc = 0;

	if (ifun == 0) {
		res = valB - valA;
		if ((uint64_t)valB >= (uint64_t)valA) cc |= 8;
		if (((valB < 0) != (valA < 0)) && ((valA < 0) == (res < 0))) cc |= 1;
	} else {
		res = valB & valA;
	}

	cc |= res ? 0 : 4;
	cc |= res < 0 ? 2 : 0;
	*ptr_cc = cc;

	return OK;
}

int jump(uint8_t ifun, int64_t *ptr_pc, uint8_t cc) {
	int64_t pc = *ptr_pc;
	if (ifun < 0 || ifun > 0xD) return INS;

	int64_t imme = *(int64_t *)(mem + pc + 1);
	if (ifun < 0xD) {
		if (!ifun || cond(ifun, cc)) pc = imme + PROGRAM_INDEX - 9;
	} else {
		if (reg[4] < 8 || reg[4] >= MEMORY_SIZE) return ADR;
		*(int64_t *)(mem + reg[4] - 8) = pc + 9;
		reg[4] -= 8;
		pc = imme + PROGRAM_INDEX - 9;
	}
	*ptr_pc = pc;
	return OK;
}

int ret(int64_t *ptr_pc) {
	if (reg[4] < 0 || reg[4] >= PROGRAM_INDEX) return ADR;
	*ptr_pc = *(int64_t *)(mem + reg[4]) - 1;
	reg[4] += 8;
	return OK;
}

int push_pop(uint8_t ifun, int64_t pc) {
	if (ifun != 0 && ifun != 1) return INS;

	uint8_t rA = mem[pc + 1] >> 4;
	if (rA == 0xf) return REG;
	
	if (ifun == 0) {
		if (reg[4] < 8 || reg[4] >= MEMORY_SIZE) return ADR;
		*(int64_t *)(mem + reg[4] - 8) = reg[rA];
		reg[4] -= 8;
	} else {
		if (reg[4] < 0 || reg[4] >= MEMORY_SIZE - 8) return ADR;
		reg[rA] = *(int64_t *)(mem + reg[4]);
		reg[4] += 8;
	}
	return OK;
}

int echo(uint8_t ifun, int64_t pc) {
	if (ifun < 0 || ifun > 1) return INS;

	uint8_t rA = mem[pc + 1] >> 4;
	if (rA == 0xf) return REG;
	
	if (ifun == 0) {
		printf("%" PRId64 "\n", reg[rA]);
	} else {
		printf("%" PRIu64 "\n", reg[rA]);
	}
	return OK;
}

int next(int64_t *ptr_pc, uint8_t *ptr_cc) {
	int state = OK;
	int64_t pc = *ptr_pc;
	uint8_t cc = *ptr_cc;
	// pc address error
	if (pc < PROGRAM_INDEX || pc >= DATA_INDEX) return ADR;

	uint8_t op_code = mem[pc];
	uint8_t icode = op_code >> 4;
	uint8_t ifun = op_code & 0xf;

	if (pc + ins_len[icode] >= DATA_INDEX) return ADR;

//	fprintf(stderr, "[%1x:%1x]\n", icode, ifun);
	switch (icode) {
		case 0:
			if (ifun == 0) state = HLT; break;
		case 1:
			state = rrmov(ifun, pc, cc); break;
		case 2:
			state = irmov(ifun, pc); break;
		case 3:
			state = rmmrmov(ifun, pc); break;
		case 4:
			state = arith(ifun, pc, &cc); break;
		case 5:
			state = mul_div(ifun, pc); break;
		case 6:
			state = cqto(); break;
		case 7:
			state = lea(ifun, pc); break;
		case 8:
			state = cmp_test(ifun, pc, &cc); break;
		case 9:
			state = jump(ifun, &pc, cc); break;
		case 0xA:
			state = ret(&pc); break;
		case 0xB:
			state = push_pop(ifun, pc); break;
		case 0xC:
			state = echo(ifun, pc); break;
		default:
			state = INS; break;
	}

	if (!state) {
		pc += ins_len[icode];
		*ptr_pc = pc;
		*ptr_cc = cc;
	}
	return state;
}

int load(char *s) {
	FILE *fpi = fopen(s, "r");
	if (!fpi) return 0;
	size_t num = fread(mem + PROGRAM_INDEX, 1, DATA_INDEX - PROGRAM_INDEX, fpi);
	if (num == 0) return 0;
	return 1;
}

int run() {
	int64_t pc = PROGRAM_INDEX;
	uint8_t cc = 0;
	reg[4] = 0x10000; // initial stack size
	int state = OK;
	do {
		state = next(&pc, &cc);
//		fprintf(stderr, "%#" PRIx64 ": %02x %1x\n", pc, mem[pc], cc);
	} while (state == OK);
	return state;
}

int main(int argc, char *argv[]) {
	if (argc != 2) {
		fprintf(stderr, "Too bad !\n");
		exit(0);
	}

	if (!load(argv[1])) {
		fprintf(stderr, "Failed to load file %s.\n", argv[1]);
		exit(0);
	}

	int exception = run();
	if (exception == HLT) fprintf(stderr, "Program ended successfully.\n");
	else if (exception == ADR) fprintf(stderr, "Illegal memory address !\n");
	else if (exception == INS) fprintf(stderr, "Invalid instruction !\n");
	else if (exception == REG) fprintf(stderr, "Illegal register id !\n");
	else if (exception == LOG) fprintf(stderr, "Logic error occured !\n");

	return 0;
}
