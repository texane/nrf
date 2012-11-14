#include <stdint.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include "../../common/spi.c"
#include "../../common/uart.c"
#include "../rx/dac7554.c"


/* timer1a compare on match handler */

static const uint8_t sample_array[256] =
{
#if 0
# include "../tone/tone_40000_1000.c"
#elif 1
# include "../tone/tone_40000_4000.c"
#elif 0
# include "../tone/tone_40000_8000.c"
#elif 0
# include "../tone/tone_40000_16000.c"
#elif 0
# include "../tone/tone_40000_32000.c"
#endif
};

static uint8_t sample_index;

ISR(TIMER1_COMPA_vect)
{
  dac7554_write(sample_array[sample_index++] << 4, 0);
}


int main(void)
{
  uart_setup();

  spi_setup_master();
  spi_set_sck_freq(SPI_SCK_FREQ_FOSC2);

  /* must be done before spi_setup_master since cs */
  dac7554_setup();

  uart_write((uint8_t*)"press space\r\n", 13);
  uart_read_uint8();
  uart_write((uint8_t*)"starting\r\n", 10);

  /* sample index */
  sample_index = 0;

  /* clock source disabled, safe to access 16 bits counter */
  /* setup timer1, ctc mode, interrupt on match 400 */
  TCNT1 = 0;
  OCR1A = 400;
  OCR1B = 0xffff;
  TCCR1A = 0;
  TCCR1C = 0;
  TIMSK1 = 1 << 1;
  /* prescaler set to 1, autoclear on interrupt on match */
  TCCR1B = (1 << 3) | (1 << 0);

  /* enable interrupts */
  sei();

  while (1) ;

  return 0;
}
