#include <zephyr/kernel.h>
#include "common.h"
#include "mtr_api.h"

void uart_init(void);

#define			RECEIVE_TIMEOUT			100
#define			RECEIVE_BUFF_SIZE		4

const struct device *uart = DEVICE_DT_GET(DT_NODELABEL(uart0));

static uint8_t tx_buf[] = {"UART Service initialized!\n\r"};
uint8_t rx_buf[RECEIVE_BUFF_SIZE] = {0};

static void uart_cb(const struct device *dev, struct uart_event *evt, void *user_data)
{
	switch (evt->type)
	{
		case UART_RX_RDY:
			if ((evt->data.rx.len) == 4)
			{
                k_work_submit(&uart_rx_work_item);              
			}
			break;
		case UART_RX_DISABLED:
			uart_rx_enable(dev, rx_buf, sizeof(rx_buf), RECEIVE_TIMEOUT);
			break;

		default:
			break;
	}
}

void uart_init(void)
{
    int err;
	
	if (!device_is_ready(uart))
	{
		printk("Error: %s device is not ready!\r\n", uart->name);
		
	}

	err = uart_callback_set(uart, uart_cb, NULL);
	if (err)
	{
		printk("Error: %s callback not set!\r\n", uart->name);
		
	}

	// err = uart_tx(uart, tx_buf, sizeof(tx_buf),SYS_FOREVER_US);
	// if (err)
	// {
	// 	printk("Error: %s transmission failed!\r\n", uart->name);
		
	// }

	err = uart_rx_enable(uart, rx_buf, sizeof(rx_buf), RECEIVE_TIMEOUT);
	if (err)
	{
		printk("Error: %s reception enabling failed!\r\n", uart->name);
		
	}

}

int uart_tx_byte (uint8_t data)
{
	int err;
	err = uart_tx(uart, &data, sizeof(data),SYS_FOREVER_US);
	if (err)
	{
		printk("Error: %s transmission failed!\r\n", uart->name);
		
	}

	return err;
}
