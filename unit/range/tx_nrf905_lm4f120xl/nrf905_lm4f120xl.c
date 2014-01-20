#include <stdint.h>

/* platform specific */

#if defined(CONFIG_LM4F120XL)

#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "driverlib/gpio.h"
#include "driverlib/rom_map.h"
#include "driverlib/sysctl.h"
#include "driverlib/systick.h"
#include "../../../src/spi_lm4f120xl.c"

static unsigned long nrf905_txe_addr;
static unsigned long nrf905_trx_addr;
static unsigned long nrf905_pwr_addr;
static unsigned long nrf905_cd_addr;
static unsigned long nrf905_am_addr;
static unsigned long nrf905_dr_addr;

static void nrf905_setup_txe(void)
{
  SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB);

  nrf905_txe_addr = GPIO_PORTB_BASE + (GPIO_PIN_4 << 2);

  MAP_GPIOPinTypeGPIOOutput
  (
   nrf905_txe_addr & 0xfffff000,
   (nrf905_txe_addr & 0x00000fff) >> 2
  );
}

static void nrf905_set_txe(void)
{
  HWREG(nrf905_txe_addr) = 0xff;
}

static void nrf905_clear_txe(void)
{
  HWREG(nrf905_txe_addr) = 0x00;
}

static void nrf905_setup_trx(void)
{
  SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOE);

  nrf905_trx_addr = GPIO_PORTE_BASE + (GPIO_PIN_4 << 2);

  MAP_GPIOPinTypeGPIOOutput
  (
   nrf905_trx_addr & 0xfffff000,
   (nrf905_trx_addr & 0x00000fff) >> 2
  );
}

static void nrf905_set_trx(void)
{
  HWREG(nrf905_trx_addr) = 0xff;
}

static void nrf905_clear_trx(void)
{
  HWREG(nrf905_trx_addr) = 0x00;
}

static void nrf905_setup_pwr(void)
{
  SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOE);

  nrf905_pwr_addr = GPIO_PORTE_BASE + (GPIO_PIN_5 << 2);

  MAP_GPIOPinTypeGPIOOutput
  (
   nrf905_pwr_addr & 0xfffff000,
   (nrf905_pwr_addr & 0x00000fff) >> 2
  );
}

static void nrf905_set_pwr(void)
{
  HWREG(nrf905_pwr_addr) = 0xff;
}

static void nrf905_clear_pwr(void)
{
  HWREG(nrf905_pwr_addr) = 0x00;
}

static void nrf905_setup_cd(void)
{
  SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB);

  nrf905_cd_addr = GPIO_PORTB_BASE + (GPIO_PIN_1 << 2);

  MAP_GPIOPinTypeGPIOInput
  (
   nrf905_cd_addr & 0xfffff000,
   (nrf905_cd_addr & 0x00000fff) >> 2
  );
}

static uint8_t nrf905_is_cd(void)
{
  return HWREG(nrf905_cd_addr);
}

static void nrf905_setup_dr(void)
{
  SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB);

  nrf905_dr_addr = GPIO_PORTB_BASE + (GPIO_PIN_0 << 2);

  MAP_GPIOPinTypeGPIOInput
  (
   nrf905_dr_addr & 0xfffff000,
   (nrf905_dr_addr & 0x00000fff) >> 2
  );
}

static uint8_t nrf905_is_dr(void)
{
  return HWREG(nrf905_dr_addr);
}

static void nrf905_setup_am(void)
{
  SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB);

  nrf905_am_addr = GPIO_PORTB_BASE + (GPIO_PIN_5 << 2);

  MAP_GPIOPinTypeGPIOInput
  (
   nrf905_am_addr & 0xfffff000,
   (nrf905_am_addr & 0x00000fff) >> 2
  );
}

static uint8_t nrf905_is_am(void)
{
  return HWREG(nrf905_am_addr);
}

#else /* ATMEGA328P */

#include <avr/io.h>
#include "spi.c"

/* hardcoded in spi.c */
#define NRF905_IO_CSN_MASK (1 << 2)
#define NRF905_IO_CSN_DDR DDRB
#define NRF905_IO_CSN_PORT PORTB
#define NRF905_IO_MOSI_MASK (1 << 3)
#define NRF905_IO_MOSI_DDR DDRB
#define NRF905_IO_MOSI_PORT PORTB
#define NRF905_IO_MISO_MASK (1 << 4)
#define NRF905_IO_MISO_DDR DDRB
#define NRF905_IO_MISO_PIN PINB
#define NRF905_IO_SCK_MASK (1 << 5)
#define NRF905_IO_SCK_DDR DDRB
#define NRF905_IO_SCK_PORT PORTB

