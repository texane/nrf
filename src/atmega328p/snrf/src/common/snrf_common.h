#ifndef SNRF_COMMON_H_INCLUDED
#define SNRF_COMMON_H_INCLUDED


#define SNRF_OP_SET 0
#define SNRF_OP_GET 1
#define SNRF_OP_PAYLOAD 2
#define SNRF_OP_COMPL 3

#define SNRF_KEY_STATE 0
#define SNRF_KEY_CRC 1
#define SNRF_KEY_RATE 2
#define SNRF_KEY_CHAN 3
#define SNRF_KEY_ADDR_WIDTH 4
#define SNRF_KEY_RX_ADDR 5
#define SNRF_KEY_TX_ADDR 6
#define SNRF_KEY_ACK 7
#define SNRF_KEY_PAYLOAD_WIDTH 8

#define SNRF_STATE_IDLE 0
#define SNRF_STATE_TXRX 1

typedef struct
{
  uint8_t op;
  uint8_t id;

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
  } u;

} __attribute__((packed)) snrf_msg_t;


#endif /* SNRF_COMMON_H_INCLUDED */
