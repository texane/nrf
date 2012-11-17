/* doc8161.pdf, chapter 23 (page 250) */

#include <stdint.h>
#include <avr/io.h>

#define ADC_DDR DDRC
#define ADC_PORT PORTC
#define ADC_MASK (1 << 0)

static inline void adc_setup(void)
{
  /* this routine must be called once and before the others */

  DDRC &= ~ADC_MASK;

#if 0 /* fadc to 125khz */

  /* 10 bits resolution requires a clock between 50khz and 200khz. */
  /* a prescaler is used to derive fadc from fcpu. the prescaler */
  /* starts counting when ADEN is set, and is always reset when */
  /* ADEN goes low. */

  /* 16mhz / 200khz = 80. prescaler set to 128, thus fadc = 125khz */
  /* a normal conversion takes 13 FADC cycles */
  /* thus, actual sampling rate of 125000 / 13 = 9615 samples per second */

  ADCSRA = (7 << ADPS0);

#else /* force fadc to 1mhz */

  ADCSRA = (4 << ADPS0);

#endif /* fadc */

  ADCSRB = 0;

  /* aref, internal vref off, channel 0 */
  ADMUX = 1 << REFS0;

  /* disable digital input 0 to reduce power consumption, cf 23.9.5 */
  DIDR0 = 0x1;
}

static inline void adc_wait_25(void)
{
  /* wait for 25 adc cycles */
  /* FADC = FCPU / 128, thus waits for 128 * 25 = 3200 cpu cycles */
  uint8_t x = 0xff;
  for (; x; --x) __asm__ __volatile__ ("nop\n\t");
}

static void adc_start_free_running(void)
{
  /* enable adc and free running mode */
  ADCSRB = 0;
  ADCSRA |= (1 << ADEN) | (1 << ADSC) | (1 << ADATE);

  /* the first conversion starts 25 FADC cycles after ADEN set */
  adc_wait_25();
}

static inline void adc_stop(void)
{
  ADCSRA &= ~(1 << ADEN);
}

static inline uint16_t adc_read(void)
{
  /* read adcl first */
  const uint8_t l = ADCL;
  return ((((uint16_t)ADCH) << 8) | (uint16_t)l) & 0x3ff;
}
