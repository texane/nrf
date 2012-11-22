#include <stdint.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include "../tx/adc8327.c"
#include "../../common/uart.c"


int main(void)
{
  volatile uint16_t x;

  uart_setup();

  while (1)
  {
    for (x = 0; x < 10000; ++x) ;
    uart_write(uint16_to_string(adc8327_read()), 4);
    uart_write((uint8_t*)"\r\n", 2);
  }

  return 0;
}
