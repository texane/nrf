#include <stdint.h>
#include <stddef.h>
#include <avr/io.h>
#include <avr/sleep.h>
#include <avr/interrupt.h>
#include "../common/snrf_common.h"
#include "../../../src/uart.c"

#define SNRF_CONFIG_NRF905 1

#if (SNRF_CONFIG_NRF24L01P == 1)
#include "../../../src/nrf24l01p.c"
#elif (SNRF_CONFIG_NRF905 == 1)
#include "../../../src/nrf905.c"
#else
#error "no transport layer defined"
#endif


/* nrf wrappers */

static inline void nrf_setup(void)
{
  /* setup spi first */
  spi_setup_master();
  spi_set_sck_freq(SPI_SCK_FREQ_FOSC2);

#if (SNRF_CONFIG_NRF24L01P == 1)

  /* nrf24l01p default configuration */
  nrf24l01p_setup();
  nrf24l01p_disable_crc();
  nrf24l01p_set_rate(NRF24L01P_RATE_2MBPS);
  nrf24l01p_set_chan(2);
  nrf24l01p_set_addr_width(NRF24L01P_ADDR_WIDTH_4);

  /* rx address */
  nrf24l01p_cmd_buf[0] = 0x10;
  nrf24l01p_cmd_buf[1] = 0x10;
  nrf24l01p_cmd_buf[2] = 0x10;
  nrf24l01p_cmd_buf[3] = 0x10;
  nrf24l01p_write_reg40(NRF24L01P_REG_RX_ADDR_P0);

  /* tx address */
  nrf24l01p_cmd_buf[0] = 0x20;
  nrf24l01p_cmd_buf[1] = 0x20;
  nrf24l01p_cmd_buf[2] = 0x20;
  nrf24l01p_cmd_buf[3] = 0x20;
  nrf24l01p_write_reg40(NRF24L01P_REG_TX_ADDR);

  /* enable tx no ack command */
  nrf24l01p_enable_tx_noack();

#elif (SNRF_CONFIG_NRF905 == 1)

  uint8_t tx_addr[4] = { 0x10, 0x10, 0x10, 0x10 };
  uint8_t rx_addr[4] = { 0x20, 0x20, 0x20, 0x20 };

  nrf905_setup();
  nrf905_set_tx_addr(tx_addr, 4);
  nrf905_set_rx_addr(rx_addr, 4);
  nrf905_set_payload_width(16);
  nrf905_commit_config();

#endif
}

static inline void nrf_set_rx_mode(void)
{
  /* NOTE: enter in powerdown, leave in rx mode */

#if (SNRF_CONFIG_NRF24L01P == 1)

  nrf24l01p_powerdown_to_standby();
  nrf24l01p_standby_to_rx();

#elif (SNRF_CONFIG_NRF905 == 1)

  nrf905_set_rx();

#endif
}

static inline void nrf_set_powerdown_mode(void)
{
#if (SNRF_CONFIG_NRF24L01P == 1)

  nrf24l01p_set_powerdown();

#elif (SNRF_CONFIG_NRF905 == 1)

  /* TODO: check the doc and current state */

#endif
}

static inline void nrf_setup_rx_irq(void)
{
#if (SNRF_CONFIG_NRF24L01P == 1)

  /* setup interrupt on change. disable pullup. */

  NRF24L01P_IO_IRQ_DDR &= ~NRF24L01P_IO_IRQ_MASK;
  NRF24L01P_IO_IRQ_PORT &= ~NRF24L01P_IO_IRQ_MASK;
  PCICR |= NRF24L01P_IO_IRQ_PCICR_MASK;
  NRF24L01P_IO_IRQ_PCMSK |= NRF24L01P_IO_IRQ_MASK;

#elif (SNRF_CONFIG_NRF905 == 1)

  NRF905_IO_IRQ_DDR &= ~NRF905_IO_IRQ_MASK;
  NRF905_IO_IRQ_PORT &= ~NRF905_IO_IRQ_MASK;
  PCICR |= NRF905_IO_IRQ_PCICR_MASK;
  NRF905_IO_IRQ_PCMSK |= NRF905_IO_IRQ_MASK;

#endif
}

