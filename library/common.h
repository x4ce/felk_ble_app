#include <zephyr/kernel.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/sys/printk.h>

extern uint8_t rx_buf[];

void uart_init(void);
int uart_tx_byte (uint8_t data);
