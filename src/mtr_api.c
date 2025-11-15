#include <zephyr/kernel.h>
#include "mtr_api.h"
#include "common.h"

uint8_t mtr_ble_status[4] = {0};
uint8_t mtr_uart_status;

void uart_rx_work_handler(struct k_work *work) {

uint8_t tmp = rx_buf[2];

	switch(tmp)
	{
	case 11:		// Motor Start
		mtr_uart_status = (rx_buf[0] == 1) ? (mtr_uart_status | 0x01) : (mtr_uart_status & 0xFE);
		break;
	case 12:		// Motor Stop
        mtr_uart_status = (rx_buf[0] == 1) ? (mtr_uart_status | 0x02) : (mtr_uart_status & 0xFD);
		break;
	case 13:		// Fault Ack
        mtr_uart_status = (rx_buf[0] == 1) ? (mtr_uart_status | 0x04) : (mtr_uart_status & 0xFB);
		break;
	case 14:		// Ping
        mtr_ble_status[3] = (rx_buf[0] == 1) ? (mtr_ble_status[3] | 0x80) : (mtr_uart_status & 0x7F);
		break;
	case 15:		// Motor Run Status
		mtr_ble_status[1] = (mtr_ble_status[1] & 0x07) | ((rx_buf[0] & 0x1F) << 3);
		break;
	case 16:		// Fault type
		mtr_ble_status[0] = rx_buf[0];
		mtr_ble_status[1] = (mtr_ble_status[1] & 0xF8) | rx_buf[1];
		break;
	case 17:		// Motor Speed (RPM)
        mtr_ble_status[2] = rx_buf[0];
		mtr_ble_status[3] = (mtr_ble_status[3] & 0xC0) | rx_buf[1];
		break;
	default:
		__NOP();
		break;
	}

}

K_WORK_DEFINE(uart_rx_work_item, uart_rx_work_handler);

int err;

int bldc_start(void)
{
	int tmp = 11;
    err = uart_tx_byte(tmp);
	if (err)
	{
		printk("Error: BLDC Start UART TX Failed!\r\n");
	}
	return err;
}

int bldc_stop(void)
{
	int tmp = 12;
    err = uart_tx_byte(tmp);
	if (err)
	{
		printk("Error: BLDC Stop UART TX Failed!\r\n");
	}
	return err;
}

int bldc_send_ack(void)
{
	int tmp = 13;
    err = uart_tx_byte(tmp);
	if (err)
	{
		printk("Error: BLDC ACK UART TX Failed!\r\n");
	}
	return err;
}

int bldc_ping(void)
{
	int tmp = 14;
    err = uart_tx_byte(tmp);
	if (err)
	{
		printk("Error: BLDC Ping UART TX Failed!\r\n");
	}
	return err;
}

int bldc_get_run_stat(void)
{
	int tmp = 15;
    err = uart_tx_byte(tmp);
	if (err)
	{
		printk("Error: BLDC Run UART TX Failed!\r\n");
	}
	return err;
}

int bldc_get_fault(void)
{
	int tmp = 16;
    err = uart_tx_byte(tmp);
	if (err)
	{
		printk("Error: BLDC Fault UART TX Failed!\r\n");
	}
	return err;
}

int bldc_get_RPM(void)
{
	int tmp = 17;
    err = uart_tx_byte(tmp);
	if (err)
	{
		printk("Error: BLDC RPM UART TX Failed!\r\n");
	}
	return err;
}

int bldc_process_status(uint8_t num)
{
	int err;
	uint8_t tmp;
	switch (tmp)
	{
		case 14:
			tmp = num;	// Ping
			break;
		case 15:
			tmp = num;	// Run Stat
			break;
		case 16:
			tmp = num;	// Fault type
			break;
		case 17:
			tmp = num;	// Speed (RPM)
			break;
		default:
			__NOP();
			break;
	}

	err = uart_tx_byte(tmp);
	if (err)
	{
		printk("Error: BLDC Status TX Failed!\r\n");
	}
	return err;

}