static inline void nrf_disable_rx_irq(void)
{
#if (SNRF_CONFIG_NRF24L01P == 1)

  NRF24L01P_IO_IRQ_PCMSK &= ~NRF24L01P_IO_IRQ_MASK;

#elif (SNRF_CONFIG_NRF905 == 1)

  NRF905_IO_IRQ_PCMSK &= ~NRF905_IO_IRQ_MASK;

#endif
}

static inline void nrf_enable_rx_irq(void)
{
#if (SNRF_CONFIG_NRF24L01P == 1)

  NRF24L01P_IO_IRQ_PCMSK |= NRF24L01P_IO_IRQ_MASK;

#elif (SNRF_CONFIG_NRF905 == 1)

  NRF905_IO_IRQ_PCMSK |= NRF905_IO_IRQ_MASK;

#endif
}

static inline uint8_t nrf_get_rx_irq(void)
{
  /* NOTE: get and acknowledge irq if required */

#if (SNRF_CONFIG_NRF24L01P == 1)

  return nrf24l01p_is_rx_irq();

#elif (SNRF_CONFIG_NRF905 == 1)

  return nrf905_is_dr();

#endif
}

static inline uint8_t nrf_peek_rx_irq(void)
{
  /* NOTE: get but do not acknowledge irq */

#if (SNRF_CONFIG_NRF24L01P == 1)

  return nrf24l01p_is_rx_irq_noread();

#elif (SNRF_CONFIG_NRF905 == 1)

  return nrf905_is_dr();

#endif
}

static inline uint8_t nrf_read_payload(uint8_t** buf)
{
#if (SNRF_CONFIG_NRF24L01P == 1)

  nrf24l01p_read_rx();
  *buf = nrf24l01p_cmd_buf;
  return nrf24l01p_cmd_len;

#elif (SNRF_CONFIG_NRF905 == 1)

  nrf905_read_payload();
  *buf = nrf905_payload_buf;
  return nrf905_payload_width;

#endif
}

static inline void nrf_send_payload(uint8_t* p)
{
 /* NOTE: enter and leave in powerdown mode */

#if (SNRF_CONFIG_NRF24L01P == 1)

  nrf24l01p_powerdown_to_standby();
  nrf24l01p_standby_to_tx();
  nrf24l01p_write_tx_noack_zero(p);
  nrf24l01p_complete_tx_noack_zero();
  while (nrf24l01p_is_tx_irq() == 0) ;
  nrf24l01p_set_powerdown();

#elif (SNRF_CONFIG_NRF905 == 1)

  nrf905_write_payload_zero(p);

#endif
}

static inline void nrf_set_payload_width(uint8_t x)
{
#if (SNRF_CONFIG_NRF24L01P == 1)

  nrf24l01p_set_payload_width(x);

#elif (SNRF_CONFIG_NRF905 == 1)

  nrf905_set_payload_width(x);
  nrf905_commit_config();

#endif
}

static inline uint8_t nrf_get_payload_width(void)
{
#if (SNRF_CONFIG_NRF24L01P == 1)

  return nrf24l01p_read_reg8(NRF24L01P_REG_RX_PW_P0);

#elif (SNRF_CONFIG_NRF905 == 1)

  return nrf905_payload_width;

#endif
}

static inline uint8_t nrf_set_addr_width(uint8_t x)
{
  /* x in bytes */

#if (SNRF_CONFIG_NRF24L01P == 1)

  static const uint8_t map[] =
  {
    NRF24L01P_ADDR_WIDTH_3,
    NRF24L01P_ADDR_WIDTH_4,
    NRF24L01P_ADDR_WIDTH_5
  };

#define ARRAY_COUNT(__a) (sizeof(__a) / sizeof((__a)[0]))
  if (x >= ARRAY_COUNT(map)) return -1;

  /* x in NRF24L01P_ADDR_WIDTH_xxx */
  nrf24l01p_set_addr_width(map[x]);

#elif (SNRF_CONFIG_NRF905 == 1)

  /* x in bytes */
  nrf905_set_rx_afw(x);
  nrf905_set_tx_afw(x);
  nrf905_commit_config();

#endif

  return 0;
}

