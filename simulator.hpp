#ifndef RISC_V_SIMULATOR
#define RISC_V_SIMULATOR

#include "scanner.hpp"

template<size_t _len = 0x20000>
class simulator_t {
	private:
		size_t x[32], PC;

		size_t IFID_IR, IFID_NPC;
		size_t IDEX_IR, IDEX_OPCode, IDEX_RD, IDEX_NPC, IDEX_Funct3, IDEX_Funct7, IDEX_RS1, IDEX_RS2, IDEX_A, IDEX_B, IDEX_Imm;
		size_t EXMEM_IR, EXMEM_OPCode, EXMEM_RD, EXMEM_NPC, EXMEM_ALUOutput, EXMEM_Funct3, EXMEM_B, EXMEM_cond;
		size_t MEMWB_IR, MEMWB_OPCode, MEMWB_RD, MEMWB_NPC, MEMWB_ALUOutput, MEMWB_LMD;
	
		size_t MEM_delay;

		char *mem;

		void store(size_t *par, size_t ind, size_t len) {
			// printf("STORE %u %u\n", *par, ind);
			memcpy(&mem[ind], par, len);
		}
		void load(size_t *par, size_t ind, size_t len) {
			*par = 0;
			memcpy(par, &mem[ind], len);
			// if (par != &IFID_IR)
			// 	printf("LOAD %u %u\n", *par, ind);
		}

		
		size_t getbin(size_t x, size_t l, size_t r) {
			return (x >> l) & (x << 31 - r >> 31 - r >> l);
		}
		int getIimm(size_t x) {
			return int(getbin(x, 20, 31)) << 20 >> 20;
		}
		int getUimm(size_t x) {
			return int(getbin(x, 12, 31)) << 12;
		}
		int getSimm(size_t x) {
			return int(getbin(x, 25, 31) << 5 | getbin(x, 7, 11)) << 20 >> 20;
		}
		int getBimm(size_t x) {
			return int(getbin(x, 31, 31) << 11 | getbin(x, 7, 7) << 10 | getbin(x, 25, 30) << 4 | getbin(x, 8, 11)) << 20 >> 19;
		}
		int getJimm(size_t x) {
			return int(getbin(x, 31, 31) << 19 | getbin(x, 12, 19) << 11 | getbin(x, 20, 20) << 10 | getbin(x, 21, 30)) << 12 >> 11; 
		}
		size_t getopcode(size_t x) {
			return getbin(x, 0, 6);
		}
		int getimm(size_t x) {
			size_t opcode = getopcode(x);
			switch (opcode) {
			case 0b0110111: case 0b0010111:
				return getUimm(x);
				break;
			case 0b1101111:
				return getJimm(x);
				break;
			case 0b1100111: case 0b0000011: case 0b0010011:
				return getIimm(x);
				break;
			case 0b1100011:
				return getBimm(x);
				break;
			case 0b0100011:
				return getSimm(x);
				break;
			default:
				return 0;
				break;
			}
		}
		size_t getrd(size_t x) {
			return getbin(x, 7, 11);
		}
		size_t getrs1(size_t x) {
			return getbin(x, 15, 19);
		}
		size_t getrs2(size_t x) {
			return getbin(x, 20, 24);
		}
		size_t getfunct3(size_t x) {
			return getbin(x, 12, 14);
		}
		size_t getfunct7(size_t x) {
			return getbin(x, 25, 31);
		}


		bool is_ALU(size_t opcode) {
			return opcode == 0b0110111 || opcode == 0b0010111 || opcode == 0b0010011 || opcode == 0b0110011;
		}
		bool is_SL(size_t opcode) {
			return opcode == 0b0000011 || opcode == 0b0100011;
		}
		bool is_branch(size_t opcode) {
			return opcode == 0b1101111 || opcode == 0b1100111 || opcode == 0b1100011;
		}
		bool is_Zero(size_t opcode) {
			return opcode == 0b0000000;
		}

