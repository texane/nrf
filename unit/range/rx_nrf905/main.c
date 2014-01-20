#include <stdint.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include "../../../src/spi.c"
#include "../../../src/nrf905.c"
#include "../../../src/uart.c"

int main(void)
{
  uint8_t tx_addr[3] = { 0x0a, 0x0b, 0x0c };
  uint8_t rx_addr[3] = { 0x01, 0x02, 0x03 };
  uint8_t i;

  /* setup spi first */
  spi_setup_master();
  spi_set_sck_freq(SPI_SCK_FREQ_FOSC2);

  uart_setup();

  nrf905_setup();
  nrf905_set_tx_addr(tx_addr, 3);
  nrf905_set_rx_addr(rx_addr, 3);

#if 1
  {
    uint8_t x;
    uart_write((uint8_t*)"rx side\r\n", 9);
    uart_write((uint8_t*)"press space\r\n", 13);
    uart_read_uint8(&x);
    uart_write((uint8_t*)"starting\r\n", 10);
  }
#endif

  nrf905_set_rx();

  while (nrf905_is_cd() == 0) ;
  uart_write((uint8_t*)"cd\r\n", 4);

  while (1)
  {
    nrf905_set_rx();

    while (nrf905_is_am() == 0) ;
    uart_write((uint8_t*)"am\r\n", 4);

    while (nrf905_is_dr() == 0) ;
    uart_write((uint8_t*)"dr\r\n", 4);

    nrf905_read_payload();

    for (i = 0; i != nrf905_payload_width; ++i)
      uart_write(nrf905_payload_buf + i, 1);
    uart_write((uint8_t*)"\r\n", 2);
  }

  return 0;
}