static inline uint8_t nrf_get_addr_width(void)
{
  /* return the addr width in bytes */

#if (SNRF_CONFIG_NRF24L01P == 1)

  static const uint8_t map[] =
  {
    0xff, /* invalid */
    0, /* width = 3 */
    1, /* width = 4 */
    2 /* width = 5 */
  };

  const uint8_t x = nrf24l01p_read_reg8(NRF24L01P_REG_SETUP_AW);

  return map[x & 3];

#elif (SNRF_CONFIG_NRF905 == 1)

  return nrf905_get_tx_afw();

#endif
}

static inline void nrf_disable_crc(void)
{
#if (SNRF_CONFIG_NRF24L01P == 1)
  nrf24l01p_disable_crc();
#elif (SNRF_CONFIG_NRF905 == 1)
  nrf905_set_crc_en(0);
  nrf905_set_crc_mode(0);
  nrf905_commit_config();
#endif
}

static inline void nrf_enable_crc8(void)
{
#if (SNRF_CONFIG_NRF24L01P == 1)
  nrf24l01p_enable_crc8();
#elif (SNRF_CONFIG_NRF905 == 1)
  nrf905_set_crc_en(1);
  nrf905_set_crc_mode(0);
  nrf905_commit_config();
#endif
}

static inline void nrf_enable_crc16(void)
{
#if (SNRF_CONFIG_NRF24L01P == 1)
  nrf24l01p_enable_crc16();
#elif (SNRF_CONFIG_NRF905 == 1)
  nrf905_set_crc_en(1);
  nrf905_set_crc_mode(1);
  nrf905_commit_config();
#endif
}

static inline void nrf_set_rx_addr(uint8_t* x)
{
#if (SNRF_CONFIG_NRF24L01P == 1)
  nrf24l01p_cmd_buf[0] = x[0];
  nrf24l01p_cmd_buf[1] = x[1];
  nrf24l01p_cmd_buf[2] = x[2];
  nrf24l01p_cmd_buf[3] = x[3];
  nrf24l01p_cmd_buf[4] = 0x00;
  nrf24l01p_write_reg40(NRF24L01P_REG_RX_ADDR_P0);
#elif (SNRF_CONFIG_NRF905 == 1)
  nrf905_set_rx_addr(x, nrf905_get_rx_afw());
  /* already commited */
#endif
}

static inline void nrf_set_tx_addr(uint8_t* x)
{
#if (SNRF_CONFIG_NRF24L01P == 1)
  nrf24l01p_cmd_buf[0] = x[0];
  nrf24l01p_cmd_buf[1] = x[1];
  nrf24l01p_cmd_buf[2] = x[2];
  nrf24l01p_cmd_buf[3] = x[3];
  nrf24l01p_cmd_buf[4] = 0x00;
  nrf24l01p_write_reg40(NRF24L01P_REG_TX_ADDR);
#elif (SNRF_CONFIG_NRF905 == 1)
  nrf905_set_tx_addr(x, nrf905_get_tx_afw());
  /* already commited */
#endif
}

static inline void nrf_disable_tx_ack(void)
{
#if (SNRF_CONFIG_NRF24L01P == 1)
  nrf24l01p_enable_tx_noack();
#elif (SNRF_CONFIG_NRF905 == 1)
  nrf905_set_auto_retran(0);
  nrf905_commit_config();
#endif
}

static inline void nrf_enable_tx_ack(void)
{
#if (SNRF_CONFIG_NRF24L01P == 1)
  nrf24l01p_disable_tx_noack();
#elif (SNRF_CONFIG_NRF905 == 1)
  nrf905_set_auto_retran(1);
  nrf905_commit_config();
#endif
}

