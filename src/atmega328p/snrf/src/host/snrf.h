#ifndef SNRF_H_INCLUDED
#define SNRF_H_INCLUDED


#include <stdint.h>
#include <sys/types.h>
#include "serial.h"
#include "snrf_common.h"

typedef struct snrf_msg_node
{
  snrf_msg_t msg;
  struct snrf_msg_node* next;
} snrf_msg_node_t;

typedef struct snrf_handle
{
  serial_handle_t serial;

  /* list of pending received messages */
  snrf_msg_node_t* msg_head;
  snrf_msg_node_t* msg_tail;

  /* snrf_state_xxx */
  uint32_t state;

  /* message rx buffer */
  uint8_t msg_rbuf[sizeof(snrf_msg_t)];
  size_t msg_rpos;

} snrf_handle_t;

int snrf_open(snrf_handle_t*);
int snrf_close(snrf_handle_t*);
int snrf_sync(snrf_handle_t*);
int snrf_write_payload(snrf_handle_t*, const uint8_t*, size_t);
int snrf_read_payload(snrf_handle_t*, uint8_t*, size_t*);
int snrf_set_keyval(snrf_handle_t*, uint8_t, uint32_t);
int snrf_get_keyval(snrf_handle_t*, uint8_t, uint32_t*);
int snrf_get_pending_msg(snrf_handle_t*, snrf_msg_t*);
int snrf_read_msg(snrf_handle_t*, snrf_msg_t*);

static inline int snrf_get_fd(snrf_handle_t* snrf)
{
  return snrf->serial.fd;
}


#endif /* SNRF_H_INCLUDED */
