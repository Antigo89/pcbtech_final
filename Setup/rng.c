#include "stm32f4xx.h"
#include "rng.h"

void rng_init(void) {
  /* Enable RNG clock */
  RCC->AHB2ENR |= RCC_AHB2ENR_RNGEN;
  /* Run RNG */
  RNG->CR |= RNG_CR_RNGEN;
}

uint32_t rng_random(void) {
  while (!(RNG->SR & RNG_SR_DRDY));
  
  return RNG->DR;
}