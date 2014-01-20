#include <stdint.h>
#include "../../../src/nrf905.c"

/* platform specific */

#if defined(CONFIG_LM4F120XL)

#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "driverlib/debug.h"
#include "driverlib/fpu.h"
#include "driverlib/gpio.h"
#include "driverlib/pin_map.h"
#include "driverlib/rom.h"
#include "driverlib/sysctl.h"
#include "utils/uartstdio.h"

static void sys_setup(void)
{
  ROM_FPULazyStackingEnable();
  ROM_SysCtlClockSet(SYSCTL_SYSDIV_4 | SYSCTL_USE_PLL | SYSCTL_XTAL_16MHZ | SYSCTL_OSC_MAIN);
  ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);
  ROM_GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE, GPIO_PIN_2);
}

static void uart_setup(void)
{
  ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
  ROM_GPIOPinConfigure(GPIO_PA0_U0RX);
  ROM_GPIOPinConfigure(GPIO_PA1_U0TX);
  ROM_GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);
  UARTStdioInit(0);
}

static void uart_write(const uint8_t* s, uint8_t n)
{
  uint8_t i;
  for (i = 0; i != n; ++i) UARTprintf("%c", s[i]);
}

static void uart_read_uint8(uint8_t* x)
{
}

static void wait(void)
{
  ROM_SysCtlDelay(10000000);
}

#else

#include <avr/io.h>
#include <avr/interrupt.h>
#include "../../../src/uart.c"

static void sys_setup(void)
{
}

static void wait(void)
{
  volatile uint16_t x;
  volatile uint16_t xx;

  for (xx = 0; xx < 5; ++xx)
    for (x = 0; x != (uint16_t)0xffff; ++x)
      __asm__ __volatile__ ("nop");
}

#endif

static void make_pattern(void)
{
  uint8_t i;
  uint8_t x;

  for (i = 0, x = 0x2a; i != nrf905_payload_width; ++i, ++x)
    nrf905_payload_buf[i] = x;
}

int main(void)
{
  uint8_t tx_addr[3] = { 0x01, 0x02, 0x03 };
  uint8_t rx_addr[3] = { 0x0a, 0x0b, 0x0c };

  sys_setup();

  uart_setup();

  /* setup spi first */
  spi_setup_master();
  spi_set_sck_freq(SPI_SCK_FREQ_FOSC2);

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
