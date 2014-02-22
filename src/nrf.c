/* nrf abstract layer */

#include <avr/io.h>


#if (NRF_CONFIG_NRF24L01P == 1)
#include "./nrf24l01p.c"
#elif (NRF_CONFIG_NRF905 == 1)
#include "./nrf905.c"
#else
#error "NRF_CONFIG_xxx not defined"
#endif


static inline void nrf_setup(void)
{
  /* setup spi first */
  spi_setup_master();
  spi_set_sck_freq(SPI_SCK_FREQ_FOSC2);

#if (NRF_CONFIG_NRF24L01P == 1)

  /* nrf24l01p default configuration */
  nrf24l01p_setup();
  nrf24l01p_disable_crc();
  nrf24l01p_set_rate(NRF24L01P_RATE_2MBPS);
  nrf24l01p_set_chan(2);
  nrf24l01p_set_addr_width(NRF24L01P_ADDR_WIDTH_4);
  nrf24l01p_enable_tx_noack();

#elif (NRF_CONFIG_NRF905 == 1)

  nrf905_setup();

#endif
}

static inline void nrf_set_rx_mode(void)
{
  /* NOTE: enter in powerdown, leave in rx mode */

#if (NRF_CONFIG_NRF24L01P == 1)

  nrf24l01p_powerdown_to_standby();
  nrf24l01p_standby_to_rx();

#elif (NRF_CONFIG_NRF905 == 1)

  nrf905_set_rx();

#endif
}

static inline void nrf_set_powerdown_mode(void)
{
#if (NRF_CONFIG_NRF24L01P == 1)

  nrf24l01p_set_powerdown();

#elif (NRF_CONFIG_NRF905 == 1)

  nrf905_set_standby();

#endif
}

static inline void nrf_setup_rx_irq(void)
{
#if (NRF_CONFIG_NRF24L01P == 1)

  /* setup interrupt on change. disable pullup. */

  NRF24L01P_IO_IRQ_DDR &= ~NRF24L01P_IO_IRQ_MASK;
  NRF24L01P_IO_IRQ_PORT &= ~NRF24L01P_IO_IRQ_MASK;
  PCICR |= NRF24L01P_IO_IRQ_PCICR_MASK;
  NRF24L01P_IO_IRQ_PCMSK |= NRF24L01P_IO_IRQ_MASK;

#elif (NRF_CONFIG_NRF905 == 1)

  NRF905_IO_IRQ_DDR &= ~NRF905_IO_IRQ_MASK;
  NRF905_IO_IRQ_PORT &= ~NRF905_IO_IRQ_MASK;
  PCICR |= NRF905_IO_IRQ_PCICR_MASK;
  NRF905_IO_IRQ_PCMSK |= NRF905_IO_IRQ_MASK;

#endif
}

static inline void nrf_disable_rx_irq(void)
{
#if (NRF_CONFIG_NRF24L01P == 1)

  NRF24L01P_IO_IRQ_PCMSK &= ~NRF24L01P_IO_IRQ_MASK;

#elif (NRF_CONFIG_NRF905 == 1)

  NRF905_IO_IRQ_PCMSK &= ~NRF905_IO_IRQ_MASK;

#endif
}

static inline void nrf_enable_rx_irq(void)
{
#if (NRF_CONFIG_NRF24L01P == 1)

  NRF24L01P_IO_IRQ_PCMSK |= NRF24L01P_IO_IRQ_MASK;

#elif (NRF_CONFIG_NRF905 == 1)

  NRF905_IO_IRQ_PCMSK |= NRF905_IO_IRQ_MASK;

#endif
}

static inline uint8_t nrf_get_rx_irq(void)
{
  /* NOTE: get and acknowledge irq if required */

#if (NRF_CONFIG_NRF24L01P == 1)

  return nrf24l01p_is_rx_irq();

#elif (NRF_CONFIG_NRF905 == 1)

  return nrf905_is_dr();

#endif
}

static inline uint8_t nrf_peek_rx_irq(void)
{
  /* NOTE: get but do not acknowledge irq */

#if (NRF_CONFIG_NRF24L01P == 1)

  return nrf24l01p_is_rx_irq_noread();

#elif (NRF_CONFIG_NRF905 == 1)

  return nrf905_is_dr();

#endif
}

