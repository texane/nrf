/* simple test for nrf24l01p library */


#include <stdint.h>
#include <avr/io.h>
#include "../common/nrf24l01p.c"
#include "../common/uart.c"


int main(void)
{
  uart_setup();

  nrf24l01p_setup();

  /* sparkfun usb serial board configuration */
  /* NOTE: nrf24l01p_enable_crc8(); for nrf24l01p board */
  nrf24l01p_enable_crc16();
  /* auto ack disabled */
  /* auto retransmit disabled */
  /* 4 bytes payload */
  /* 1mbps, 0dbm */
  nrf24l01p_set_rate(NRF24L01P_RATE_1MBPS);
  /* channel 2 */
  nrf24l01p_set_chan(2);
  /* 5 bytes addr width */
  nrf24l01p_set_addr_width(NRF24L01P_ADDR_WIDTH_5);
  /* rx address */
  nrf24l01p_cmd_buf[0] = 0xe7;
  nrf24l01p_cmd_buf[1] = 0xe7;
  nrf24l01p_cmd_buf[2] = 0xe7;
  nrf24l01p_cmd_buf[3] = 0xe7;
  nrf24l01p_cmd_buf[4] = 0xe7;
  nrf24l01p_write_reg40(NRF24L01P_REG_RX_ADDR_P0);
  /* tx address */
  nrf24l01p_cmd_buf[0] = 0xe7;
  nrf24l01p_cmd_buf[1] = 0xe7;
  nrf24l01p_cmd_buf[2] = 0xe7;
  nrf24l01p_cmd_buf[3] = 0xe7;
  nrf24l01p_cmd_buf[4] = 0xe7;
  nrf24l01p_write_reg40(NRF24L01P_REG_TX_ADDR);
  /* enable tx no ack command */
  nrf24l01p_enable_tx_noack();

  nrf24l01p_powerdown_to_standby();

  uart_write((uint8_t*)"starting\r\n", 10);

#if 1
  nrf24l01p_standby_to_tx();

  if (nrf24l01p_is_tx_empty() == 0) nrf24l01p_flush_tx();
  nrf24l01p_cmd_buf[0] = '0';
  nrf24l01p_cmd_buf[1] = '1';
  nrf24l01p_cmd_buf[2] = '2';
  nrf24l01p_cmd_buf[3] = '3';
  nrf24l01p_write_tx_noack();
  while (nrf24l01p_is_tx_irq() == 0) ;

  uart_write((uint8_t*)"first\r\n", 7);

  if (nrf24l01p_is_tx_empty() == 0) nrf24l01p_flush_tx();
  nrf24l01p_cmd_buf[0] = 'a';
  nrf24l01p_cmd_buf[1] = 'b';
  nrf24l01p_cmd_buf[2] = 'c';
  nrf24l01p_cmd_buf[3] = 'd';
  nrf24l01p_write_tx_noack();
  while (nrf24l01p_is_tx_irq() == 0) ;

  uart_write((uint8_t*)"second\r\n", 8);

  /* still in standby mode */
#endif

 redo_receive:

  nrf24l01p_standby_to_rx();

  if (nrf24l01p_is_rx_full()) nrf24l01p_flush_rx();

  while (nrf24l01p_is_rx_irq() == 0) ;

  nrf24l01p_read_rx();

  if (nrf24l01p_cmd_len == 0)
  {
    uart_write((uint8_t*)"ko", 2);
  }
  else
  {
    uint8_t saved_buf[4];
    uint8_t i;

    uart_write((uint8_t*)nrf24l01p_cmd_buf, nrf24l01p_cmd_len);

    /* save before new commands */
    for (i = 0; i < nrf24l01p_cmd_len; ++i)
      saved_buf[i] = nrf24l01p_cmd_buf[i];

    /* result in setting standby mode */
    nrf24l01p_rx_to_tx();

    if (nrf24l01p_is_tx_empty() == 0)
    {
      uart_write((uint8_t*)"\r\nNE", 4);
      nrf24l01p_flush_tx();
    }

    /* restore saved buf */
    nrf24l01p_cmd_len = i;
    for (i = 0; i < nrf24l01p_cmd_len; ++i)
      nrf24l01p_cmd_buf[i] = saved_buf[i];

    /* push into the tx fifo, then pulse ce to send */
    nrf24l01p_write_tx_noack();
    while (nrf24l01p_is_tx_irq() == 0) ;
  }
  uart_write((uint8_t*)"\r\n", 2);

  goto redo_receive;

  return 0;
}
