#include "simulator.hpp"
#include <iostream>

simulator_t<0x20000> simulator;

int main() {
	unsigned int ans = simulator.run();
	std::cout << ans << std::endl;
	return 0;
}
