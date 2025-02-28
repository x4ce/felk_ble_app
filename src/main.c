#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/sys/printk.h>

#include "common.h"
#include "remote.h"
#include "pwm_f.h"
#include "adc_f.h"

#define         STACKSIZE               2048
#define         ADC_THREAD_PRIORITY     6
#define         ADC_SLEEP_TIME          500

#define         SOL1_NODE               DT_ALIAS(sol1)
#define         SOL1EXT_NODE            DT_ALIAS(sol1ext)
#define         SOL2_NODE               DT_ALIAS(sol2)
#define         SOL2EXT_NODE            DT_ALIAS(sol2ext)
#define         BLDC_NODE               DT_ALIAS(bldc)

#define         VBAT_THRESHOLD          10000
#define         CR1_THRESHOLD           300
#define         CR2_THRESHOLD           300

uint16_t adc_data[3] = {0};

static void timer0_handler(struct k_timer *timer_id)
{
        printk("Timer 0 fired!\r\n");
        // Acquire ADC data
        adc_data[0] = read_adc(6);      // Battery
        adc_data[1] = read_adc(4);      // CR1
        adc_data[2] = read_adc(5);      // CR2
        printk("ADC 0: %d, ADC 4: %d, ADC 5: %d\r\n", adc_data[0], adc_data[1], adc_data[2]);

        if (adc_data[0] > VBAT_THRESHOLD)
        {
                printk("Battery Power detected!\r\n");
        }
        else
        {
                printk("Battery Power not detected!\r\n");
        }

        if (adc_data[1] > CR1_THRESHOLD)
        {
                printk("CR1 Power detected!\r\n");
        }
        else
        {
                printk("CR1 Power not detected!\r\n");
        }

        if (adc_data[2] > CR2_THRESHOLD)
        {
                printk("CR2 Power detected!\r\n");
        }
        else
        {
                printk("CR2 Power not detected!\r\n");
        }

}

static K_TIMER_DEFINE(timer0, timer0_handler, NULL);

static const struct gpio_dt_spec sol1 = GPIO_DT_SPEC_GET(SOL1_NODE, gpios);
static const struct gpio_dt_spec sol1ext = GPIO_DT_SPEC_GET(SOL1EXT_NODE, gpios);
static const struct gpio_dt_spec sol2 = GPIO_DT_SPEC_GET(SOL2_NODE, gpios);
static const struct gpio_dt_spec sol2ext = GPIO_DT_SPEC_GET(SOL2EXT_NODE, gpios);
static const struct gpio_dt_spec bldc = GPIO_DT_SPEC_GET(BLDC_NODE, gpios);

static void app_cmd_cb(uint8_t cmd)
{
        printk("BLE Command Received: %d \n", cmd);
	uint8_t tmp;
        tmp = cmd & 0x80;
        if (tmp == 0)
        {
                tmp = cmd & 0x10;
                if (tmp == 0x10)
                {
                        gpio_pin_set_dt(&bldc, 1);
                }
                else
                {
                        gpio_pin_set_dt(&bldc, 0);
                }
                tmp = cmd & 0x08;
                if (tmp == 0x08)
                {
                        gpio_pin_set_dt(&sol1, 1);
                }
                else
                {
                        gpio_pin_set_dt(&sol1, 0);
                }

                tmp = cmd & 0x04;
                if (tmp == 0x04)
                {
                        gpio_pin_set_dt(&sol1ext, 1);
                }       
                else
                {
                        gpio_pin_set_dt(&sol1ext, 0);
                }

                tmp = cmd & 0x02;
                if (tmp == 0x02)
                {
                        gpio_pin_set_dt(&sol2, 1);
                }
                else
                {
                        gpio_pin_set_dt(&sol2, 0);
                }

                tmp = cmd & 0x01;
                if (tmp == 0x01)
                {
                        gpio_pin_set_dt(&sol2ext, 1);
                }
                else
                {
                        gpio_pin_set_dt(&sol2ext, 0);
                }

                if (cmd == 0)
                {
                        gpio_pin_set_dt(&bldc, 0);
                        gpio_pin_set_dt(&sol1, 0);
                        gpio_pin_set_dt(&sol1ext, 0);
                        gpio_pin_set_dt(&sol2, 0);
                        gpio_pin_set_dt(&sol2ext, 0);
                }
        } else if (tmp == 0x80)
        {
                tmp = cmd & 0x7F;
                pwm_set_dc(1, tmp);
        } else {
                printk("Error: Invalid command!\r\n");
        }
   

}

static uint16_t app_data_cb(void)
{
        adc_data[0] = read_adc(6);
        adc_data[1] = read_adc(4);
        adc_data[2] = read_adc(5);
        return 0xff;
}

static struct felk_ble_cb app_callbacks = {
	.cmd_cb = app_cmd_cb,
	.data_cb = app_data_cb,
};

int main(void)
{
        int ret;

        ret = pwms_init();

        ret = bluetooth_init();
	if (ret)
	{
                printk("BLE Service failed to initialize!\r\n");
		return 7;
	}

        if (ret)
        {
                printk("Error: PWM initialization failed!\r\n");
        }

        if (!device_is_ready(sol1.port))
        {
                printk("Error: %s device is not ready!\r\n", sol1.port->name);
                return 1;
        }

        ret = gpio_pin_configure_dt(&sol1, GPIO_OUTPUT_INACTIVE);
	if (ret)
	{
                printk("Error: %s %d configuration failed!\r\n", sol1.port->name, sol1.pin);
		return 2;
	}

        ret = gpio_pin_configure_dt(&sol1ext, GPIO_OUTPUT_INACTIVE);
	if (ret)
	{
                printk("Error: %s %d configuration failed!\r\n", sol1ext.port->name, sol1ext.pin);
		return 3;
	}

        ret = gpio_pin_configure_dt(&sol2, GPIO_OUTPUT_INACTIVE);
	if (ret)
	{
                printk("Error: %s %d configuration failed!\r\n", sol2.port->name, sol2.pin);
		return 4;
	}

        ret = gpio_pin_configure_dt(&sol2ext, GPIO_OUTPUT_INACTIVE);
	if (ret)
	{
                printk("Error: %s %d configuration failed!\r\n", sol2ext.port->name, sol2ext.pin);
		return 5;
	}

        ret = gpio_pin_configure_dt(&bldc, GPIO_OUTPUT_INACTIVE);
	if (ret)
	{
                printk("Error: %s %d configuration failed!\r\n", bldc.port->name, bldc.pin);
		return 6;
	}

        ret = felk_ble_init(&app_callbacks);

        printk("BLE Service successfully initialized!\r\n");

        adc_init();
        
        // Start timer 0 once every 0.5 seconds
        k_timer_start(&timer0, K_MSEC(500), K_MSEC(500));

        return 0;
}

static void adc_thread_func(void *unused1, void *unused2, void *unused3)
{
        ARG_UNUSED(unused1);
        ARG_UNUSED(unused2);
        ARG_UNUSED(unused3);

        while (1)
        {
                printk("ADC Thread fired!\r\n");
                // Acquire ADC data
                adc_data[0] = read_adc(6);      // Battery
                adc_data[1] = read_adc(4);      // CR1
                adc_data[2] = read_adc(5);      // CR2
                printk("ADC 6: %d, ADC 4: %d, ADC 5: %d\r\n", adc_data[0], adc_data[1], adc_data[2]);
                k_sleep(K_MSEC(ADC_SLEEP_TIME));
        }
}

K_THREAD_DEFINE(adc_thread_id, STACKSIZE, adc_thread_func, NULL, NULL, NULL, ADC_THREAD_PRIORITY, 0, 0);