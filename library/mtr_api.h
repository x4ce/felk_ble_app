#include <inttypes.h>
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>
#include <zephyr/drivers/uart.h>

extern struct k_work uart_rx_work_item;
void uart_rx_work_handler(struct k_work *work);

int bldc_ping(void);
int bldc_start(void);
int bldc_stop(void);
int bldc_get_RPM(void);