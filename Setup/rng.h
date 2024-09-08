#include <stdint.h>

#define RNG_RANDOM_MAX   0xFFFFFFFF

void rng_init(void);
uint32_t rng_random(void);