static inline uint8_t nrf_get_tx_ack(void)
{
  /* return 0 if disabled, non zero otherwise */

#if (SNRF_CONFIG_NRF24L01P == 1)

  const uint8_t x = nrf24l01p_read_reg8(NRF24L01P_REG_FEATURE);
  if (x & (1 << 0)) return 0;
  else return 1;

#elif (SNRF_CONFIG_NRF905 == 1)

  return nrf905_get_auto_retran();

#endif
}


#if 0 /* unused */

static void set_led(uint8_t mask)
{
#define LED_RX_POS 6
#define LED_MATCH_POS 7
#define LED_RX_MASK (1 << LED_RX_POS)
#define LED_MATCH_MASK (1 << LED_MATCH_POS)

  static uint8_t pre_mask = 0;

  DDRD |= (LED_RX_MASK | LED_MATCH_MASK);

  if (mask == pre_mask) return ;

  PORTD &= ~(LED_RX_MASK | LED_MATCH_MASK);
  PORTD |= mask;

  pre_mask = mask;
}

#endif /* unused */


/* interrupt on change handlers */
/* do nothing, processed by sequential in do_nrf */

ISR(PCINT0_vect) {}
ISR(PCINT1_vect) {}
ISR(PCINT2_vect) {}

static inline void write_payload_msg(const uint8_t* data, uint8_t size)
{
  /* write a payload message */
  /* warning: rely on snrf_msg_t field ordering and packing */
  /* use multiple uart_write to avoid memory copies */

  static const uint8_t op = SNRF_OP_PAYLOAD;
  static const uint8_t sync = 0x00;

  uart_write(&op, sizeof(uint8_t));
  uart_write(data, size);
  uart_write(&size, sizeof(uint8_t));
  uart_write(&sync, sizeof(uint8_t));
}

static uint8_t do_nrf(void)
{
  /* return 0 if no msg processed, 1 otherwise */

  uint8_t width;
  uint8_t* buf;

  if (nrf_get_rx_irq() == 0) return 0;

  width = nrf_read_payload(&buf);

  /* TODO: use an error message */
  if (width > SNRF_MAX_PAYLOAD_WIDTH)
    width = SNRF_MAX_PAYLOAD_WIDTH;

  write_payload_msg(buf, width);

  /* on message handled */
  return 1;
}

/* uart interrupt handler */

static void uart_enable_rx_int(void)
{
  UCSR0B |= 1 << RXCIE0;
}

static inline uint8_t uart_is_rx_empty(void)
{
  return (UCSR0A & (1 << RXC0)) == 0;
}

static volatile uint8_t uart_buf[sizeof(snrf_msg_t)];
static volatile uint8_t uart_pos = 0;

#define UART_FLAG_MISS (1 << 0)
#define UART_FLAG_ERR (1 << 1)
static volatile uint8_t uart_flags = 0;

ISR(USART_RX_vect)
{
  /* warning: uart_pos must only be incremented. this allows */
  /* the sequential part of the code to access its contents */
  /* without disabling uart interrupts. especially, uart_pos */
  /* must not be set to 0 here on error, and doing so is left */
  /* to the sequential */

  uint8_t err;
  uint8_t x;

  while (uart_is_rx_empty() == 0)
  {
    err = uart_read_uint8(&x);

    /* missed byte */
    if (uart_pos == sizeof(snrf_msg_t))
    {
      uart_flags |= UART_FLAG_MISS;
      return ;
    }

    /* uart rx error */
    if (err)
    {
      /* force synchronization */
      uart_flags |= UART_FLAG_ERR;
      uart_pos = sizeof(snrf_msg_t);
      uart_buf[offsetof(snrf_msg_t, sync)] = SNRF_SYNC_BYTE;
      return ;
    }

    uart_buf[uart_pos++] = x;
  }
}


/* snrf global state */

static uint8_t snrf_state;

/* uart message handlers */

#define MAKE_COMPL_ERROR(__m, __e)		\
do {						\
  (__m)->op = SNRF_OP_COMPL;			\
  (__m)->u.compl.err = __e;			\
  (__m)->u.compl.val = __LINE__;		\
} while (0)

