#ifndef RISC_V_SIMULATOR
#define RISC_V_SIMULATOR

#include "scanner.hpp"

template<unsigned int _len = 0x20000>
class simulator_t {
	private:
		unsigned int x[32], PC;

		unsigned int IFID_IR, IFID_PC, IFID_NPC;
		unsigned int IDEX_IR, IDEX_OPCode, IDEX_RD, IDEX_PC, IDEX_NPC, IDEX_Funct3, IDEX_Funct7, IDEX_RS1, IDEX_RS2, IDEX_A, IDEX_B, IDEX_Imm;
		unsigned int EXMEM_IR, EXMEM_OPCode, EXMEM_RD, EXMEM_NPC, EXMEM_ALUOutput, EXMEM_Funct3, EXMEM_B, EXMEM_cond;
		unsigned int MEMWB_IR, MEMWB_OPCode, MEMWB_RD, MEMWB_NPC, MEMWB_ALUOutput, MEMWB_LMD;
	
		unsigned int MEM_delay;

		char *mem;

		void store(unsigned int *par, unsigned int ind, unsigned int len) {
			memcpy(&mem[ind], par, len);
		}
		void load(unsigned int *par, unsigned int ind, unsigned int len) {
			*par = 0;
			memcpy(par, &mem[ind], len);
		}

		
		unsigned int getbin(unsigned int x, unsigned int l, unsigned int r) {
			return (x >> l) & (x << 31 - r >> 31 - r >> l);
		}
		int getIimm(unsigned int x) {
			return int(getbin(x, 20, 31)) << 20 >> 20;
		}
		int getUimm(unsigned int x) {
			return int(getbin(x, 12, 31)) << 12;
		}
		int getSimm(unsigned int x) {
			return int(getbin(x, 25, 31) << 5 | getbin(x, 7, 11)) << 20 >> 20;
		}
		int getBimm(unsigned int x) {
			return int(getbin(x, 31, 31) << 11 | getbin(x, 7, 7) << 10 | getbin(x, 25, 30) << 4 | getbin(x, 8, 11)) << 20 >> 19;
		}
		int getJimm(unsigned int x) {
			return int(getbin(x, 31, 31) << 19 | getbin(x, 12, 19) << 11 | getbin(x, 20, 20) << 10 | getbin(x, 21, 30)) << 12 >> 11; 
		}
		unsigned int getopcode(unsigned int x) {
			return getbin(x, 0, 6);
		}
		int getimm(unsigned int x) {
			unsigned int opcode = getopcode(x);
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
		unsigned int getrd(unsigned int x) {
			return getbin(x, 7, 11);
		}
		unsigned int getrs1(unsigned int x) {
			return getbin(x, 15, 19);
		}
		unsigned int getrs2(unsigned int x) {
			return getbin(x, 20, 24);
		}
		unsigned int getfunct3(unsigned int x) {
			return getbin(x, 12, 14);
		}
		unsigned int getfunct7(unsigned int x) {
			return getbin(x, 25, 31);
		}


		bool is_ALU(unsigned int opcode) {
			return opcode == 0b0110111 || opcode == 0b0010111 || opcode == 0b0010011 || opcode == 0b0110011;
		}
		bool is_SL(unsigned int opcode) {
			return opcode == 0b0000011 || opcode == 0b0100011;
		}
		bool is_branch(unsigned int opcode) {
			return opcode == 0b1101111 || opcode == 0b1100111 || opcode == 0b1100011;
		}
		bool is_Zero(unsigned int opcode) {
			return opcode == 0b0000000;
		}

		bool IF() {
/*
			if (EXMEM_IR && is_branch(EXMEM_OPCode) && EXMEM_cond)
				PC = EXMEM_ALUOutput;
			load(&IFID_IR, PC, 4);
			IFID_PC = PC;
			PC = IFID_NPC = PC + 4;
*/

			if (EXMEM_IR && (EXMEM_OPCode == 0b1101111 || EXMEM_OPCode == 0b1100011) && !EXMEM_cond) {
				PC = EXMEM_NPC;
			}
			if (EXMEM_IR && EXMEM_OPCode == 0b1100111 && EXMEM_cond) {
				PC = EXMEM_ALUOutput;
			}
			load(&IFID_IR, PC, 4);
			IFID_PC = PC;
			IFID_NPC = PC + 4;
			if (getopcode(IFID_IR) == 0b1101111 || getopcode(IFID_IR) == 0b1100011)
				PC += getimm(IFID_IR);
			else
				PC = PC + 4;

			return 1;
		}
		bool ID() {
			if (IFID_IR == 0) {
				IDEX_IR = 0;
				return 1;
			}
			if (EXMEM_IR && ((EXMEM_OPCode == 0b1101111 || EXMEM_OPCode == 0b1100011) && !EXMEM_cond || EXMEM_OPCode == 0b1100111 && EXMEM_cond)) {
//			if (EXMEM_IR && is_branch(EXMEM_OPCode) && EXMEM_cond) {
				IFID_IR = 0;
				return 1;
			}			

			IDEX_RS1 = getrs1(IFID_IR);
			IDEX_RS2 = getrs2(IFID_IR);
			
			if (EXMEM_IR && EXMEM_RD && (is_ALU(EXMEM_OPCode) || EXMEM_OPCode == 0b0000011 || EXMEM_OPCode == 0b1101111 || EXMEM_OPCode == 0b1100111) && EXMEM_RD == IDEX_RS1) {
				if (is_ALU(EXMEM_OPCode))
					IDEX_A = EXMEM_ALUOutput;
				else if (EXMEM_OPCode == 0b0000011) {
					IDEX_IR = 0;
					return 0;
				}
				else if (EXMEM_OPCode == 0b1101111 || EXMEM_OPCode == 0b1100111)
					IDEX_A = EXMEM_NPC;
			}
			else if (MEMWB_IR && MEMWB_RD && (is_ALU(MEMWB_OPCode) || MEMWB_OPCode == 0b0000011 || MEMWB_OPCode == 0b1101111 || MEMWB_OPCode == 0b1100111) && MEMWB_RD == IDEX_RS1) {
				if (is_ALU(MEMWB_OPCode))
					IDEX_A = MEMWB_ALUOutput;
				else if (MEMWB_OPCode == 0b0000011)
					IDEX_A = MEMWB_LMD;
				else if (MEMWB_OPCode == 0b1101111 || MEMWB_OPCode == 0b1100111)
					IDEX_A = MEMWB_NPC;
			}
			else {
				IDEX_A = x[IDEX_RS1];
			}
			
			if (EXMEM_IR && EXMEM_RD && (is_ALU(EXMEM_OPCode) || EXMEM_OPCode == 0b0000011 || EXMEM_OPCode == 0b1101111 || EXMEM_OPCode == 0b1100111) && EXMEM_RD == IDEX_RS2) {
				if (is_ALU(EXMEM_OPCode))
					IDEX_B = EXMEM_ALUOutput;
				else if (EXMEM_OPCode == 0b0000011) {
					IDEX_IR = 0;
					return 0;
				}
				else if (EXMEM_OPCode == 0b1101111 || EXMEM_OPCode == 0b1100111)
					IDEX_B = EXMEM_NPC;
			}
			else if (MEMWB_IR && MEMWB_RD && (is_ALU(MEMWB_OPCode) || MEMWB_OPCode == 0b0000011 || MEMWB_OPCode == 0b1101111 || MEMWB_OPCode == 0b1100111) && MEMWB_RD == IDEX_RS2) {
				if (is_ALU(MEMWB_OPCode))
					IDEX_B = MEMWB_ALUOutput;
				else if (MEMWB_OPCode == 0b0000011)
					IDEX_B = MEMWB_LMD;
				else if (MEMWB_OPCode == 0b1101111 || MEMWB_OPCode == 0b1100111)
					IDEX_B = MEMWB_NPC;
			}
			else
				IDEX_B = x[IDEX_RS2];
			
			IDEX_IR = IFID_IR;
			IDEX_PC = IFID_PC;
			IDEX_NPC = IFID_NPC;
			IDEX_OPCode = getopcode(IFID_IR);
			IDEX_Imm = getimm(IFID_IR);
			IDEX_RD = getrd(IFID_IR);
			IDEX_Funct3 = getfunct3(IFID_IR);
			IDEX_Funct7 = getfunct7(IFID_IR);

			return 1;
		}
		bool EX() {
			if (IDEX_IR == 0) {
				EXMEM_IR = 0;
				return 1;
			}
//			if (EXMEM_IR && is_branch(EXMEM_OPCode) && EXMEM_cond) {
			if (EXMEM_IR && ((EXMEM_OPCode == 0b1101111 || EXMEM_OPCode == 0b1100011) && !EXMEM_cond || EXMEM_OPCode == 0b1100111 && EXMEM_cond)) {
				IDEX_IR = 0;
				return 1;
			}
			if (IDEX_IR == 0x00c68223) {
				EXMEM_IR = IDEX_IR;
				return 0;
			}
			if (is_ALU(IDEX_OPCode)) {
				switch (IDEX_OPCode)
				{
				case 0b0110111:
					EXMEM_ALUOutput = IDEX_Imm;
					break;
				case 0b0010111:
					EXMEM_ALUOutput = IDEX_Imm + IDEX_PC;
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
						EXMEM_ALUOutput = (IDEX_A < (unsigned int)(IDEX_Imm));
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
					EXMEM_ALUOutput += IDEX_PC;
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
			EXMEM_IR = IDEX_IR;
			EXMEM_RD = IDEX_RD;
			EXMEM_OPCode = IDEX_OPCode;
			EXMEM_Funct3 = IDEX_Funct3;
			EXMEM_NPC = IDEX_NPC;
			return 1;
		}
		bool MEM() {
			if (EXMEM_IR == 0) {
				MEMWB_IR = 0;
				return 1;
			}
			if (EXMEM_IR == 0x00c68223) {
				MEMWB_IR = EXMEM_IR;
				return 0;
			}
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
			MEMWB_IR = EXMEM_IR;
			MEMWB_RD = EXMEM_RD;
			MEMWB_OPCode = EXMEM_OPCode;
			MEMWB_NPC = EXMEM_NPC;

			return 1;
		}
		bool WB() {
			if (MEMWB_IR == 0x00c68223)
				return 0;			
			if (MEMWB_IR == 0)
				return 1;
			if (is_ALU(MEMWB_OPCode) && MEMWB_RD)
				x[MEMWB_RD] = MEMWB_ALUOutput;
			else if (MEMWB_OPCode == 0b0000011 && MEMWB_RD)
				x[MEMWB_RD] = MEMWB_LMD;
			else if ((MEMWB_OPCode == 0b1101111 || MEMWB_OPCode == 0b1100111) && MEMWB_RD)
				x[MEMWB_RD] = MEMWB_NPC;
			return 1;
		}
	public:
		simulator_t() {
			mem = new char[_len];
			for (int i = 0; i < _len; ++i)
				mem[i] = 0;
			PC = 0;
			IFID_IR = IDEX_IR = EXMEM_IR = MEMWB_IR = 0;
			MEM_delay = 0;
			scanner_t scanner;
			scanner.readInst(mem);
		}
		void debug() {
			for (int i = 0; i < 32; ++i)
				if (x[i] > 0)
					printf("x[%d]=%d ", i, x[i]);
		}
		unsigned int run() {
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
