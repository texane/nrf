/* doc8161.pdf, chapter 23 (page 250) */

#include <stdint.h>
#include <avr/io.h>

#define ADC_DDR DDRC
#define ADC_PORT PORTC
#define ADC_MASK (1 << 0)

static inline void adc_setup(void)
{
}

static void adc_start_read(void)
{
}

static uint16_t adc_wait_read(void)
{
  return 0;
}

static inline uint16_t adc_read(void)
{
  adc_start_read();
  return adc_wait_read();
}