static inline uint32_t uint32_to_le(uint32_t x)
{
  return x;
}

static inline uint32_t le_to_uint32(uint32_t x)
{
  return x;
}

static uint32_t buf_to_uint32(const uint8_t* buf)
{
  uint32_t x = 0;

  x |= ((uint32_t)buf[0]) << 24;
  x |= ((uint32_t)buf[1]) << 16;
  x |= ((uint32_t)buf[2]) << 8;
  x |= ((uint32_t)buf[3]) << 0;

  return x;
}

static void handle_set_msg(snrf_msg_t* msg)
{
  /* capture before modifying */
  const uint8_t key = msg->u.set.key;
  const uint32_t val = le_to_uint32(msg->u.set.val);

  if ((snrf_state != SNRF_STATE_CONF) && (key != SNRF_KEY_STATE))
  {
    MAKE_COMPL_ERROR(msg, SNRF_ERR_VAL);
    return ;
  }

  /* completion sent back, assume success */
  MAKE_COMPL_ERROR(msg, SNRF_ERR_SUCCESS);

  switch (key)
  {
  case SNRF_KEY_STATE:
    {
      if (val >= SNRF_STATE_MAX)
      {
	MAKE_COMPL_ERROR(msg, SNRF_ERR_VAL);
	break ;
      }

      if (val == snrf_state)
      {
	/* nothing */
	break ;
      }

      if (val == SNRF_STATE_CONF)
      {
	nrf_set_powerdown_mode();
      }
      else if (val == SNRF_STATE_TXRX)
      {
	nrf_set_rx_mode();
      }

      snrf_state = val;

      break ;
    }

  case SNRF_KEY_CRC:
    if (val == SNRF_CRC_DISABLED) nrf_disable_crc();
    else if (val == SNRF_CRC_8) nrf_enable_crc8();
    else if (val == SNRF_CRC_16) nrf_enable_crc16();
    else MAKE_COMPL_ERROR(msg, SNRF_ERR_VAL);
    break ;

#if (SNRF_CONFIG_NRF24L01P == 1)
  case SNRF_KEY_RATE:
    {
      static const uint8_t map[] =
      {
	NRF24L01P_RATE_250KBPS,
	NRF24L01P_RATE_1MBPS,
	NRF24L01P_RATE_2MBPS
      };
      if (val >= ARRAY_COUNT(map)) MAKE_COMPL_ERROR(msg, SNRF_ERR_VAL);
      else nrf24l01p_set_rate(map[val]);
      break ;
    }
#endif

#if (SNRF_CONFIG_NRF24L01P == 1)
  case SNRF_KEY_CHAN:
    nrf24l01p_set_chan((uint8_t)val);
    break ;
#endif

  case SNRF_KEY_ADDR_WIDTH:
    {
      if (nrf_set_addr_width(val)) MAKE_COMPL_ERROR(msg, SNRF_ERR_VAL);
      break ;
    }

  case SNRF_KEY_RX_ADDR:
    nrf_set_rx_addr((uint8_t*)&msg->u.set.val);
    break ;

  case SNRF_KEY_TX_ADDR:
    nrf_set_tx_addr((uint8_t*)&msg->u.set.val);
    break ;

  case SNRF_KEY_TX_ACK:
    /* disable tx ack */
    if (val == 0) nrf_disable_tx_ack();
    /* enable tx ack */
    else if (val == 1) nrf_enable_tx_ack();
    else MAKE_COMPL_ERROR(msg, SNRF_ERR_VAL);
    break ;

  case SNRF_KEY_PAYLOAD_WIDTH:
    if (val > SNRF_MAX_PAYLOAD_WIDTH) MAKE_COMPL_ERROR(msg, SNRF_ERR_VAL);
    else nrf_set_payload_width((uint8_t)val);
    break ;

  case SNRF_KEY_UART_FLAGS:
    uart_flags = (uint8_t)val;
    break ;

  default:
    MAKE_COMPL_ERROR(msg, SNRF_ERR_KEY);
    break ;
  }
}