/* nrf905 misc pins */
#define NRF905_IO_TXE_MASK (1 << 2)
#define NRF905_IO_TXE_DDR DDRD
#define NRF905_IO_TXE_PORT PORTD
#define NRF905_IO_PWR_MASK (1 << 3)
#define NRF905_IO_PWR_DDR DDRD
#define NRF905_IO_PWR_PORT PORTD
/* TRX is also known as CE */
#define NRF905_IO_TRX_MASK (1 << 4)
#define NRF905_IO_TRX_DDR DDRD
#define NRF905_IO_TRX_PORT PORTD
#define NRF905_IO_CD_MASK (1 << 5)
#define NRF905_IO_CD_DDR DDRD
#define NRF905_IO_CD_PIN PIND
#define NRF905_IO_DR_MASK (1 << 6)
#define NRF905_IO_DR_DDR DDRD
#define NRF905_IO_DR_PIN PIND
#define NRF905_IO_AM_MASK (1 << 7)
#define NRF905_IO_AM_DDR DDRD
#define NRF905_IO_AM_PIN PIND

static inline void spi_setup_cs(void)
{
  /* spi cs, pb2 */
  NRF905_IO_CSN_DDR |= NRF905_IO_CSN_MASK;
}

static inline void spi_cs_low(void)
{
  NRF905_IO_CSN_PORT &= ~NRF905_IO_CSN_MASK;
}

static inline void spi_cs_high(void)
{
  NRF905_IO_CSN_PORT |= NRF905_IO_CSN_MASK;
}

static void nrf905_setup_txe(void)
{
  NRF905_IO_TXE_DDR |= NRF905_IO_TXE_MASK;
}

static void nrf905_set_txe(void)
{
  NRF905_IO_TXE_PORT |= NRF905_IO_TXE_MASK;
}

static void nrf905_clear_txe(void)
{
  NRF905_IO_TXE_PORT &= ~NRF905_IO_TXE_MASK;
}

static void nrf905_setup_trx(void)
{
  NRF905_IO_TRX_DDR |= NRF905_IO_TRX_MASK;
}

static void nrf905_set_trx(void)
{
  NRF905_IO_TRX_PORT |= NRF905_IO_TRX_MASK;
}

static void nrf905_clear_trx(void)
{
  NRF905_IO_TRX_PORT &= ~NRF905_IO_TRX_MASK;
}

static void nrf905_setup_pwr(void)
{
  NRF905_IO_PWR_DDR |= NRF905_IO_PWR_MASK;
}

static void nrf905_set_pwr(void)
{
  NRF905_IO_PWR_PORT |= NRF905_IO_PWR_MASK;
}

static void nrf905_clear_pwr(void)
{
  NRF905_IO_PWR_PORT &= ~NRF905_IO_PWR_MASK;
}

static void nrf905_setup_cd(void)
{
  NRF905_IO_CD_DDR &= ~NRF905_IO_CD_MASK;
}

static uint8_t nrf905_is_cd(void)
{
  /* carrier detect */
  return NRF905_IO_CD_PIN & NRF905_IO_CD_MASK;
}

static void nrf905_setup_dr(void)
{
  NRF905_IO_DR_DDR &= ~NRF905_IO_DR_MASK;
}

static uint8_t nrf905_is_dr(void)
{
  /* data ready */
  return NRF905_IO_DR_PIN & NRF905_IO_DR_MASK;
}

static void nrf905_setup_am(void)
{
  NRF905_IO_AM_DDR &= ~NRF905_IO_AM_MASK;
}

static uint8_t nrf905_is_am(void)
{
  /* address match */
  return NRF905_IO_AM_PIN & NRF905_IO_AM_MASK;
}

#endif /* CONFIG_LM4F120XL */

/* global buffers */

static uint8_t nrf905_config[10];
static uint8_t nrf905_payload_buf[32];
static uint8_t nrf905_payload_width = 16;

/* spi communication */

#define NRF905_CMD_WC 0x00
#define NRF905_CMD_RC 0x10
#define NRF905_CMD_WTP 0x20
#define NRF905_CMD_RTP 0x21
#define NRF905_CMD_WTA 0x22
#define NRF905_CMD_RTA 0x23
#define NRF905_CMD_RRP 0x24

static void nrf905_cmd_prolog(uint8_t cmd)
{
  /* in case it is already selected */
  spi_cs_high();
  spi_cs_low();
  spi_write_uint8(cmd);
}

static void nrf905_cmd_read(uint8_t cmd, uint8_t* buf, uint8_t size)
{
  nrf905_cmd_prolog(cmd);
  spi_read(buf, size);
  spi_cs_high();
}

static void nrf905_cmd_write(uint8_t cmd, const uint8_t* buf, uint8_t size)
{
  nrf905_cmd_prolog(cmd);
  spi_write(buf, size);
  spi_cs_high();
}

