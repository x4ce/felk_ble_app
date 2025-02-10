#include "common.h"

void uart_init(void);

#define			RECEIVE_TIMEOUT			100
#define			RECEIVE_BUFF_SIZE		100

const struct device *uart = DEVICE_DT_GET(DT_NODELABEL(uart0));

static uint8_t tx_buf[] = {"UART Service initialized!\n\r"};
static uint8_t rx_buf[RECEIVE_BUFF_SIZE] = {0};

static void uart_cb(const struct device *dev, struct uart_event *evt, void *user_data)
{
	switch (evt->type)
	{
		case UART_RX_RDY:
			if ((evt->data.rx.len) == 1)
			{
                                
				// if (evt->data.rx.buf[evt->data.rx.offset] == '1')
				// 	gpio_pin_toggle_dt(&led0);
				// else if (evt->data.rx.buf[evt->data.rx.offset] == '2')
				// 	gpio_pin_toggle_dt(&led1);
				// else if (evt->data.rx.buf[evt->data.rx.offset] == '3')
				// 	gpio_pin_toggle_dt(&led2);
				// else if (evt->data.rx.buf[evt->data.rx.offset] == '4')
				// 	gpio_pin_toggle_dt(&led3);
                                
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

	err = uart_tx(uart, tx_buf, sizeof(tx_buf),SYS_FOREVER_US);
	if (err)
	{
		printk("Error: %s transmission failed!\r\n", uart->name);
		
	}

	err = uart_rx_enable(uart, rx_buf, sizeof(rx_buf), RECEIVE_TIMEOUT);
	if (err)
	{
		printk("Error: %s reception enabling failed!\r\n", uart->name);
		
	}

}
