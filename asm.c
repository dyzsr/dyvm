#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
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
*  C0 iecho rA
*  C1 echo rA
* 
*  ***************************************************************************
* 
* */

// DEFINATIONS ================================================================

//typedef unsigned char uint8_t;

const char *ins_id[16][16] = {
	{ "halt", "nop", },
	{ "rrmov", "cmove", "cmovne", "cmovs", "cmovns", "cmovg", "cmovge",
		"cmovl", "cmovle", "cmova", "cmovae", "cmovb", "cmovbe", },
	{ "irmov", },
	{ "rmmov", "mrmov", },
	{ "add", "sub", "and", "or", "xor", "sal", "sar", "shr", "mul", 
		"inc", "dec", "neg", "not",},
	{ "imul", "umul", "idiv", "div", },
	{ "cqto", },
	{ "lea", },
	{ "cmp", "test", },
	{ "jmp", "je", "jne", "js", "jns", "jg", "jge", 
		"jl", "jle", "ja", "jae", "jb", "jbe", "call", },
	{ "ret", },
	{ "push", "pop", },
	{ "iecho", "echo", },
};

// instruction types
// 0: null
// 1: l
// 2: r
// 3: rr
// 4: ir
// 5: rm
// 6: mr

const int ins_type[16][16] = {
	{ 0, 0, },
	{ 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, },
	{ 4, },
	{ 5, 6, },
	{ 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 2, 2, 2},
	{ 2, 2, 2, 2, },
	{ 0, },
	{ 6, },
	{ 3, 3, },
	{ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, },
	{ 0, },
	{ 2, 2, },
	{ 2, 2, },
};

#define REG_NUM 15

const char *reg_id[REG_NUM] = {
	"%rax", "%rcx", "%rdx", "%rbx", "%rsp", "%rbp", "%rsi", "%rdi",
	"%r8", "%r9", "%r10", "%r11", "%r12", "%r13", "%r14",
};

#define MAX_SIZE 100000
#define MAX_LINE_WIDTH 100

char input[MAX_SIZE];
uint8_t output[MAX_SIZE];


// ERRORS =====================================================================

#define OK 0
#define ERROR_INCORRECT_FORMAT -1
#define ERROR_NO_INPUT_FILE -2
#define ERROR_SIZE_EXCEED -3
#define ERROR_WIDTH_EXCEED -4
#define ERROR_INVALID_IMMEDIATE_FORMAT -5
#define ERROR_INVALID_IDENTIFIER -6
#define ERROR_INVALID_LABEL_FORMAT -7
#define ERROR_INVALID_REGISTER_ID -8
#define ERROR_INVALID_MEMORY_ADDRESS_FORMAT -9
#define ERROR_AMBIGUOUS_LABELS -10
#define ERROR_LABEL_NUM_EXCEED -11
#define ERROR_NO_SUCH_LABEL -12
#define ERROR_INCOMPLETE_INPUT -13

void error_incorrect_format() {
	fprintf(stderr, "Incorrect input format !\n");
	exit(0);
}

void error_no_input_file() {
	fprintf(stderr, "No such input file !\n");
	exit(0);
}

void error_size_exceed() {
	fprintf(stderr, "Code size exceeds !\n");
	exit(0);
}

void error_width_exceed() {
	fprintf(stderr, "Code width exceeds !\n");
	exit(0);
}

void error_invalid_immediate_format() {
	fprintf(stderr, "Invalid immediate format !\n");
	exit(0);
}

void error_invalid_identifier() {
	fprintf(stderr, "Invalid identifier !\n");
	exit(0);
}

void error_invalid_label_format() {
	fprintf(stderr, "Invalid label format !\n");
	exit(0);
}

void error_invalid_reg_id() {
	fprintf(stderr, "Invalid register identifier !\n");
	exit(0);
}

void error_invalid_mem_addr_format() {
	fprintf(stderr, "Invalid memory address format !\n");
	exit(0);
}

void error_ambiguous_label() {
	fprintf(stderr, "Ambiguous labels !\n");
	exit(0);
}

void error_no_such_label() {
	fprintf(stderr, "Undeclared label !\n");
	exit(0);
}

void error_label_num_exceed() {
	fprintf(stderr, "Too many labels !\n");
	exit(0);
}

void error_incomplete_input() {
	fprintf(stderr, "Incomplete input !\n");
	exit(0);
}

