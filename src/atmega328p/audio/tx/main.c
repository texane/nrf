#include <stdint.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include "../../common/spi.c"
#include "../../common/nrf24l01p.c"
#include "../../common/uart.c"


/* data input source */

#define CONFIG_INPUT_ADC 1

#if (CONFIG_INPUT_ADC == 1)

#include "adc.c"

#else

/* tone generated array */

static const uint8_t tone_array[256] =
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

static uint8_t tone_index = 0;

#endif

static uint8_t sample_buffer[NRF24L01P_PAYLOAD_WIDTH];
static uint8_t sample_index;


/* timer1a compare on match handler */

ISR(TIMER1_COMPA_vect)
{
  /* capture next sample */

#if (CONFIG_INPUT_ADC == 1)
  const uint8_t x = (uint8_t)(adc_read() >> 2);
#else
  const uint8_t x = tone_array[tone_index++];
#endif

  sample_buffer[sample_index++] = x;

  if (sample_index == NRF24L01P_PAYLOAD_WIDTH)
  {
    sample_index = 0;
    nrf24l01p_write_tx_noack_zero(sample_buffer);
    while (nrf24l01p_is_tx_irq() == 0) ;
  }
}


int main(void)
{
  /* setup spi first */
  spi_setup_master();
  spi_set_sck_freq(SPI_SCK_FREQ_FOSC2);

  uart_setup();

  nrf24l01p_setup();

  /* sparkfun usb serial board configuration */
  /* NOTE: nrf24l01p_enable_crc8(); for nrf24l01p board */
  /* nrf24l01p_enable_crc16(); */
  nrf24l01p_disable_crc();
  /* auto ack disabled */
  /* auto retransmit disabled */
  /* 4 bytes payload */
  /* 1mbps, 0dbm */
  /* nrf24l01p_set_rate(NRF24L01P_RATE_1MBPS); */
  nrf24l01p_set_rate(NRF24L01P_RATE_2MBPS);
  /* nrf24l01p_set_rate(NRF24L01P_RATE_250KBPS); */
  /* channel 2 */
  nrf24l01p_set_chan(2);
  /* 5 bytes addr width */
  /* nrf24l01p_set_addr_width(NRF24L01P_ADDR_WIDTH_5); */
  nrf24l01p_set_addr_width(NRF24L01P_ADDR_WIDTH_3);
  /* rx address */
  nrf24l01p_cmd_buf[0] = 0xe7;
  nrf24l01p_cmd_buf[1] = 0xe7;
  nrf24l01p_cmd_buf[2] = 0xe7;
  nrf24l01p_cmd_buf[3] = 0xe7;
  nrf24l01p_cmd_buf[4] = 0xe7;
  nrf24l01p_write_reg40(NRF24L01P_REG_RX_ADDR_P0);
  /* tx address */
  nrf24l01p_cmd_buf[0] = 0xe7;
  nrf24l01p_cmd_buf[1] = 0xe7;
  nrf24l01p_cmd_buf[2] = 0xe7;
  nrf24l01p_cmd_buf[3] = 0xe7;
  nrf24l01p_cmd_buf[4] = 0xe7;
  nrf24l01p_write_reg40(NRF24L01P_REG_TX_ADDR);
  /* enable tx no ack command */
  nrf24l01p_enable_tx_noack();

  nrf24l01p_powerdown_to_standby();
  nrf24l01p_standby_to_tx();
  if (nrf24l01p_is_tx_empty() == 0) nrf24l01p_flush_tx();

  uart_write((uint8_t*)"tx side\r\n", 9);
  uart_write((uint8_t*)"press space\r\n", 13);
  uart_read_uint8();
  uart_write((uint8_t*)"starting\r\n", 10);

  sample_index = 0;

#if (CONFIG_INPUT_ADC == 1)
  /* enable before sampling interrupt */
  adc_setup();
  adc_start_free_running();
#endif

  /* clock source disabled, safe to access 16 bits counter */
  /* setup timer1, ctc mode, interrupt on match 400 */
  TCNT1 = 0;
  OCR1A = 400;
  OCR1B = 0xffff;
  TCCR1A = 0;
  TCCR1C = 0;
  TIMSK1 = 1 << 1;
  /* sample at 40khz, send every 32 bytes */
  /* prescaler set to 1, autoclear on interrupt on match */
  TCCR1B = (1 << 3) | (1 << 0);

  /* enable interrupts */
  sei();

  while (1)
  {
#if 0 /* test, print every 1s using 16 bits counter */
    if (counter >= 40000)
    {
      cli();
      counter = 0;
      sei();
      uart_write((uint8_t*)"x\r\n", 3);
    }
#endif /* test */
  }

  return 0;
}
