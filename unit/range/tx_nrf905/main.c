#include <stdint.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include "../../../src/spi.c"
#include "../../../src/nrf905.c"
#include "../../../src/uart.c"

static void make_pattern(void)
{
  uint8_t i;
  uint8_t x;

  for (i = 0, x = 0x2a; i < nrf905_payload_width; ++i, ++x)
    nrf905_payload_buf[i] = x;
}

int main(void)
{
  uint8_t tx_addr[3] = { 0x01, 0x02, 0x03 };
  uint8_t rx_addr[3] = { 0x0a, 0x0b, 0x0c };

  uint32_t counter = 0;
#if 0
  uint8_t x;
#endif

  /* setup spi first */
  spi_setup_master();
  spi_set_sck_freq(SPI_SCK_FREQ_FOSC2);

  uart_setup();

  nrf905_setup();
  nrf905_set_tx_addr(tx_addr, 3);
  nrf905_set_rx_addr(rx_addr, 3);

  nrf905_set_standby();

#if 0
  uart_write((uint8_t*)"tx side\r\n", 9);
  uart_write((uint8_t*)"press space\r\n", 13);
  uart_read_uint8(&x);
  uart_write((uint8_t*)"starting\r\n", 10);
#endif

  while (1)
  {
    if ((++counter) != (200000UL / 8UL)) continue ;
    counter = 0;
    make_pattern();
    nrf905_write_payload();
  }

  return 0;
}
