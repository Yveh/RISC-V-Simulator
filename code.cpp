#include <bits/stdc++.h>

typedef unsigned int ui;
char mem[0x20000];
ui x[32], pc;

char getch() {
	char ch = getchar();
	while (ch == ' ' || ch == '\n')
		ch = getchar();
	return ch;
}
ui toInt(char x) {
	if ('A' <= x && x <= 'F')
		return x - 'A' + 10;
	else
		return x - '0';
}
void readInst() {
	ui ind = 0;
	ui offset = 0;
	bool flag = 0;
	ui sta = 0;
	ui tmp;
	for (char ch = getch(); ch != EOF; ch = getch()) {
		if (ch == '@') {
			flag =  1;
			ind = 0;
			offset = 0;
		}
		else {
			if (flag) {
				ind = ind << 4 | toInt(ch);
				++sta;
				if (sta == 8) {
					flag = 0;
					sta = 0;
				}
			}
			else {
				memcpy(&tmp, &mem[ind + offset], sizeof(ui));
				tmp = toInt(ch) << ((sta ^ 1) << 2) | tmp;
				memcpy(&mem[ind + offset], &tmp, sizeof(ui));
				++sta;
				if (sta == 8) {
					offset += 4;
					sta = 0;
				}
			}
		}
	}
}

ui getbin(ui x, ui l, ui r) {
	return (x >> l) & (x << 31 - r >> 31 - r >> l);
}

int getIimm(ui x) {
	return int(getbin(x, 20, 31)) << 20 >> 20;
}

int getUimm(ui x) {
	return int(getbin(x, 12, 31)) << 12;
}

int getSimm(ui x) {
	return int(getbin(x, 25, 31) << 5 | getbin(x, 7, 11)) << 20 >> 20;
}

int getBimm(ui x) {
	return int(getbin(x, 31, 31) << 11 | getbin(x, 7, 7) << 10 | getbin(x, 25, 30) << 4 | getbin(x, 8, 11)) << 20 >> 19;
}

