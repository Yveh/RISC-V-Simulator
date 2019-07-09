#ifndef RISC_V_SIMULATOR_SCANNER
#define RISC_V_SIMULATOR_SCANNER

#include <iostream>
#include <cstring>

class scanner_t{
	private:
		char getch() {
			char ch;
			ch = std::cin.get();
			while (ch == ' ' || ch == '\n')
				ch = std::cin.get();
			return ch;
		}
		size_t toInt(char x) {
			if ('A' <= x && x <= 'F')
				return x - 'A' + 10;
			else
				return x - '0';
		}
	public:
		scanner_t() {}
		~scanner_t() {}
		void readInst(char *mem) {
			size_t ind = 0, offset = 0, sta = 0, tmp = 0;
			bool flag = 0;
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
						tmp = toInt(ch) << ((sta ^ 1) << 2) | tmp;
						++sta;
						if (sta == 8) {
							memcpy(&mem[ind + offset], &tmp, 4);
							offset += 4;
							sta = 0;
							tmp = 0;
						}
					}
				}
			}
		}

};

#endif