// ============================================================================

void read_file() {
	char tmp[MAX_LINE_WIDTH];
	int length = strlen(input);
	while (fgets(tmp, MAX_LINE_WIDTH, stdin)) {
		for (char *s = tmp; *s; s++) {
			if (*s == ';') {
				*s = 0;
				break;
			}
		}
		int len = strlen(tmp);
		length += len;
		if (length > MAX_SIZE) {
			error_size_exceed();
		}
		strcat(input, tmp);
	}
}

void preprocess() {
	char *s = input, *t;
	for (; *s; s++) {
		char c = *s;
		if (c == '\n' || c == '\r' || c == '\t') {
			*s = ' ';
		} else if (c == ',' || c == '(' || c == ')' || c == ':') {
			t = s - 1;
			for (; t >= input; t--) {
				if (*t != ' ') break;
				*(t + 1) = *t;
			}
			*(t + 1) = c;
		}
	}

	s = t = input;
	int flag = 0;
	for (; *s; s++) {
		char c = *s;
		if (c == ' ' || c == ',' || c == '(' || c == ')') {
			if (!flag) {
				*(t++) = c;
				flag = 1;
			}
		} else {
			flag = 0;
			*(t++) = *s;
		}
	}
	*t = 0;
}

int eat_blank(char **src) {
	char *s = *src;
	while (*s == ' ') s++;
	if (!*s) return 0;
	*src = s;
	return 1;
}

int read_ins_id(uint8_t *dest, char **src) {
	char *s = *src;
	int k;
	for (k = 0; s[k] && s[k] != ' '; k++);
	char c = s[k];
	s[k] = 0;
	printf("%s\n", s);

	*dest = 0xff;
	for (int i = 0; i < 16; i++) {
		for (int j = 0; j < 16 && ins_id[i][j]; j++) {
			if (strcmp(ins_id[i][j], s) == 0) {
				printf("(%d %d %s)\n", i, j, ins_id[i][j]);
				*dest = (i << 4 & 0xff) | (j & 0xf);
				break;
			}
		}
	}
	s += k;
	*s = c;
	*src = ++s;

	return OK;
}

int read_label(char *dest, char **src) {
	char *s = *src;
	if (!(isalnum(*s) || *s == '.')) return ERROR_INVALID_LABEL_FORMAT;
	*(dest++) = *s;
	for (s++; *s && *s != ' '; s++) {
		if (!isalnum(*s)) return ERROR_INVALID_LABEL_FORMAT;
		*(dest++) = *s;
	}
	*dest = 0;
	*src = ++s;

	return OK;
}

int read_matching_label(char *dest, char **src) {
	char *s = *src;
	if (!(isalnum(*s) || *s == '.')) return ERROR_INVALID_IDENTIFIER;
	*(dest++) = *s;
	for (s++; *s != ':'; s++) {
		if (!isalnum(*s)) return ERROR_INVALID_IDENTIFIER;
		*(dest++) = *s;
	}
	*dest = 0;
	*src = ++s;

	return OK;
}

int read_immediate(int64_t *dest, char **src) {
	char *s = *src;
	if (*s != '$') return ERROR_INVALID_IMMEDIATE_FORMAT;
	s++;

	int64_t val = 0, sgn = 0;
	if (*s == '-') sgn = 1, s++;

	int base = 10;
	if (*s == '0') {
		s++;
		if (*s == 'x') base = 16, s++;
	}

	if (base == 10) {
		for (; *s && *s != ' ' && *s != ','; s++) {
			char c = *s;
			if (!isdigit(c)) return ERROR_INVALID_IMMEDIATE_FORMAT;
			val = val * 10 + c - '0';
		}
	} else {
		for (; *s && *s != ' ' && *s != ','; s++) {
			char c = *s;
			if (!isxdigit(c)) return ERROR_INVALID_IMMEDIATE_FORMAT;
			val <<= 4;
			if (isupper(c)) val |= c - 'A' + 10;
			else if (islower(c)) val |= c - 'a' + 10;
			else val |= c - '0';
		}
	}
	if (sgn) val = -val;
	*dest = val;

	*src = ++s;

	return OK;
}

