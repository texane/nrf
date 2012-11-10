#include <stdint.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include "../../common/spi.c"
#include "../../common/nrf24l01p.c"
#include "../../common/uart.c"
#include "dac7554.c"


/* fifo */

typedef struct fifo
{
  /* read write pointers */
  volatile uint8_t r;
  volatile uint8_t w;

  /* the fifo is sized to uint8_max + 1 so that */
  /* index wrapping is automatically handled */
  uint8_t buf[256];

} fifo_t;

static fifo_t fifo;

static inline void fifo_setup(void)
{
  fifo.w = 0;
  fifo.r = 0;
}

static void fifo_write_payload(void)
{
  /* write payload from nrf24l01p_cmd_buf */

  uint8_t i;

  for (i = 0; i < NRF24L01P_PAYLOAD_WIDTH; ++i)
  {
    fifo.buf[fifo.w] = nrf24l01p_cmd_buf[i];
    /* note that it is concurrent with reader interrupt */
    /* incrementing fifo.w comes after the actual write */
    /* so that the reader always access a valid location */
    ++fifo.w;
  }
}

static inline uint8_t fifo_is_empty(void)
{
  return fifo.w == fifo.r;
}

static inline uint8_t fifo_read_uint8(void)
{
  /* index wrapping automatically handled */
  return fifo.buf[fifo.r++];
}


/* timer1a compare on match handler */

ISR(TIMER1_COMPA_vect)
{
  uint8_t x = 0;

  /* note that this is concurrent with fifo_write */
  /* the reader may miss a location in progress, but */
  /* never accesses an invalid one */
  if (fifo_is_empty() == 0) x = fifo_read_uint8();

  /* 12 bits dac, extend for now */
  dac7554_write((uint16_t)x << 4, 0);
}

static void on_nrf24l01p_irq(void)
{
  /* TODO: read_rx could read directly into the fifo */
  nrf24l01p_read_rx();
  fifo_write_payload();
}


/* main */

int main(void)
{
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

  uart_setup();
  fifo_setup();
  dac7554_setup();

  /* setup timer1, ctc mode, interrupt on match 400 */
  TCNT1 = 0;
  OCR1A = 400;
  OCR1B = 0xffff;
  TCCR1A = 0;
  TCCR1B = 0;
  TCCR1C = 0;
  TIMSK1 = 1 << 1;

  /* enable interrupts */
  sei();

  uart_write((uint8_t*)"rx side\r\n", 9);
  uart_write((uint8_t*)"press space\r\n", 13);
  uart_read_uint8();
  uart_write((uint8_t*)"starting\r\n", 10);

  nrf24l01p_standby_to_rx();

  if (nrf24l01p_is_rx_full()) nrf24l01p_flush_rx();

  /* clock source disabled, safe to access 16 bits counter */
  TCNT1 = 0;
  /* prescaler set to 1, interrupt at 40khz */
  TCCR1B = (1 << 3) | (1 << 0);

  while (1)
  {
    /* wait for irq and process */
    while (nrf24l01p_is_rx_irq() == 0) ;
    on_nrf24l01p_irq();
    /* still in rx mode */
  }

  return 0;
}
