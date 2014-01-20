#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "driverlib/debug.h"
#include "driverlib/fpu.h"
#include "driverlib/gpio.h"
#include "driverlib/pin_map.h"
#include "driverlib/rom.h"
#include "driverlib/sysctl.h"
#include "utils/uartstdio.h"

#include "./nrf905_lm4f120xl.c"

#ifdef DEBUG
void __error__(char* pcFilename, unsigned long ulLine)
{
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
  ROM_FPULazyStackingEnable();
  ROM_SysCtlClockSet(SYSCTL_SYSDIV_4 | SYSCTL_USE_PLL | SYSCTL_XTAL_16MHZ | SYSCTL_OSC_MAIN);
  ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);
  ROM_GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE, GPIO_PIN_2);

  ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
  ROM_GPIOPinConfigure(GPIO_PA0_U0RX);
  ROM_GPIOPinConfigure(GPIO_PA1_U0TX);
  ROM_GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);
  UARTStdioInit(0);
  UARTprintf("Hello, world!\n");

  /* ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOD); */
  /* const unsigned long addr = GPIO_PORTD_BASE + (GPIO_PIN_2 << 2); */
  /* MAP_GPIOPinTypeGPIOOutput */
  /* ( */
  /*  addr & 0xfffff000, */
  /*  (addr & 0x00000fff) >> 2 */
  /* ); */
  /* HWREG(addr) = 0xff; */


  uint8_t tx_addr[3] = { 0x01, 0x02, 0x03 };
  uint8_t rx_addr[3] = { 0x0a, 0x0b, 0x0c };

  spi_setup_master();
  nrf905_setup();
  nrf905_set_tx_addr(tx_addr, 3);
  nrf905_set_rx_addr(rx_addr, 3);

  nrf905_set_standby();

  UARTprintf("starting!\n");

  while(1)
  {
    ROM_SysCtlDelay(10000000);

    make_pattern();
    nrf905_write_payload();

    UARTprintf("x\n");
  }
}