static inline uint8_t nrf_read_payload(uint8_t** buf)
{
#if (NRF_CONFIG_NRF24L01P == 1)

  nrf24l01p_read_rx();
  *buf = nrf24l01p_cmd_buf;
  return nrf24l01p_cmd_len;

#elif (NRF_CONFIG_NRF905 == 1)

  nrf905_read_payload();
  *buf = nrf905_payload_buf;
  return nrf905_payload_width;

#endif
}

static void nrf_read_payload_zero(uint8_t* p)
{
#if (NRF_CONFIG_NRF24L01P == 1)

  nrf24l01p_read_rx_zero(p);

#elif (NRF_CONFIG_NRF905 == 1)

  nrf905_read_payload_zero(p);

#endif
}

static inline void nrf_send_payload_zero(uint8_t* p)
{
 /* NOTE: enter and leave in powerdown mode */

#if (NRF_CONFIG_NRF24L01P == 1)

  nrf24l01p_powerdown_to_standby();
  nrf24l01p_standby_to_tx();
  nrf24l01p_write_tx_noack_zero(p);
  nrf24l01p_complete_tx_noack_zero();
  while (nrf24l01p_is_tx_irq() == 0) ;
  nrf24l01p_set_powerdown();

#elif (NRF_CONFIG_NRF905 == 1)

  nrf905_write_payload_zero(p);

#endif
}

static inline void nrf_set_payload_width(uint8_t x)
{
#if (NRF_CONFIG_NRF24L01P == 1)

  nrf24l01p_set_payload_width(x);

#elif (NRF_CONFIG_NRF905 == 1)

  nrf905_set_payload_width(x);
  nrf905_commit_config();

#endif
}

static inline uint8_t nrf_get_payload_width(void)
{
#if (NRF_CONFIG_NRF24L01P == 1)

  return nrf24l01p_read_reg8(NRF24L01P_REG_RX_PW_P0);

#elif (NRF_CONFIG_NRF905 == 1)

  return nrf905_get_payload_width();

#endif
}

static inline uint8_t nrf_set_addr_width(uint8_t x)
{
  /* x in bytes */

#if (NRF_CONFIG_NRF24L01P == 1)

  static const uint8_t map[] =
  {
    NRF24L01P_ADDR_WIDTH_3,
    NRF24L01P_ADDR_WIDTH_4,
    NRF24L01P_ADDR_WIDTH_5
  };

  if (x < 3) return (uint8_t)-1;
  x -= 3;

#define NRF_ARRAY_COUNT(__a) (sizeof(__a) / sizeof((__a)[0]))
  if (x >= NRF_ARRAY_COUNT(map)) return (uint8_t)-1;

  /* x in NRF24L01P_ADDR_WIDTH_xxx */
  nrf24l01p_set_addr_width(map[x]);

#elif (NRF_CONFIG_NRF905 == 1)

  if ((x < 1) || (x > 4)) return (uint8_t)-1;

  nrf905_set_rx_afw(x);
  nrf905_set_tx_afw(x);
  nrf905_commit_config();

#endif

  return 0;
}

static inline uint8_t nrf_get_addr_width(void)
{
  /* return the addr width in bytes */

#if (NRF_CONFIG_NRF24L01P == 1)

  return 3 + nrf24l01p_read_reg8(NRF24L01P_REG_SETUP_AW);

#elif (NRF_CONFIG_NRF905 == 1)

  return nrf905_get_tx_afw();

#endif
}

static inline void nrf_disable_crc(void)
{
#if (NRF_CONFIG_NRF24L01P == 1)

  nrf24l01p_disable_crc();

#elif (NRF_CONFIG_NRF905 == 1)

  nrf905_set_crc_en(0);
  nrf905_set_crc_mode(0);
  nrf905_commit_config();

#endif
}

static inline void nrf_enable_crc8(void)
{
#if (NRF_CONFIG_NRF24L01P == 1)

  nrf24l01p_enable_crc8();

#elif (NRF_CONFIG_NRF905 == 1)

  nrf905_set_crc_en(1);
  nrf905_set_crc_mode(0);
  nrf905_commit_config();

#endif
}

static inline void nrf_enable_crc16(void)
{
#if (NRF_CONFIG_NRF24L01P == 1)

  nrf24l01p_enable_crc16();

#elif (NRF_CONFIG_NRF905 == 1)

  nrf905_set_crc_en(1);
  nrf905_set_crc_mode(1);
  nrf905_commit_config();

#endif
}

