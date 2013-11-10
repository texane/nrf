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

#define SNRF_STATE_CONF 0
#define SNRF_STATE_TXRX 1
#define SNRF_STATE_MAX 2

#define SNRF_ERR_SUCCESS 0
#define SNRF_ERR_FAILURE 1
#define SNRF_ERR_OP 2
#define SNRF_ERR_KEY 3
#define SNRF_ERR_VAL 4 
#define SNRF_ERR_STATE 5

typedef struct
{
  uint8_t op;

  union
  {
    struct
    {
#define SNRF_MAX_PAYLOAD_WIDTH 16
      uint8_t data[SNRF_MAX_PAYLOAD_WIDTH];
      uint8_t size;
    } payload;

    struct
    {
      uint8_t key;
      uint32_t val;
    } set;

    struct
    {
      uint8_t key;
    } get;

    struct
    {
      uint8_t err;
      uint32_t val;
    } compl;

    struct
    {
      uint32_t data;
      uint32_t line;
    } debug;

  } u;

} __attribute__((packed)) snrf_msg_t;


#endif /* SNRF_COMMON_H_INCLUDED */