		bool IF() {
			load(&IFID_IR, PC, 4);
			// printf("PC = %X IFID_IR = %08X ", PC, IFID_IR);
			if (IFID_IR == 0x00c68223)
				return 0;
			if (EXMEM_IR && is_branch(EXMEM_OPCode) && EXMEM_cond) {
				PC = IFID_NPC = EXMEM_ALUOutput;
				IDEX_IR = 0;
				IFID_IR = 0;
			}
			else {
				PC = IFID_NPC = PC + 4;
			}
			return 1;
		}
		bool ID() {
			IDEX_IR = IFID_IR;
			if (IFID_IR == 0x00c68223)
				return 0;
			if (IFID_IR == 0)
				return 1;

			IDEX_RS1 = getrs1(IFID_IR);
			IDEX_RS2 = getrs2(IFID_IR);
			IDEX_A = x[IDEX_RS1];
			IDEX_B = x[IDEX_RS2];
			if (EXMEM_IR && (EXMEM_RD == IDEX_RS1 || EXMEM_RD == IDEX_RS2)) {
				if (is_ALU(EXMEM_OPCode)) {
					if (EXMEM_RD == IDEX_RS1)
						IDEX_A = EXMEM_ALUOutput;
					if (EXMEM_RD == IDEX_RS2)
						IDEX_B = EXMEM_ALUOutput;
				}
				else if (EXMEM_OPCode == 0b0000011) {
					if (EXMEM_RD == IDEX_RS1 || EXMEM_RD == IDEX_RS2) {
						IDEX_IR = 0;
						return 0;
					}
				}
			}
			else if (MEMWB_IR && (MEMWB_RD == IDEX_RS1 || MEMWB_RD == IDEX_RS2)) {
				if (is_ALU(MEMWB_OPCode)) {
					if (MEMWB_RD == IDEX_RS1)
						IDEX_A = MEMWB_ALUOutput;
					if (MEMWB_RD == IDEX_RS2)
						IDEX_B = MEMWB_ALUOutput;
				}
				else if (MEMWB_OPCode == 0b0000011) {
					if (MEMWB_RD == IDEX_RS1)
						IDEX_A = MEMWB_LMD;
					if (MEMWB_RD == IDEX_RS2)
						IDEX_B = MEMWB_LMD;
				}
			}
			IDEX_NPC = IFID_NPC;
			IDEX_Imm = getimm(IFID_IR);
			IDEX_RD = getrd(IFID_IR);
			IDEX_OPCode = getopcode(IFID_IR);
			IDEX_Funct3 = getfunct3(IFID_IR);
			IDEX_Funct7 = getfunct7(IFID_IR);

			IFID_IR = 0;
			return 1;
		}
		bool EX() {
			EXMEM_IR = IDEX_IR;
			if (IDEX_IR == 0x00c68223)
				return 0;
			if (IDEX_IR == 0)
				return 1;
			if (is_ALU(IDEX_OPCode)) {
				switch (IDEX_OPCode)
				{
				case 0b0110111:
					EXMEM_ALUOutput = IDEX_Imm;
					break;
				case 0b0010111:
					EXMEM_ALUOutput = IDEX_Imm + IDEX_NPC - 4;
					break;
				case 0b0010011:
					switch (IDEX_Funct3) {
					case 0b000:
						EXMEM_ALUOutput = IDEX_A + IDEX_Imm;
						break;
					case 0b010:
						EXMEM_ALUOutput = (int(IDEX_A) < IDEX_Imm);
						break;
					case 0b011:
						EXMEM_ALUOutput = (IDEX_A < size_t(IDEX_Imm));
						break;
					case 0b100:
						EXMEM_ALUOutput = IDEX_A ^ IDEX_Imm;
						break;
					case 0b110:
						EXMEM_ALUOutput = IDEX_A | IDEX_Imm;
						break;
					case 0b111:
						EXMEM_ALUOutput = IDEX_A & IDEX_Imm;
						break;
					case 0b001:
						EXMEM_ALUOutput = IDEX_A << (IDEX_Imm & ((1 << 6) - 1));
						break;
					case 0b101:
						if (IDEX_Funct7 == 0b0000000)
							EXMEM_ALUOutput = IDEX_A >> (IDEX_Imm & ((1 << 6) - 1));
						else
							EXMEM_ALUOutput = int(IDEX_A) >> (IDEX_Imm & ((1 << 6) - 1));
						break;
					}
					break;
				case 0b0110011:
					switch (IDEX_Funct3) {
					case 0b000:
						if (IDEX_Funct7 == 0b0000000)
							EXMEM_ALUOutput = IDEX_A + IDEX_B;
						else
							EXMEM_ALUOutput = IDEX_A - IDEX_B;
						break;
					case 0b001:
						EXMEM_ALUOutput = IDEX_A << (IDEX_B & ((1 << 6) - 1));
						break;
					case 0b010:
						EXMEM_ALUOutput = (int(IDEX_A) < int(IDEX_B));
						break;
					case 0b011:
						EXMEM_ALUOutput = (IDEX_A < IDEX_B);
						break;
					case 0b100:
						EXMEM_ALUOutput = IDEX_A ^ IDEX_B;
						break;
					case 0b101:
						if (IDEX_Funct7 == 0b0000000)
							EXMEM_ALUOutput = IDEX_A >> (IDEX_B & ((1 << 6) - 1));
						else
							EXMEM_ALUOutput = int(IDEX_A) >> (IDEX_B & ((1 << 6) - 1));
						break;
					case 0b110:
						EXMEM_ALUOutput = IDEX_A | IDEX_B;
						break;
					case 0b111:
						EXMEM_ALUOutput = IDEX_A & IDEX_B;
						break;
					}
					break;
				}
			}
			else if (is_SL(IDEX_OPCode)) {
				EXMEM_ALUOutput = IDEX_A + IDEX_Imm;
				EXMEM_B = IDEX_B;
			}
			else if (is_branch(IDEX_OPCode)) {
				EXMEM_ALUOutput = IDEX_Imm;
				if (IDEX_OPCode == 0b1100111) {
					EXMEM_ALUOutput += IDEX_A;
					EXMEM_ALUOutput &= -1;
				}
				else
					EXMEM_ALUOutput += IDEX_NPC - 4;
					
				if (IDEX_OPCode == 0b1101111 || IDEX_OPCode == 0b1100111)
					EXMEM_cond = 1;
				else if (IDEX_OPCode == 0b1100011) {
					switch (IDEX_Funct3) {
					case 0b000:
						EXMEM_cond = IDEX_A == IDEX_B;
						break;
					case 0b001:
						EXMEM_cond = IDEX_A != IDEX_B;
						break;
					case 0b100:
						EXMEM_cond = int(IDEX_A) < int(IDEX_B);
						break;
					case 0b110:
						EXMEM_cond = IDEX_A < IDEX_B;
						break;
					case 0b101:
						EXMEM_cond = int(IDEX_A) >= int(IDEX_B);
						break;
					case 0b111:
						EXMEM_cond = IDEX_A >= IDEX_B;
						break; 
					}
				}
			}
			EXMEM_RD = IDEX_RD;
			EXMEM_OPCode = IDEX_OPCode;
			EXMEM_Funct3 = IDEX_Funct3;
			EXMEM_NPC = IDEX_NPC;

			IDEX_IR = 0;
			return 1;
		}
		bool MEM() {
			MEMWB_IR = EXMEM_IR;
			if (EXMEM_IR == 0x00c68223)
				return 0;
			if (EXMEM_IR == 0)
				return 1;
			if (is_ALU(EXMEM_OPCode)) {
				MEMWB_ALUOutput = EXMEM_ALUOutput;
			}
			else if (is_SL(EXMEM_OPCode)) {
				--MEM_delay;
				if (MEM_delay != 0) {
					if (int(MEM_delay) < 0)
						MEM_delay = 2;
					MEMWB_IR = 0;
					return 0;
				}
				switch (EXMEM_OPCode)
				{
				case 0b0000011:
					switch (EXMEM_Funct3) {
					case 0b000:
						load(&MEMWB_LMD, EXMEM_ALUOutput, 1);
						MEMWB_LMD = MEMWB_LMD | int(MEMWB_LMD) >> 31 << 8;
						break;
					case 0b001:
						load(&MEMWB_LMD, EXMEM_ALUOutput, 2);
						MEMWB_LMD = MEMWB_LMD | int(MEMWB_LMD) >> 31 << 16;
						break;
					case 0b010:
						load(&MEMWB_LMD, EXMEM_ALUOutput, 4);
						break;
					case 0b100:
						load(&MEMWB_LMD, EXMEM_ALUOutput, 1);
						break;
					case 0b101:
						load(&MEMWB_LMD, EXMEM_ALUOutput, 2);
						break;
					}
					break;
				case 0b0100011:
					switch (EXMEM_Funct3) {
					case 0b000:
						store(&EXMEM_B, EXMEM_ALUOutput, 1);
						break;
					case 0b001:
						store(&EXMEM_B, EXMEM_ALUOutput, 2);
						break;
					case 0b010:
						store(&EXMEM_B, EXMEM_ALUOutput, 4);
						break;
					}
					break;
				}
			}
			MEMWB_RD = EXMEM_RD;
			MEMWB_OPCode = EXMEM_OPCode;
			MEMWB_NPC = EXMEM_NPC;

			EXMEM_IR = 0;
			return 1;
		}
		bool WB() {
			if (MEMWB_IR == 0x00c68223)
				return 0;			
			if (MEMWB_IR == 0)
				return 1;
			if (is_ALU(MEMWB_OPCode)) {
				if (MEMWB_RD)
					x[MEMWB_RD] = MEMWB_ALUOutput;
			}
			else if (is_SL(MEMWB_OPCode)) {
				if(MEMWB_OPCode == 0b0000011 && MEMWB_RD)
					x[MEMWB_RD] = MEMWB_LMD;
			}
			else if (is_branch(MEMWB_OPCode)) {
				if ((MEMWB_OPCode == 0b1101111 || MEMWB_OPCode == 0b1100111) && MEMWB_RD)
					x[MEMWB_RD] = MEMWB_NPC;
			}
			MEMWB_IR = 0;
			return 1;
		}
	public:
		simulator_t() {
			mem = new char[_len];
			PC = 0;
			IFID_IR = IDEX_IR = EXMEM_IR = MEMWB_IR = 0;
			MEM_delay = 0;
			scanner_t scanner;
			scanner.readInst(mem);
		}
		size_t run() {
			while (1) {
				if (!WB())
					break;
				if (!MEM())
					continue;
				if (!EX())
					continue;
				if (!ID())
					continue;
				if (!IF())
					continue;
			}
			return x[10] & ((1 << 8) - 1);
		}
		~simulator_t() {
			delete[] mem;
		}
};

#endif