static void nrf905_cmd_wc(void)
{
  nrf905_cmd_write(NRF905_CMD_WC, nrf905_config, sizeof(nrf905_config));
}

static void nrf905_cmd_rc(void)
{
  nrf905_cmd_read(NRF905_CMD_RC, nrf905_config, sizeof(nrf905_config));
}

static void nrf905_cmd_wtp(void)
{
  nrf905_cmd_write(NRF905_CMD_WTP, nrf905_payload_buf, nrf905_payload_width);
}

static void nrf905_cmd_rtp(void)
{
  nrf905_cmd_read(NRF905_CMD_RTP, nrf905_payload_buf, nrf905_payload_width);
}

static void nrf905_cmd_wta(const uint8_t* a, uint8_t w)
{
  nrf905_cmd_write(NRF905_CMD_WTA, a, w);
}

static void nrf905_cmd_rta(uint8_t* a, uint8_t w)
{
  nrf905_cmd_read(NRF905_CMD_RTA, a, w);
}

static void nrf905_cmd_rrp(void)
{
  nrf905_cmd_read(NRF905_CMD_RRP, nrf905_payload_buf, nrf905_payload_width);
}

static uint8_t nrf905_read_status(void)
{
  /* status available on miso on high to low transition */

  uint8_t x;

  /* in case it is already selected */
  spi_cs_high();

  spi_cs_low();
  x = spi_read_uint8();
  spi_cs_high();

  return x;
}

/* configuration register */
/* wtp must be used to commit the operation */

static void nrf905_clear_set_config(uint8_t i, uint8_t j, uint8_t w, uint8_t x)
{
  /* i the byte offset */
  /* j the bit offset */
  /* w the width */
  /* x the value */

  nrf905_config[i] &= ~(((1 << w) - 1) << j);
  nrf905_config[i] |= x << j;
}

static void nrf905_set_ch_no(uint16_t x)
{
  /* config[0:8] */
  nrf905_clear_set_config(0, 0, 8, x & 0xff);
  nrf905_clear_set_config(1, 0, 1, x >> 8);
}

static void nrf905_set_hfreq_pll(uint8_t x)
{
  /* config[9] */
  nrf905_clear_set_config(1, 1, 1, x);
}

static void nrf905_set_pa_pwr(uint8_t x)
{
  /* config[10:11] */
  nrf905_clear_set_config(1, 2, 2, x);
}

static void nrf905_set_rx_red_pwr(uint8_t x)
{
  /* config[12] */
  nrf905_clear_set_config(1, 3, 1, x);
}

static void nrf905_set_auto_retran(uint8_t x)
{
  /* config[13] */
  nrf905_clear_set_config(1, 4, 1, x);
}

static void nrf905_set_rx_afw(uint8_t x)
{
  /* config[14:16] */
  nrf905_clear_set_config(1, 5, 2, x & 3);
  nrf905_clear_set_config(2, 0, 1, x >> 2);
}

static void nrf905_set_tx_afw(uint8_t x)
{
  /* config[17:19] */
  nrf905_clear_set_config(2, 1, 3, x);
}

static void nrf905_set_rx_pw(uint8_t x)
{
  /* config[20:25] */
  nrf905_clear_set_config(2, 4, 4, x);
  nrf905_clear_set_config(3, 0, 2, x >> 4);
}

static void nrf905_set_tx_pw(uint8_t x)
{
  /* config[26:31] */
  nrf905_clear_set_config(3, 3, 6, x);
}

static void __nrf905_set_rx_addr(const uint8_t* a, uint8_t w)
{
  /* config[32:63] */
  uint8_t i;
  for (i = 0; i != w; ++i) nrf905_clear_set_config(4 + i, 0, 8, a[i]);
}

static void nrf905_set_up_clk_freq(uint8_t x)
{
  /* config[64:65] */
  nrf905_clear_set_config(8, 0, 2, x);
}

static void nrf905_set_up_clk_en(uint8_t x)
{
  /* config[66] */
  nrf905_clear_set_config(8, 2, 1, x);
}

static void nrf905_set_xof(uint8_t x)
{
  /* config[67:69] */
  nrf905_clear_set_config(8, 3, 3, x);
}

static void nrf905_set_crc_en(uint8_t x)
{
  /* config[70] */
  nrf905_clear_set_config(8, 6, 1, x);
}

static void nrf905_set_crc_mode(uint8_t x)
{
  /* config[71] */
  nrf905_clear_set_config(8, 7, 1, x);
}

/* rf channel */
/* f = (422.4 + (ch_no / 10)) * (1 + hfreq_pll) MHz */
/* example are take from datasheet */

static void nrf905_set_channel(uint8_t hfreq_pll, uint16_t ch_no)
{
  nrf905_set_hfreq_pll(hfreq_pll);
  nrf905_set_ch_no(ch_no);
}

