#include <stdint.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include "../../common/spi.c"
#include "../../common/nrf24l01p.c"
#include "../../common/uart.c"


/* timer1a compare on match handler */

static volatile uint8_t is_timer1_irq;

ISR(TIMER1_COMPA_vect)
{
  TCCR1B = 0;
  is_timer1_irq = 1;
}


/* main */

int main(void)
{
  uint16_t counter;

  /* setup spi first */
  spi_setup_master();
  spi_set_sck_freq(SPI_SCK_FREQ_FOSC2);

  uart_setup();

  /* setup timer1, normal mode, interrupt on match 0xffff */
  OCR1A = 0xffff;
  TCCR1A = 0;
  TCCR1B = 0;
  TCCR1C = 0;
  TIMSK1 = 1 << 1;

  /* enable interrupts */
  sei();

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

  uart_write((uint8_t*)"rx side\r\n", 9);

 redo_receive:

  counter = 0;
  is_timer1_irq = 0;

  uart_write((uint8_t*)"press space\r\n", 13);
  uart_read_uint8();
  uart_write((uint8_t*)"starting\r\n", 10);

  nrf24l01p_standby_to_rx();

  /* wait for the starting packet */
  if (nrf24l01p_is_rx_full()) nrf24l01p_flush_rx();
  while (nrf24l01p_is_rx_irq() == 0) ;
  nrf24l01p_read_rx();

  /* clock source disabled, safe to access 16 bits counter */
  TCNT1 = 0;

  /* prescaler set to 256 */
  /* interrupt every 16000000 / (65535 * 256) = 0.954 s */
  TCCR1B = 1 << 2;

  while (1)
  {
    if (is_timer1_irq == 1)
    {
      break ;
    }
    else if (nrf24l01p_is_rx_irq())
    {
      nrf24l01p_read_rx();
      ++counter;
    }
  }

  /* print counter */
  uart_write((uint8_t*)"counter: ", 9);
  uart_write((uint8_t*)uint16_to_string(counter), 4);
  uart_write((uint8_t*)"\r\n", 2);

  /* still in rx mode */
  nrf24l01p_set_standby();

  goto redo_receive;

  return 0;
}
