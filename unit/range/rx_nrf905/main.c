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

static void led_setup(void)
{
#define LED_RED GPIO_PIN_1
#define LED_BLUE GPIO_PIN_2
#define LED_GREEN GPIO_PIN_3
  ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);
  ROM_GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE, LED_RED | LED_BLUE | LED_GREEN);
}

static void led_set(uint32_t mask)
{
  ROM_GPIOPinWrite(GPIO_PORTF_BASE, LED_RED | LED_BLUE | LED_GREEN, mask);
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

static void led_setup(void)
{
}

static void led_set(uint32_t x)
{
#define LED_RED 0
#define LED_BLUE 0
#define LED_GREEN 0
}

#endif

static void reset_pattern(void)
{
  uint8_t i;

  for (i = 0; i != nrf905_payload_width; ++i)
    nrf905_payload_buf[i] = 0;
}

static int check_pattern(void)
{
  uint8_t i;

  for (i = 0; i != nrf905_payload_width; ++i)
  {
    if (nrf905_payload_buf[i] != 0x2a) return -1;
  }

  return 0;
}

static uint8_t to_hex(uint8_t x)
{
  if ((x >= 0) && (x <= 9)) return '0' + x;
  return 'a' + (x - 0xa);
}

static void dump_buf(const uint8_t* buf, uint8_t n)
{
  uint8_t c;
  uint8_t i;

  for (i = 0; i != n; ++i)
  {
    c = to_hex(buf[i] >> 4);
    uart_write(&c, 1);
    c = to_hex(buf[i] & 0xf);
    uart_write(&c, 1);
  }

  uart_write((const uint8_t*)"\r\n", 2);
}

static void dump_config(void)
{
  dump_buf(nrf905_config, sizeof(nrf905_config));
}

int main(void)
{
  uint8_t tx_addr[4] = { 0xaa, 0xab, 0xac, 0xad };
  uint8_t rx_addr[4] = { 0xa1, 0xa2, 0xa3, 0xa4 };

  sys_setup();

  uart_setup();

  /* setup spi first */
  spi_setup_master();
  spi_set_sck_freq(SPI_SCK_FREQ_FOSC2);

  nrf905_setup();
  nrf905_set_tx_addr(tx_addr, 4);
  nrf905_set_rx_addr(rx_addr, 4);
  nrf905_set_payload_width(2);
  nrf905_commit_config();

#if 0
  {
    uint8_t i;
    dump_config();
    for (i = 0; i != sizeof(nrf905_config); ++i)
      nrf905_config[i] = 0x2a;
    nrf905_cmd_rc();
    dump_config();
  }
#endif

#if 0
  {
    uint8_t x;
    uart_write((uint8_t*)"rx side\r\n", 9);
    uart_write((uint8_t*)"press space\r\n", 13);
    uart_read_uint8(&x);
    uart_write((uint8_t*)"starting\r\n", 10);
  }
#endif

  led_setup();

  nrf905_set_rx();

  while (1)
  {
  wait_cd:
    if (nrf905_is_cd() == 0)
    {
      led_set(LED_RED);
      while (nrf905_is_cd() == 0) ;
      /* uart_write((uint8_t*)"cd\r\n", 4); */
    }

#if 0
    while (nrf905_is_am() == 0) ;
    uart_write((uint8_t*)"am\r\n", 4);
    led_mask = LED_GREEN;
#endif

    led_set(LED_GREEN);

  wait_dr:
    while (nrf905_is_dr() == 0)
    {
      if (nrf905_is_cd() == 0) goto wait_cd;
    }
    uart_write((uint8_t*)"dr\r\n", 4);

    reset_pattern();

    /* wait(); */
    nrf905_read_payload();

    if (check_pattern() == 0)
    {
      uart_write((uint8_t*)"ok\r\n", 4);
      led_set(LED_BLUE);
    }
    else led_set(LED_GREEN);

    goto wait_dr;
  }

  return 0;
}