static void nrf905_set_channel_430_0(void)
{
  nrf905_set_channel(0, 0x004a);
}

static void nrf905_set_channel_433_1(void)
{
  nrf905_set_channel(0, 0x006b);
}

static void nrf905_set_channel_433_2(void)
{
  nrf905_set_channel(0, 0x006c);
}

static void nrf905_set_channel_434_7(void)
{
  nrf905_set_channel(0, 0x007b);
}

static void nrf905_set_channel_862_0(void)
{
  nrf905_set_channel(1, 0x0056);
}

static void nrf905_set_channel_868_2(void)
{
  nrf905_set_channel(1, 0x0075);
}

static void nrf905_set_channel_868_4(void)
{
  /* eu */
  nrf905_set_channel(1, 0x0076);
}

static void nrf905_set_channel_868_6(void)
{
  nrf905_set_channel(1, 0x0077);
}

static void nrf905_set_channel_869_8(void)
{
  nrf905_set_channel(1, 0x007d);
}

static void nrf905_set_channel_902_2(void)
{
  nrf905_set_channel(1, 0x011f);
}

static void nrf905_set_channel_902_4(void)
{
  nrf905_set_channel(1, 0x0120);
}

static void nrf905_set_channel_927_8(void)
{
  nrf905_set_channel(1, 0x19f);
}

/* control signals */


/* operating modes */

static void nrf905_set_powerdown(void)
{
  /* minimize power consumption to 3uA */
  /* configuration word is maintained */

  nrf905_clear_pwr();
}

static void nrf905_set_standby(void)
{
  /* minimize power consumption to 50uA */
  /* maintains short startup time */

  nrf905_set_trx();
  nrf905_set_pwr();
}

static void nrf905_set_tx(void)
{
  nrf905_set_txe();
  nrf905_set_trx();
  nrf905_set_pwr();
}

static void nrf905_set_rx(void)
{
  nrf905_clear_txe();
  nrf905_set_trx();
  nrf905_set_pwr();
}

static void nrf905_disable_trxx(void)
{
  nrf905_clear_trx();
}

/* exported */

static void nrf905_write_payload(void)
{
  nrf905_cmd_write(NRF905_CMD_WTP, nrf905_payload_buf, nrf905_payload_width);

  /* send the packet */
  nrf905_set_tx();

  /* a packet is always transmitted */
  /* before the new mode is applied */

  /* leave in standby mode */
  nrf905_set_standby();
}

static void nrf905_read_payload(void)
{
  /* dr and am set low after payload read */

  nrf905_cmd_read(NRF905_CMD_RRP, nrf905_payload_buf, nrf905_payload_width);
}

static void nrf905_set_tx_addr(const uint8_t* a, uint8_t w)
{
  /* a[w] the address of w bytes */

  nrf905_set_tx_afw(w);
  nrf905_cmd_wc();

  nrf905_cmd_wta(a, w);
}

static void nrf905_set_rx_addr(const uint8_t* a, uint8_t w)
{
  /* config[32:63] */
  uint8_t i;
  for (i = 0; i != w; ++i)
    nrf905_clear_set_config(4 + i, 0, 8, a[i]);
  nrf905_set_rx_afw(w);
  nrf905_cmd_wc();
}

static void nrf905_setup(void)
{
#if defined(CONFIG_LM4F120XL)
  /* cs already setup in softspi.c */
#else
  spi_setup_cs();
#endif

  spi_cs_high();

  /* control signals */
  nrf905_setup_txe();
  nrf905_setup_trx();
  nrf905_setup_pwr();
  nrf905_setup_cd();
  nrf905_setup_am();
  nrf905_setup_dr();

  /* default config */
  /* nrf905_set_channel_868_4(); */
  nrf905_set_channel_868_6();
  /* -10db */
  nrf905_set_pa_pwr(0);
  /* power reduction disabled */
  nrf905_set_rx_red_pwr(0);
  /* auto retransmission disabled */
  nrf905_set_auto_retran(0);
  /* 24 bits addresses */
  nrf905_set_rx_afw(3);
  nrf905_set_tx_afw(3);
  nrf905_set_rx_pw(nrf905_payload_width);
  nrf905_set_tx_pw(nrf905_payload_width);
  /* 500KHz output clock frequency */
  nrf905_set_up_clk_freq(3);
  /* external clock signal disabled */
  nrf905_set_up_clk_en(0);
  /* 16MHz crystal frequency */
  nrf905_set_xof(3);
  nrf905_set_crc_en(0);
  nrf905_set_crc_mode(0);

  /* commit the configuration */
  nrf905_cmd_wc();

  /* default to powerdown */
  nrf905_set_powerdown();
}