int read_reg_id(uint8_t *dest, char **src) {
	char *s = *src;
	int k = 0;
	for (; s[k] && s[k] != ' ' && s[k] != ','; k++);
	char c = s[k];
	s[k] = 0;

	*dest = 0xff;
	for (int i = 0; i < REG_NUM; i++) {
		if (strcmp(reg_id[i], s) == 0) {
			*dest = i;
			break;
		}
	}
	s += k;
	*s = c;
	*src = ++s;

	if (*dest == 0xff) return ERROR_INVALID_REGISTER_ID;
	return OK;
}

int read_mem_addr(int64_t *I, uint8_t *rB, uint8_t *rI, uint8_t *C, char **src) {
	int64_t t_I = 0;
	uint8_t t_rB = 0xff, t_rI = 0xff, t_C = 1;

	char *s = *src;

	// immediate number
	int base = 10;
	if (*s == '0') {
		s++;
		if (*s == 'x') base = 16, s++;
	}
	if (base == 10) {
		for (; *s && *s != '('; s++) {
			char c = *s;
			if (!isdigit(c)) return ERROR_INVALID_MEMORY_ADDRESS_FORMAT;
			t_I = t_I * 10 + c - '0';
		}
	} else {
		for (; *s && *s != '('; s++) {
			char c = *s;
			if (!isxdigit(c)) return ERROR_INVALID_MEMORY_ADDRESS_FORMAT;
			t_I <<= 4;
			if (islower(c)) t_I |= c - 'a' + 10;
			else if (isupper(c)) t_I |= c - 'A' + 10;
			else t_I |= c - '0';
		}
	}
	assert(*s == '(');
	s++;

	// base register
	char c = *s;
	int k = 0;

	if (c == ',') goto L1;
	if (c == '(') goto L3;

	for (; s[k] && s[k] != ',' && s[k] != ')'; k++);
	c = s[k];
	s[k] = 0;
	uint8_t tmp = 0xff;
	for (int i = 0; i < REG_NUM; i++) {
		if (strcmp(reg_id[i], s) == 0) {
			tmp = i;
			break;
		}
	}
	if (tmp == 0xff) return ERROR_INVALID_MEMORY_ADDRESS_FORMAT;
	t_rB = tmp;
	s += k;
	*s = c;

L1: // index register
	if (c == ')') goto L3;

	assert(c == ',');
	if (*(++s) == ',') goto L2;

	for (k = 0; s[k] && s[k] != ',' && s[k] != ')'; k++);
	c = s[k];
	s[k] = 0;
	tmp = 0xff;
	for (int i = 0; i < REG_NUM; i++) {
		if (strcmp(reg_id[i], s) == 0) {
			tmp = i;
			break;
		}
	}
	if (tmp == 0xff) return ERROR_INVALID_MEMORY_ADDRESS_FORMAT;
	t_rI = tmp;
	s += k;
	*s = c;

L2: // Coefficient
	if (c == ')') goto L3;

	c = *(++s);
	t_C = c - '0';
	if (t_C != 1 && t_C != 2 && t_C != 4 && t_C != 8) {
		return ERROR_INVALID_MEMORY_ADDRESS_FORMAT;
	}

	c = *(++s);
	if (c != ')') return ERROR_INVALID_MEMORY_ADDRESS_FORMAT;

L3:
	*src = ++s;
	*I = t_I;
	*rB = t_rB;
	*rI = t_rI;
	*C = t_C;

	return OK;
}

// LABEL ======================================================================

#define MAX_LABEL_NUM 512
#define MAX_LABEL_LENGTH 64

struct Label {
	char identifier[MAX_LABEL_LENGTH];
	int64_t index;
	int flag;
} dest_labels[MAX_LABEL_NUM],
	src_labels[MAX_LABEL_NUM * 2];

int add_label(char *iden, int64_t index) {
	int i;
	for (i = 0; i < MAX_LABEL_NUM; i++) {
		char *s = dest_labels[i].identifier;
		if (*s == 0) {
			strcpy(s, iden);
			dest_labels[i].index = index;
			break;
		}
		if (strcmp(s, iden) == 0) {
			return ERROR_AMBIGUOUS_LABELS;
		}
	}
	if (i == MAX_LABEL_NUM) return ERROR_LABEL_NUM_EXCEED;
	return OK;
}

int find_label(char *iden, int64_t *index) {
	int64_t idx = -1;
	for (int i = 0; i < MAX_LABEL_NUM; i++) {
		char *s = dest_labels[i].identifier;
		if (*s == 0) break;
		if (strcmp(s, iden) == 0) {
			idx = dest_labels[i].index;
			break;
		}
	}
	if (idx == -1) return ERROR_NO_SUCH_LABEL;
	*index = idx;
	return OK;
}

