#ifndef SNRF_COMMON_H_INCLUDED
#define SNRF_COMMON_H_INCLUDED


#define SNRF_OP_SET 0
#define SNRF_OP_GET 1
#define SNRF_OP_PAYLOAD 2
#define SNRF_OP_COMPL 3
#define SNRF_OP_DEBUG 4

#define SNRF_KEY_INFO 0
#define SNRF_KEY_STATE 1
#define SNRF_KEY_CRC 2
#define SNRF_KEY_RATE 3
#define SNRF_KEY_CHAN 4
#define SNRF_KEY_ADDR_WIDTH 5
#define SNRF_KEY_RX_ADDR 6
#define SNRF_KEY_TX_ADDR 7
#define SNRF_KEY_TX_ACK 8
#define SNRF_KEY_PAYLOAD_WIDTH 9
#define SNRF_KEY_UART_FLAGS 10

#define SNRF_STATE_CONF 0
#define SNRF_STATE_TXRX 1
#define SNRF_STATE_MAX 2

#define SNRF_CRC_DISABLED 0
#define SNRF_CRC_8 1
#define SNRF_CRC_16 2

#define SNRF_RATE_250KBPS 0
#define SNRF_RATE_1MBPS 1
#define SNRF_RATE_2MBPS 2

#define SNRF_ADDR_WIDTH_3 0
#define SNRF_ADDR_WIDTH_4 1

#define SNRF_TX_ACK_DISABLED 0
#define SNRF_TX_ACK_ENABLED 1

#define SNRF_ERR_SUCCESS 0
#define SNRF_ERR_FAILURE 1
#define SNRF_ERR_OP 2
#define SNRF_ERR_KEY 3
#define SNRF_ERR_VAL 4 
#define SNRF_ERR_STATE 5

#define SNRF_SYNC_BYTE 0xff
#define SNRF_SYNC_END 0x2a

typedef struct
{
  /* warning: everything must be packed attribtued */
  /* or gcc 4.7.2 sizeof(snrf_msg_t) == 21 */

  uint8_t op;

  union
  {
    struct
    {
#define SNRF_MAX_PAYLOAD_WIDTH 16
      uint8_t data[SNRF_MAX_PAYLOAD_WIDTH];
      uint8_t size;
    } __attribute__((packed)) payload;

    struct
    {
      uint8_t key;
      uint32_t val;
    } __attribute__((packed)) set;

    struct
    {
      uint8_t key;
    } __attribute__((packed)) get;

    struct
    {
      uint8_t err;
      uint32_t val;
    } __attribute__((packed)) compl;

    struct
    {
      uint32_t data;
      uint32_t line;
    } __attribute__((packed)) debug;

  } __attribute__((packed)) u;

  /* 0xff if synchronization wanted */
  uint8_t sync;

} __attribute__((packed)) snrf_msg_t;


#endif /* SNRF_COMMON_H_INCLUDED */
