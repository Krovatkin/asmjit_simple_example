#include <cstdint>


void addKernel(const float* a, const float* b, float* c) {


	uint64_t mask16 = 0xFFFFFFFFFFFFFFF0;
	uint64_t not_mask16 = ~mask16;

	// 4 8 16
}