int add_src_label(char *iden, int64_t index) {
	const static int max_label_num = MAX_LABEL_NUM * 2;
	static int label_num = 0;
	if (label_num >= max_label_num) return ERROR_LABEL_NUM_EXCEED;

	strcpy(src_labels[label_num].identifier, iden);
	src_labels[label_num].index = index;
	label_num++;
	return OK;
}

int replace_labels() {
	const static int max_label_num = MAX_LABEL_NUM * 2;
	for (int i = 0; i < max_label_num; i++) {
		char *s = src_labels[i].identifier;
		if (*s == 0) break;
		printf("%s\n", s);
		for (int j = 0; j < MAX_LABEL_NUM; j++) {
			char *t = dest_labels[j].identifier;
			if (!*t) break;
			if (strcmp(s, t) == 0) {
				int64_t index1 = src_labels[i].index;
				src_labels[i].flag = 1;
				int64_t index2 = dest_labels[j].index;
				dest_labels[j].flag = 1;
				int64_t alter = index2;
				printf("$ %ld %ld %ld $\n", index1, index2, alter);
				int64_t *p = (int64_t *)(output + index1 + 1);
				*p = alter;
			}
		}
	}
	for (int i = 0; i < max_label_num; i++) {
		char *s = src_labels[i].identifier;
		if (!*s) break;
		if (!src_labels[i].flag) return ERROR_NO_SUCH_LABEL;
	}
	for (int i = 0; i < MAX_LABEL_NUM; i++) {
		char *t = dest_labels[i].identifier;
		if (!*t) break;
		if (!dest_labels[i].flag) return ERROR_NO_SUCH_LABEL;
	}
	return OK;
}

// INTERPRET ==================================================================