int getJimm(ui x) {
	return int(getbin(x, 31, 31) << 19 | getbin(x, 12, 19) << 11 | getbin(x, 20, 20) << 10 | getbin(x, 21, 30)) << 12 >> 11; 
}
int main() {
	readInst();
	/*
	for (auto i : mem)
		printf("%X %08X\n", i.first, i.second);
	*/
	pc = 0x00000000;
	ui cinst, opcode, rd, rs1, rs2, funct3, funct7, shamt, tmp;
	int imm;
	while (1) {
		memcpy(&cinst, &mem[pc], sizeof(ui));
		if (cinst == 0x00c68223)
		{
			printf("%u", x[10] & ((1 << 8) - 1));
			//printf("%d\n", x[10]);
			return 0;
		}
		opcode = getbin(cinst, 0, 6);
		switch (opcode) {
		case 0b0110111:
			//LUI
			imm = getUimm(cinst);
			rd = getbin(cinst, 7, 11);
			if (rd)
				x[rd] = imm;
			break;
		case 0b0010111:
			//AUIPC
			imm = getUimm(cinst);
			rd = getbin(cinst, 7, 11);
			if (rd)
				x[rd] = imm + pc;
			break;
		case 0b1101111:
			//JAL
			imm = getJimm(cinst);
			rd = getbin(cinst, 7, 11);
			if (rd)
				x[rd] = pc + 4;
			pc += imm;
			continue;
			break;
		case 0b1100111:	
			//JALR
			imm = getIimm(cinst);
			rd = getbin(cinst, 7, 11);
			rs1 = getbin(cinst, 15, 19);
			if (rd)
				x[rd] = pc + 4;
			pc = (x[rs1] + imm) & ~1;
			continue;
			break;
		case 0b1100011:
			//BEQ, BNE, BLT, BGE, BLTU, BGEU
			funct3 = getbin(cinst, 12, 14);
			imm = getBimm(cinst);
			rs1 = getbin(cinst, 15, 19);
			rs2 = getbin(cinst, 20, 24);
			switch (funct3) {
			case 0b000:
				//BEQ
				if (x[rs1] == x[rs2])
				{
					pc += imm;
					continue;
				}
				break;
			case 0b001:
				//BNE
				if (x[rs1] != x[rs2])
				{
					pc += imm;
					continue;
				}
				break;
			case 0b100:
				//BLT
				if (int(x[rs1]) < int(x[rs2]))
				{
					pc += imm;
					continue;
				}
				break;
			case 0b110:
				//BLTU
				if (x[rs1] < x[rs2])
				{
					pc += imm;
					continue;
				}
				break;
			case 0b101:
				//BGE
				if (int(x[rs1]) >= int(x[rs2]))
				{
					pc += imm;
					continue;
				}
				break;
			case 0b111:
				//BGEU
				if (x[rs1] >= x[rs2])
				{
					pc += imm;
					continue;
				}
				break;
			}
			break;
		case 0b0000011:
			//LB, LH, LW, LBU, LHU
			funct3 = getbin(cinst, 12, 14);
			imm = getIimm(cinst);
			rs1 = getbin(cinst, 15, 19);
			rd = getbin(cinst, 7, 11);
			tmp = 0;
			switch (funct3) {
			case 0b000:
				//LB
				if (rd) {
					memcpy(&tmp, &mem[x[rs1] + imm], 1);
					x[rd] = tmp | int(tmp) >> 31 << 8;
				}
				break;
			case 0b001:
				//LH
				if (rd) {
					memcpy(&tmp, &mem[x[rs1] + imm], 2);
					x[rd] = tmp | int(tmp) >> 31 << 16;
				}
				break;
			case 0b010:
				//LW
				if (rd) {
					memcpy(&tmp, &mem[x[rs1] + imm], 4);
					x[rd] = tmp;
				}
				break;
			case 0b100:
				//LBU
				if (rd) {
					memcpy(&tmp, &mem[x[rs1] + imm], 1);
					x[rd] = tmp;
				}
				break;
			case 0b101:
				//LHU
				if (rd) {
					memcpy(&tmp, &mem[x[rs1] + imm], 2);
					x[rd] = tmp;
				}
				break;
			}
			break;
		case 0b0100011:
			//SB, SH, SW
			funct3 = getbin(cinst, 12, 14);
			imm = getSimm(cinst);
			rs1 = getbin(cinst, 15, 19);
			rs2 = getbin(cinst, 20, 24);
			switch (funct3) {
			case 0b000:
				//SB
				memcpy(&mem[imm + x[rs1]], &x[rs2], 1);
				break;
			case 0b001:
				//SH
				memcpy(&mem[imm + x[rs1]], &x[rs2], 2);
				break;
			case 0b010:
				//SW
				memcpy(&mem[imm + x[rs1]], &x[rs2], 4);
				break;
			}
			break;
		case 0b0010011:
			//ADDI, SLTI, SLTIU, XORI, ORI, ANDI, SLLI, SRLI, SRAI
			funct3 = getbin(cinst, 12, 14);
			rd = getbin(cinst, 7, 11);
			rs1 = getbin(cinst, 15, 19);
            funct7 = getbin(cinst, 25, 31);
			shamt = getbin(cinst, 20, 24);
			imm = getIimm(cinst);
			switch (funct3) {
			case 0b000:
				//ADDI
				if (rd)
					x[rd] = x[rs1] + imm;
				break;
			case 0b010:
				//SLTI
				if (rd)
					x[rd] = (int(x[rs1]) < imm);
				break;
			case 0b011:
				//SLTIU
				if (rd)
					x[rd] = (x[rs1] < ui(imm));
				break;
			case 0b100:
				//XORI
				if (rd)
					x[rd] = x[rs1] ^ imm;
				break;
			case 0b110:
				//ORI
				if (rd)
					x[rd] = x[rs1] | imm;
				break;
			case 0b111:
				//ANDI
				if (rd)
					x[rd] = x[rs1] & imm;
				break;
			case 0b001:
				//SLLI
				if (rd)
					x[rd] = x[rs1] << shamt;
				break;
			case 0b101:
				if (funct7 == 0b0000000) {
					//SRLI
					if (rd)
						x[rd] = x[rs1] >> shamt;
				}
				else {
					//SRAI
					if (rd)
						x[rd] = int(x[rs1]) >> shamt;
				}
				break;
			}
			break;
		case 0b0110011:
			//ADD, SUB, SLL, SLT, SLTU, XOR, SRL, SRA, OR, AND
			funct3 = getbin(cinst, 12, 14);
			funct7 = getbin(cinst, 25, 31);
			rd = getbin(cinst, 7, 11);
			rs1 = getbin(cinst, 15, 19);
			rs2 = getbin(cinst, 20, 24);
			switch (funct3) {
			case 0b000:
				if (funct7 == 0b0000000) {
					//ADD
					if (rd)
						x[rd] = x[rs1] + x[rs2];
				}
				else {
					//SUB
					if (rd)
						x[rd] = x[rs1] - x[rs2];
				}
				break;
			case 0b001:
				//SLL
				if (rd)
					x[rd] = x[rs1] << (x[rs2] & ((1 << 6) - 1));
				break;
			case 0b010:
				//SLT
				if (rd)
					x[rd] = (int(x[rs1]) < int(x[rs2]));
				break;
			case 0b011:
				//SLTU
				if (rd)
					x[rd] = (x[rs1] < x[rs2]);
				break;
			case 0b100:
				//XOR
				if (rd)
					x[rd] = x[rs1] ^ x[rs2];
				break;
			case 0b101:
				if (funct7 == 0b0000000) {
					//SRL
					if (rd)
						x[rd] = x[rs1] >> (x[rs2] & ((1 << 6) - 1));
				}
				else {
					//SRA
					if (rd)
						x[rd] = int(x[rs1]) >> (x[rs2] & ((1 << 6) - 1));
				}
				break;
			case 0b110:
				//OR
				if (rd)
					x[rd] = x[rs1] | x[rs2];
				break;
			case 0b111:
				//AND
				if (rd)
					x[rd] = x[rs1] & x[rs2];
				break;
			}
			break;
        default:
            printf("no op!\n");
            break;
		}
		pc += 4;
	}
	return 0;
}