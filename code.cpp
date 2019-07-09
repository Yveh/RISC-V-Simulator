#include "simulator.hpp"

simulator_t<0x20000> simulator;

int main() {
	size_t ans = simulator.run();
	printf("%u", ans);
	return 0;
}