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

  for (i = 0, x = 0x2a; i != nrf905_payload_width; ++i, ++x)
    nrf905_payload_buf[i] = x;
}

static void wait(void)
{
  volatile uint16_t x;
  volatile uint16_t xx;

  for (xx = 0; xx < 5; ++xx)
    for (x = 0; x != (uint16_t)0xffff; ++x)
      __asm__ __volatile__ ("nop");
}

int main(void)
{
  uint8_t tx_addr[3] = { 0x01, 0x02, 0x03 };
  uint8_t rx_addr[3] = { 0x0a, 0x0b, 0x0c };

  /* setup spi first */
  spi_setup_master();
  spi_set_sck_freq(SPI_SCK_FREQ_FOSC2);

  uart_setup();

  nrf905_setup();
  nrf905_set_tx_addr(tx_addr, 3);
  nrf905_set_rx_addr(rx_addr, 3);

  nrf905_set_standby();

#if 0
  {
    uint8_t x;
    uart_write((uint8_t*)"tx side\r\n", 9);
    uart_write((uint8_t*)"press space\r\n", 13);
    uart_read_uint8(&x);
    uart_write((uint8_t*)"starting\r\n", 10);
  }
#endif

  while (1)
  {
    wait();

    make_pattern();
    nrf905_write_payload();

    uart_write((uint8_t*)"x\r\n", 3);
  }

  return 0;
}