static void handle_get_msg(snrf_msg_t* msg)
{
  /* capture before modifying */
  const uint8_t key = msg->u.set.key;

  /* completion sent back, assume success */
  MAKE_COMPL_ERROR(msg, SNRF_ERR_SUCCESS);

  switch (key)
  {
  case SNRF_KEY_STATE:
    msg->u.compl.val = uint32_to_le(snrf_state);
    break ;

#if (SNRF_CONFIG_NRF24L01P == 1)
  case SNRF_KEY_CRC:
    {
      const uint8_t x = nrf24l01p_read_reg8(NRF24L01P_REG_CONFIG);
      if ((x & (1 << 3)) == 0) msg->u.compl.val = uint32_to_le(0);
      else if ((x & (1 << 2)) == 0) msg->u.compl.val = uint32_to_le(1);
      else msg->u.compl.val = uint32_to_le(2);
      break ;
    }
#endif

#if (SNRF_CONFIG_NRF24L01P == 1)
  case SNRF_KEY_RATE:
    {
      const uint8_t x = nrf24l01p_read_reg8(NRF24L01P_REG_RF_SETUP);
      static const uint8_t map[] =
      {
	NRF24L01P_RATE_1MBPS,
	NRF24L01P_RATE_2MBPS,
	NRF24L01P_RATE_250KBPS,
	NRF24L01P_RATE_250KBPS,
	NRF24L01P_RATE_250KBPS
      };
      msg->u.compl.val = uint32_to_le(map[(x >> 3) & 5]);
      break ;
    }
#endif

#if (SNRF_CONFIG_NRF24L01P == 1)
  case SNRF_KEY_CHAN:
    {
      const uint8_t x = nrf24l01p_read_reg8(NRF24L01P_REG_RF_CH);
      msg->u.compl.val = uint32_to_le(x);
      break ;
    }
#endif

  case SNRF_KEY_ADDR_WIDTH:
    {
      msg->u.compl.val = uint32_to_le(nrf_get_addr_width());
      break ;
    }

#if (SNRF_CONFIG_NRF24L01P == 1)
  case SNRF_KEY_RX_ADDR:
    {
      nrf24l01p_read_reg40(NRF24L01P_REG_RX_ADDR_P0);
      msg->u.compl.val = uint32_to_le(buf_to_uint32(nrf24l01p_cmd_buf));
      break ;
    }
#endif

#if (SNRF_CONFIG_NRF24L01P == 1)
  case SNRF_KEY_TX_ADDR:
    {
      nrf24l01p_read_reg40(NRF24L01P_REG_TX_ADDR);
      msg->u.compl.val = uint32_to_le(buf_to_uint32(nrf24l01p_cmd_buf));
      break ;
    }
#endif

  case SNRF_KEY_TX_ACK:
    msg->u.compl.val = uint32_to_le(nrf_get_tx_ack());
    break ;

  case SNRF_KEY_PAYLOAD_WIDTH:
    {
      const uint8_t x = nrf_get_payload_width();
      msg->u.compl.val = uint32_to_le(x);
      break ;
    }

  case SNRF_KEY_UART_FLAGS:
    {
      msg->u.compl.val = uint32_to_le((uint32_t)uart_flags);
      break ;
    }

  default:
    MAKE_COMPL_ERROR(msg, SNRF_ERR_KEY);
    break ;
  }
}

static void handle_payload_msg(snrf_msg_t* msg)
{
  /* send the message */
  /* msg size and payload width assumed equal */

  if (snrf_state != SNRF_STATE_TXRX)
  {
    MAKE_COMPL_ERROR(msg, SNRF_ERR_STATE);
    return ;
  }

#if (SNRF_CONFIG_NRF24L01P == 1)

  /* assumed was in rx mode */
  nrf24l01p_rx_to_tx();

  nrf24l01p_write_tx_noack_zero(msg->u.payload.data);
  nrf24l01p_complete_tx_noack_zero();
  while (nrf24l01p_is_tx_irq() == 0) ;

  nrf24l01p_tx_to_rx();

#elif (SNRF_CONFIG_NRF905 == 1)

  nrf905_write_payload_zero(msg->u.payload.data);

#endif

  /* fill completion status */
  MAKE_COMPL_ERROR(msg, SNRF_ERR_SUCCESS);
}