int64_t interpret() {
	char *src = input, *tmp = input;
	char q = 'I';
	// I: instruction: icode & ifun
	// L: destination label

	int64_t index = 0;

	char label[MAX_LABEL_LENGTH];
	int verdict = 0;

	while (eat_blank(&src)) {
		tmp = src;

		printf("[%p %ld]\n", src, index);
		verdict = 0;

		if (q == 'I') {
			uint8_t op;
			verdict = read_ins_id(&op, &src);
			if (op == 0xff) {
				q = 'L';
				src = tmp;
			} else {
				int type = ins_type[op >> 4 & 0xff][op & 0xf];	
				printf("<<%d>>\n", type);

				int64_t I, *p;
				uint8_t rA, rB, rI, C;

				switch (type) {
					case 0: // null
						if (index + 1 > MAX_SIZE) error_size_exceed();
						output[index++] = op;
						break;

					case 1: // l
						if (index + 9 > MAX_SIZE) error_size_exceed(); 
						output[index++] = op;

						if (!eat_blank(&src)) error_incomplete_input();

						verdict = read_label(label, &src);
						if (verdict) goto outside;

						add_src_label(label, index - 1);

						index += 8;
						break;

					case 2: // r
						if (index + 2 > MAX_SIZE) error_size_exceed();
						output[index++] = op;

						if (!eat_blank(&src)) error_incomplete_input();

						verdict = read_reg_id(&rA, &src);
						if (verdict) goto outside;

						output[index++] = (rA << 4 & 0xff) | 0xf;

						break;

					case 3: // rr
						if (index + 2 > MAX_SIZE) error_size_exceed();
						output[index++] = op;

						if (!eat_blank(&src)) error_incomplete_input();
						verdict = read_reg_id(&rA, &src);
						if (verdict) goto outside;

						if (!eat_blank(&src)) error_incomplete_input();
						verdict = read_reg_id(&rB, &src);
						if (verdict) goto outside;

						output[index++] = (rA << 4 & 0xff) | (rB & 0xf);

						break;

					case 4: // ir
						if (index + 10 > MAX_SIZE) error_size_exceed();
						output[index++] = op;

						if (!eat_blank(&src)) error_incomplete_input();
						verdict = read_immediate(&I, &src);
						if (verdict) goto outside;

						if (!eat_blank(&src)) error_incomplete_input();
						verdict = read_reg_id(&rA, &src);
						if (verdict) goto outside;

						output[index++] = (rA << 4 & 0xff) | 0xf;
						p = (int64_t *)(output + index);
						*p = I;
						index += 8;

						printf("%x %ld %d\n", op, I, rA);

						break;

					case 5: // rm
						if (index + 11 > MAX_SIZE) error_size_exceed();
						output[index++] = op;

						if (!eat_blank(&src)) error_incomplete_input();
						verdict = read_reg_id(&rA, &src);
						if (verdict) goto outside;

						if (!eat_blank(&src)) error_incomplete_input();
						verdict = read_mem_addr(&I, &rB, &rI, &C, &src);
						if (verdict) goto outside;

						output[index++] = (rA << 4 & 0xff) | (rB & 0xf);
						output[index++] = (rI << 4 & 0xff) | (C & 0xf);
						p = (int64_t *)(output + index);
						*p = I;
						index += 8;
						
						break;

					case 6: // mr
						if (index + 11 > MAX_SIZE) error_size_exceed();
						output[index++] = op;

						if (!eat_blank(&src)) error_incomplete_input();
						verdict = read_mem_addr(&I, &rB, &rI, &C, &src);
						if (verdict) goto outside;

						if (!eat_blank(&src)) error_incomplete_input();
						verdict = read_reg_id(&rA, &src);
						if (verdict) goto outside;

						output[index++] = (rA << 4 & 0xff) | (rB & 0xf);
						output[index++] = (rI << 4 & 0xff) | (C & 0xf);
						p = (int64_t *)(output + index);
						*p = I;
						index += 8;

						break;
				}

			}

		} else if (q == 'L') {
			puts("L");
			verdict = read_matching_label(label, &src);
			if (verdict) break;
			add_label(label, index);
			q = 'I';
		}

	}

	verdict = replace_labels();

outside:
	// deal with exceptions
	printf("verdict = %d\n", verdict);
	switch (verdict) {

		case ERROR_INVALID_IDENTIFIER:
			error_invalid_identifier();
			break;

		case ERROR_INVALID_LABEL_FORMAT:
			error_invalid_label_format();
			break;

		case ERROR_INVALID_IMMEDIATE_FORMAT:
			error_invalid_immediate_format();
			break;

		case ERROR_INVALID_REGISTER_ID:
			error_invalid_reg_id();
			break;

		case ERROR_INVALID_MEMORY_ADDRESS_FORMAT:
			error_invalid_mem_addr_format();
			break;

		case ERROR_AMBIGUOUS_LABELS:
			error_ambiguous_label();
			break;

		case ERROR_NO_SUCH_LABEL:
			error_no_such_label();
			break;

		case ERROR_LABEL_NUM_EXCEED:
			error_label_num_exceed();
			break;
	}

	return index;
}

// OUTPUT =====================================================================

void write_file(char *argv1, int64_t size) {
	int length = strlen(argv1);
	for (int i = length - 1; i >= 0; i--) {
		if (argv1[i] == '.') {
			argv1[i] = 0;
			break;
		}
	}

	char filename[128];
	strcpy(filename, argv1);
	strcat(filename, "_hex");
	FILE *fpo1 = fopen(filename, "w");
	if (!fpo1) exit(0);
	
	strcpy(filename, argv1);
	strcat(filename, "_bin");
	FILE *fpo2 = fopen(filename, "w");
	if (!fpo2) exit(0);

	int64_t idx = 0;
	while (idx < size) {
		uint8_t op = output[idx];
		int type = ins_type[op >> 4 & 0xff][op & 0xf];

		int n = 1;
		if (type == 1) n += 8;
		else if (type == 2 || type == 3) n += 1;
		else if (type == 4) n += 9;
		else if (type == 5 || type == 6) n += 10;


		fprintf(fpo1, "%#04lx:", idx);
		for (int i = 0; i < n; i++, idx++) {
			fprintf(fpo1, " %02x", output[idx]);
		}
		fprintf(fpo1, "\n");
	}
	fclose(fpo1);

	fwrite(output, sizeof(char), size, fpo2);
	fclose(fpo2);
}

// ============================================================================

int main(int argc, char *argv[]) {
	if (argc != 2) {
		error_incorrect_format();
	}

	if (!freopen(argv[1], "r", stdin)) {
		error_no_input_file();
	}

	strcpy(input, "jmp main ");
	read_file();
	preprocess();

	int64_t size = interpret();
	write_file(argv[1], size);

	return 0;
}
