#include <zephyr/kernel.h>
#include "mtr_api.h"
#include "common.h"

uint8_t mtr_status;

void uart_rx_work_handler(struct k_work *work) {

uint8_t tmp = rx_buf[1];

	switch(tmp)
	{
	case 11:							// ping
		mtr_status = (rx_buf[0] == 1) ? (mtr_status | 0x01) : (mtr_status & 0xFE);
		break;
	case 12:		// motor start
        mtr_status = (rx_buf[0] == 1) ? (mtr_status | 0x02) : (mtr_status & 0xFD);
		break;
	case 13:		// motor stop
        mtr_status = (rx_buf[0] == 1) ? (mtr_status | 0x04) : (mtr_status & 0xFB);
		break;
	default:
		__NOP();
		break;
	}

}

K_WORK_DEFINE(uart_rx_work_item, uart_rx_work_handler);

int err;

int bldc_ping(void)
{
	int tmp = 11;
    err = uart_tx_byte(tmp);
	if (err)
	{
		printk("Error: BLDC Ping UART TX Failed!\r\n");
	}
	return err;
}

int bldc_start(void)
{
	int tmp = 12;
    err = uart_tx_byte(tmp);
	if (err)
	{
		printk("Error: BLDC Start UART TX Failed!\r\n");
	}
	return err;
}

int bldc_stop(void)
{
	int tmp = 13;
    err = uart_tx_byte(tmp);
	if (err)
	{
		printk("Error: BLDC Stop UART TX Failed!\r\n");
	}
	return err;
}

int bldc_get_RPM(void)
{
	int tmp = 14;
    err = uart_tx_byte(tmp);
	if (err)
	{
		printk("Error: BLDC RPM UART TX Failed!\r\n");
	}
	return err;
}