static void handle_msg(snrf_msg_t* msg)
{
  switch (msg->op)
  {
  case SNRF_OP_SET:
    handle_set_msg(msg);
    break ;

  case SNRF_OP_GET:
    handle_get_msg(msg);
    break ;

  case SNRF_OP_PAYLOAD:
    handle_payload_msg(msg);
    break ;

  default:
    MAKE_COMPL_ERROR(msg, SNRF_ERR_OP);
    break ;
  }
}

static uint8_t do_uart(void)
{
  /* return 0 if no msg processed, 1 otherwise */

  /* note: commands comming from the host always */
  /* require a form of ack, which implies that */
  /* there wont be a uart rx burst from the dev */
  /* point of view. thus, this routine can be */
  /* slow to handle message without missing */
  /* bytes, until the reply is sent. */

  /* no need to disable interrupts. cf USART_RX_vect comment. */
  const uint8_t pos = uart_pos;

  uint8_t x;

  if (pos != sizeof(snrf_msg_t))
  {
    /* not a full message available */
    return 0;
  }

  /* synchronization procedure */
  if (uart_buf[offsetof(snrf_msg_t, sync)] == SNRF_SYNC_BYTE)
  {
    /* disable interrupts before setting uart_pos */
    /* with interrupts disabled, wait for SYNC_END */
    cli();
    uart_pos = 0;
    while (1)
    {
      /* do not stop on error during sync */
      if (uart_read_uint8(&x)) continue ;
      if (x == SNRF_SYNC_END) break ;
    }
    sei();

    /* set state to conf */
    if (snrf_state != SNRF_STATE_CONF)
    {
      snrf_state = SNRF_STATE_CONF;
      nrf_set_powerdown_mode();
    }

    return 1;
  }

  /* handle new message */
  handle_msg((snrf_msg_t*)uart_buf);

  /* warning: set uart_pos to 0 before sending */
  /* the reply, since the host will start sending */
  /* new packets and we do not want the interrupt */
  /* handler to see uart_buf full */
  uart_pos = 0;

  /* send completion */
  uart_write((uint8_t*)uart_buf, sizeof(snrf_msg_t));

  /* a message has been handled */
  return 1;
}

/* main */

int main(void)
{
  /* default state to conf */
  snrf_state = SNRF_STATE_CONF;

  nrf_setup();
  nrf_set_powerdown_mode();
  nrf_setup_rx_irq();

  uart_setup();
  uart_enable_rx_int();

  /* uart and pinchange int wakeup sources */
  set_sleep_mode(SLEEP_MODE_IDLE);

  /* enable interrupts before looping */
  sei();

  while (1)
  {
    sleep_disable();

    /* note: keep uart interrupt enabled */

    /* disable pcint interrupt since we are already awake */
    /* and entering the handler perturbates the execution */
    nrf_disable_rx_irq();

    /* alternate do_{uart,nrf} to avoid starvation */
    while (1)
    {
      uint8_t is_msg = do_uart();
      if (snrf_state != SNRF_STATE_CONF) is_msg |= do_nrf();
      if (is_msg == 0) break ;
    }

    /* reenable pcint interrupts */
    nrf_enable_rx_irq();

    /* the following procedure is used to not miss interrupts */
    /* disable interrupts, check if something available */
    /* otherwise, enable interrupt and sleep (sei, sleep) */
    /* the later ensures now interrupt is missed */

    sleep_enable();
    sleep_bod_disable();

    cli();

    if ((uart_pos == sizeof(snrf_msg_t)) || nrf_peek_rx_irq())
    {
      /* continue, do not sleep */
      sei();
    }
    else
    {
      /* warning: keep the 2 instructions in the same block */
      /* atomic, no int schedule between sei and sleep_cpu */
      sei();
      sleep_cpu();
    }
  }

  return 0;
}