static uint8_t nrf_get_crc(void)
{
  /* return the crc size in bytes */
#if (NRF_CONFIG_NRF24L01P == 1)

  const uint8_t x = nrf24l01p_read_reg8(NRF24L01P_REG_CONFIG);
  if ((x & (1 << 3)) == 0) return 0;
  else if ((x & (1 << 2)) == 0) return 1;
  else return 2;

#elif (NRF_CONFIG_NRF905 == 1)

  if (nrf905_get_crc_en() == 0) return 0;
  if (nrf905_get_crc_mode() == 0) return 1;
  return 2;

#endif
}

static inline void nrf_set_rx_addr(uint8_t* x)
{
#if (NRF_CONFIG_NRF24L01P == 1)

  nrf24l01p_cmd_buf[0] = x[0];
  nrf24l01p_cmd_buf[1] = x[1];
  nrf24l01p_cmd_buf[2] = x[2];
  nrf24l01p_cmd_buf[3] = x[3];
  nrf24l01p_cmd_buf[4] = 0x00;
  nrf24l01p_write_reg40(NRF24L01P_REG_RX_ADDR_P0);

#elif (NRF_CONFIG_NRF905 == 1)

  nrf905_set_rx_addr(x, nrf905_get_rx_afw());
  /* already commited */

#endif
}

static inline void nrf_set_tx_addr(uint8_t* x)
{
#if (NRF_CONFIG_NRF24L01P == 1)

  nrf24l01p_cmd_buf[0] = x[0];
  nrf24l01p_cmd_buf[1] = x[1];
  nrf24l01p_cmd_buf[2] = x[2];
  nrf24l01p_cmd_buf[3] = x[3];
  nrf24l01p_cmd_buf[4] = 0x00;
  nrf24l01p_write_reg40(NRF24L01P_REG_TX_ADDR);

#elif (NRF_CONFIG_NRF905 == 1)

  nrf905_set_tx_addr(x, nrf905_get_tx_afw());
  /* already commited */

#endif
}

static inline void nrf_disable_tx_ack(void)
{
#if (NRF_CONFIG_NRF24L01P == 1)

  nrf24l01p_enable_tx_noack();

#elif (NRF_CONFIG_NRF905 == 1)

  nrf905_set_auto_retran(0);
  nrf905_commit_config();

#endif
}

static inline void nrf_enable_tx_ack(void)
{
#if (NRF_CONFIG_NRF24L01P == 1)

  nrf24l01p_disable_tx_noack();

#elif (NRF_CONFIG_NRF905 == 1)

  nrf905_set_auto_retran(1);
  nrf905_commit_config();

#endif
}

static inline uint8_t nrf_get_tx_ack(void)
{
  /* return 0 if disabled, non zero otherwise */

#if (NRF_CONFIG_NRF24L01P == 1)

  const uint8_t x = nrf24l01p_read_reg8(NRF24L01P_REG_FEATURE);
  if (x & (1 << 0)) return 0;
  else return 1;

#elif (NRF_CONFIG_NRF905 == 1)

  return nrf905_get_auto_retran();

#endif
}

static inline void nrf_get_tx_addr(uint8_t* x)
{
  const uint8_t n = nrf_get_addr_width();

#if (NRF_CONFIG_NRF24L01P == 1)

  uint8_t i;
  nrf24l01p_read_reg40(NRF24L01P_REG_TX_ADDR);
  for (i = 0; i != n; ++i) x[i] = nrf24l01p_cmd_buf[i];

#elif (NRF_CONFIG_NRF905 == 1)

  nrf905_get_tx_addr(x, n);

#endif
}

static inline void nrf_get_rx_addr(uint8_t* x)
{
  const uint8_t n = nrf_get_addr_width();

#if (NRF_CONFIG_NRF24L01P == 1)

  uint8_t i;
  nrf24l01p_read_reg40(NRF24L01P_REG_RX_ADDR_P0);
  for (i = 0; i != n; ++i) x[i] = nrf24l01p_cmd_buf[i];

#elif (NRF_CONFIG_NRF905 == 1)

  nrf905_get_rx_addr(x, n);

#endif
}
