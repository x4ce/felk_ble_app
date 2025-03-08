#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/sys/printk.h>
#include <hal/nrf_gpio.h>

#include "common.h"
#include "remote.h"
#include "pwm_f.h"
#include "adc_f.h"

#define         STACKSIZE               2048
#define         EXE_THREAD_PRIORITY     6
#define         TRIG_THREAD_PRIORITY    7
#define         EXE_SLEEP_TIME          500

#define         SOL1_NODE               DT_ALIAS(sol1)
#define         SOL1EXT_NODE            DT_ALIAS(sol1ext)
#define         SOL2_NODE               DT_ALIAS(sol2)
#define         SOL2EXT_NODE            DT_ALIAS(sol2ext)
#define         BLDC_NODE               DT_ALIAS(bldc)

#define         VBAT_THRESHOLD          10000
#define         CR1_THRESHOLD_L         300
#define         CR1_THRESHOLD_H         14000
#define         CR2_THRESHOLD_L         300
#define         CR2_THRESHOLD_H         14000

extern uint8_t pwm_ch1_dc;
uint16_t adc_data[3] = {0};
bool auto_mode = true;


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
                tmp = cmd & 0x20;
                if (tmp == 0x20)
                {
                        auto_mode = false;
                }
                else
                {
                        auto_mode = true;
                }
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
                        gpio_pin_set_dt(&bldc, 0);
                        auto_mode = true;
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

static uint16_t app_status_cb(void)
{
        uint8_t tmpL = 0;
        uint8_t tmpH = 0;
        if (auto_mode)
        {
                tmpL |= 0x20;
        }
        if (nrf_gpio_pin_out_read(bldc.pin))
        {
                tmpL |= 0x10;
        }

        if (nrf_gpio_pin_out_read(sol1.pin))
        {
                tmpL |= 0x08;
        }

        if (nrf_gpio_pin_out_read(sol1ext.pin))
        {
                tmpL |= 0x04;
        }
        
        if (nrf_gpio_pin_out_read(sol2.pin))
        {
                tmpL |= 0x02;
        }

        if (nrf_gpio_pin_out_read(sol2ext.pin))
        {
                tmpL |= 0x01;
        }

        if (pwm_ch1_dc > 0 && pwm_ch1_dc <= 100)
        {
                tmpH |= 0x80;
                tmpH |= ((pwm_ch1_dc) & 0x7F);
        }


        return (tmpH << 8) | tmpL;
}

static struct felk_ble_cb app_callbacks = {
	.cmd_cb = app_cmd_cb,
	.data_cb = app_data_cb,
        .status_cb = app_status_cb,
};

int main(void)
{
        int ret;

        ret = pwms_init();
        // Set initial PWM duty cycle to 70%
        //pwm_set_dc(1, 70);

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
        // k_timer_start(&timer0, K_MSEC(500), K_MSEC(500));

        return 0;
}

static void exe_thread_func(void *unused1, void *unused2, void *unused3)
{
        ARG_UNUSED(unused1);
        ARG_UNUSED(unused2);
        ARG_UNUSED(unused3);

        int ret;
        bool vbat_state = false;
        bool cr1_state = false;
        bool cr2_state = false;

        while (1)
        {
                printk("ADC Thread fired!\r\n");
                // Acquire ADC data
                adc_data[0] = read_adc(6);      // Battery
                adc_data[1] = read_adc(4);      // CR1
                adc_data[2] = read_adc(5);      // CR2
                printk("ADC 6: %d, ADC 4: %d, ADC 5: %d\r\n", adc_data[0], adc_data[1], adc_data[2]);
              
                if (auto_mode)
                {
                        if (adc_data[0] > VBAT_THRESHOLD)
                        {
                                printk("Battery Power detected!\n");
                                vbat_state = true;
                        }
                        else
                        {
                                printk("Battery Power not detected!\n");
                                vbat_state = false;
                        }       

                        if ((adc_data[1] > CR1_THRESHOLD_L) && (adc_data[1] < CR1_THRESHOLD_H))
                        {
                                printk("CR1 Power detected!\n");
                                if (!cr1_state)
                                {
                                        ret = gpio_pin_set_dt(&sol2ext, 1);
                                        if (ret)
                                        {
                                                printk("Error: %s %d set failed!\r\n", sol2ext.port->name, sol2ext.pin);
                                        }

                                        ret = gpio_pin_set_dt(&bldc, 1);
                                        if (ret)
                                        {
                                                printk("Error: %s %d set failed!\r\n", bldc.port->name, bldc.pin);
                                        }
                                }
                                cr1_state = true;
                        }
                        else
                        {
                                printk("CR1 Power not detected!\n");
                                if (cr1_state)
                                {
                                        ret = gpio_pin_set_dt(&sol2ext, 0);
                                        if (ret)
                                        {
                                                printk("Error: %s %d set failed!\r\n", sol2ext.port->name, sol2ext.pin);
                                        }
                                        
                                        k_msleep(2000);
                                        
                                        ret = gpio_pin_set_dt(&bldc, 0);
                                        if (ret)
                                        {
                                                printk("Error: %s %d set failed!\r\n", bldc.port->name, bldc.pin);
                                        }
                                }
                                cr1_state = false;
                        }

                        if ((adc_data[2] > CR2_THRESHOLD_L) && (adc_data[2] < CR2_THRESHOLD_H))
                        {
                                printk("CR2 Power detected!\n");
                                if (!cr2_state)
                                {
                                        ret = gpio_pin_set_dt(&sol1ext, 1);
                                        if (ret)
                                        {
                                                printk("Error: %s %d set failed!\r\n", sol1ext.port->name, sol1ext.pin);
                                        }

                                        ret = gpio_pin_set_dt(&bldc, 1);
                                        if (ret)
                                        {
                                                printk("Error: %s %d set failed!\r\n", bldc.port->name, bldc.pin);
                                        }
                                }
                                cr2_state = true;
                        }
                        else
                        {
                                printk("CR2 Power not detected!\n");
                                if (cr2_state)
                                {
                                        ret = gpio_pin_set_dt(&sol1ext, 0);
                                        if (ret)
                                        {
                                                printk("Error: %s %d set failed!\r\n", sol1ext.port->name, sol1ext.pin);
                                        }
                                        
                                        k_msleep(2000);
                                        
                                        ret = gpio_pin_set_dt(&bldc, 0);
                                        if (ret)
                                        {
                                                printk("Error: %s %d set failed!\r\n", bldc.port->name, bldc.pin);
                                        }
                                }
                                cr2_state = false;
                        }
                }

                k_sleep(K_MSEC(EXE_SLEEP_TIME));
        }
}

K_THREAD_DEFINE(exe_thread_id, STACKSIZE, exe_thread_func, NULL, NULL, NULL, EXE_THREAD_PRIORITY, 0, 0);