#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/sys/printk.h>
#include <hal/nrf_gpio.h>
#include <math.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/storage/flash_map.h>
#include <zephyr/fs/nvs.h>

#include "common.h"
#include "remote.h"
#include "pwm_f.h"
#include "adc_f.h"

#define         STACKSIZE               2048
#define         EXE_THREAD_PRIORITY     6
#define         TRIG_THREAD_PRIORITY    7
#define         EXE_SLEEP_TIME          100

#define         SOL1_NODE               DT_ALIAS(sol1)
#define         SOL1EXT_NODE            DT_ALIAS(sol1ext)
#define         SOL2_NODE               DT_ALIAS(sol2)
#define         SOL2EXT_NODE            DT_ALIAS(sol2ext)
#define         BLDC_NODE               DT_ALIAS(bldc)

#define         ADC_NO_CH               8
#define         VBAT_THRESHOLD          9000
#define         CR1_THRESHOLD_L         300
#define         CR1_THRESHOLD_H         14000
#define         CR2_THRESHOLD_L         300
#define         CR2_THRESHOLD_H         14000
#define         MTR_OFF_DELAY           2000

#define         RTD_RBIAS               10000

#define NVS_PARTITION		storage_partition
#define NVS_PARTITION_DEVICE	FIXED_PARTITION_DEVICE(NVS_PARTITION)
#define NVS_PARTITION_OFFSET	FIXED_PARTITION_OFFSET(NVS_PARTITION)
#define CR_THRESH_ID            1

int ret;
bool vbat_state = false;
bool cr1_state = false;
bool cr2_state = false;

static struct nvs_fs fs;
uint16_t nvs_cr;
bool nvs_wrt_f = false;

uint16_t cr_thresh = 500;

extern uint8_t pwm_ch1_dc;
uint16_t adc_data[ADC_NO_CH] = {0};
uint16_t press_diff;
bool auto_mode = true;

static const struct gpio_dt_spec sol1 = GPIO_DT_SPEC_GET(SOL1_NODE, gpios);
static const struct gpio_dt_spec sol1ext = GPIO_DT_SPEC_GET(SOL1EXT_NODE, gpios);
static const struct gpio_dt_spec sol2 = GPIO_DT_SPEC_GET(SOL2_NODE, gpios);
static const struct gpio_dt_spec sol2ext = GPIO_DT_SPEC_GET(SOL2EXT_NODE, gpios);
static const struct gpio_dt_spec bldc = GPIO_DT_SPEC_GET(BLDC_NODE, gpios);

static void timer0_handler(struct k_timer *dummy)
{
        int ret;
        if (!cr1_state && !cr2_state)
        {
                ret = gpio_pin_set_dt(&bldc, 0);
                if (ret)
                {
                        printk("Error: %s %d set failed!\r\n", bldc.port->name, bldc.pin);
                }
        }

}

K_TIMER_DEFINE(timer0, timer0_handler, NULL);

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

static void app_cr_cb(uint16_t crval)
{
        printk("Current CR: %d, Received CR: %d\r\n", cr_thresh, crval);
        if (crval >= 500 && crval <= 6000)
        {
                cr_thresh = crval;
                nvs_wrt_f = true;
                // (void)nvs_write(&fs, CR_THRESH_ID, &cr_thresh, sizeof(cr_thresh));
                // printk("Set threshold in NVS: %d\r\n", cr_thresh);
        } else {
                printk("Incorrect value\r\n");
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
        .cr_cb = app_cr_cb,
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

        /* NVS Initialization */
        struct flash_pages_info info;
        int rc = 0;
	fs.flash_device = NVS_PARTITION_DEVICE;
	if (!device_is_ready(fs.flash_device)) {
		printk("Flash device %s is not ready\n", fs.flash_device->name);
		return 0;
	}
	fs.offset = NVS_PARTITION_OFFSET;
	rc = flash_get_page_info_by_offs(fs.flash_device, fs.offset, &info);
	if (rc) {
		printk("Unable to get page info\n");
		return 0;
	}
	fs.sector_size = info.size;
	fs.sector_count = 2U; // 2 Sectors

	rc = nvs_mount(&fs);
	if (rc) {
		printk("Flash Init failed\n");
		return 0;
	}
        
        rc = nvs_read(&fs, CR_THRESH_ID, &nvs_cr, sizeof(nvs_cr));
        if (rc > 0)
        {
               	/* item was found, show it */
		printk("Id: %d, Data: %d\n",
			CR_THRESH_ID, nvs_cr);
                // Set CR threshold
                cr_thresh = nvs_cr;
        } else {
                printk("No CR threshold found, adding %d at id %d\n",
		       cr_thresh, CR_THRESH_ID);
                       (void)nvs_write(&fs, CR_THRESH_ID, &cr_thresh, sizeof(cr_thresh));
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

        while (1)
        {
                if (nvs_wrt_f)
                {
                        (void)nvs_write(&fs, CR_THRESH_ID, &cr_thresh, sizeof(cr_thresh));
                        printk("Set threshold in NVS: %d\r\n", cr_thresh);
                        nvs_wrt_f = false;
                }
                printk("ADC Thread fired!\r\n");
                // Acquire ADC data
                // adc_data[0] = read_adc(6);      // Battery
                // adc_data[1] = read_adc(4);      // CR1
                // adc_data[2] = read_adc(5);      // CR2
                // adc_data[3] = read_adc(2);      // Pressure
                // adc_data[4] = read_adc(7);      // Temperature NTC
                // adc_data[5] = read_adc(0);      // VDD
                // adc_data[6] = read_adc(3);      // Battery/ Pressure

                // Acquire ADC data
                adc_data[0] = read_adc(0);      // Temp PSU
                adc_data[1] = read_adc(1);      // CR1
                adc_data[2] = read_adc(2);      // Pressure +ve
                adc_data[3] = read_adc(3);      // Pressure -ve
                adc_data[4] = read_adc(4);      // Internal VDD
                adc_data[5] = read_adc(5);      // CR2
                adc_data[6] = read_adc(6);      // Battery
                adc_data[7] = read_adc(7);      // Temp Pump

                /* Pressure Diff */
                press_diff = adc_data[2] - adc_data[3];

                /* Temperature PSU */
                // Obtain Rth and temperature
                //float rth = RTD_RBIAS * ((1024 / (1024.0 - (float)adc_data[4])) - 1);
                float rth = RTD_RBIAS * ((1023.0/ (float)(adc_data[0] * 1.5)) - 1);
                printk("R Thermistor: %d \r\n", (int32_t)(rth * 10));
                float tK = (3950.0 * 298.15) / (3950 + (298.15 * log(rth / 10000)));
                float tC = tK - 273.15;
                printk("Temperature: %d \r\n", (int32_t)(tC * 10));
                adc_data[4] = (uint16_t)(tC*10); 

                /* Temperature Pump */
                // Obtain Rth and temperature
                //float rth = RTD_RBIAS * ((1024 / (1024.0 - (float)adc_data[4])) - 1);
                rth = RTD_RBIAS * ((1023.0/ (float)(adc_data[7] * 1.5)) - 1);
                printk("R Thermistor: %d \r\n", (int32_t)(rth * 10));
                tK = (3950.0 * 298.15) / (3950 + (298.15 * log(rth / 10000)));
                tC = tK - 273.15;
                printk("Temperature: %d \r\n", (int32_t)(tC * 10));
                adc_data[4] = (uint16_t)(tC*10); 

                //printk("ADC 6: %d, ADC 4: %d, ADC 5: %d, ADC 2: %d, ADC 7: %d, VDD: %d \r\n", adc_data[0], adc_data[1], adc_data[2], adc_data[3], adc_data[4], adc_data[5]);
                printk("Temp PSU: %d, VDD: %d, CR1: %d, CR2: %d, VBat: %d, Temp Pump: %d \r\n", adc_data[0], adc_data[4], adc_data[1], adc_data[5], adc_data[6], adc_data[7]);

                if (adc_data[6] >= VBAT_THRESHOLD)
                {
                        printk("Battery Power detected!\n");
                        vbat_state = true;
                        auto_mode = false;

                        // ret = gpio_pin_set_dt(&sol1, 1);
                        // if (ret)
                        // {
                        //         printk("Error: %s %d set failed!\r\n", sol1ext.port->name, sol1ext.pin);
                        // }

                        // ret = gpio_pin_set_dt(&sol2, 1);
                        // if (ret)
                        // {
                        //         printk("Error: %s %d set failed!\r\n", sol2ext.port->name, sol2ext.pin);
                        // }

                        ret = gpio_pin_set_dt(&bldc, 1);
                        if (ret)
                        {
                                printk("Error: %s %d set failed!\r\n", bldc.port->name, bldc.pin);
                        }

                        pwm_set_dc(1, 50);
                }
                else
                {
                        printk("Battery Power not detected!\n");

                        if (vbat_state)
                        {
                                // ret = gpio_pin_set_dt(&sol1, 0);
                                // if (ret)
                                // {
                                //         printk("Error: %s %d set failed!\r\n", sol1ext.port->name, sol1ext.pin);
                                // }

                                // ret = gpio_pin_set_dt(&sol2, 0);
                                // if (ret)
                                // {
                                //         printk("Error: %s %d set failed!\r\n", sol2ext.port->name, sol2ext.pin);
                                // }

                                pwm_set_dc(1, 0);

                                //k_msleep(MTR_OFF_DELAY);
                                ret = gpio_pin_set_dt(&bldc, 0);
                                if (ret)
                                {
                                        printk("Error: %s %d set failed!\r\n", bldc.port->name, bldc.pin);
                                }

                        
                        }

                        vbat_state = false;
                        auto_mode = true;
                }  

                bool cr1_flag = (adc_data[1] > cr_thresh) && (adc_data[1] < CR1_THRESHOLD_H);
                bool cr2_flag = (adc_data[5] > cr_thresh) && (adc_data[5] < CR2_THRESHOLD_H);

                if (auto_mode && !vbat_state)
                {
                        // Both CR1 & CR2 detected
                        if (cr1_flag && cr2_flag)
                        {
                                printk("Both CR1 & CR2 Powers detected!\n");
                                // Turn OFF SOL-2
                                ret = gpio_pin_set_dt(&sol2ext, 0);
                                if (ret)
                                {
                                        printk("Error: %s %d set failed!\r\n", sol2ext.port->name, sol2ext.pin);
                                }

                                // Turn OFF SOL-1
                                ret = gpio_pin_set_dt(&sol1ext, 0);
                                if (ret)
                                {
                                        printk("Error: %s %d set failed!\r\n", sol1ext.port->name, sol1ext.pin);
                                }                      
                                
                                // Turn ON BLDC Motor
                                ret = gpio_pin_set_dt(&bldc, 1);
                                if (ret)
                                {
                                        printk("Error: %s %d set failed!\r\n", bldc.port->name, bldc.pin);
                                }
                                
                                // Turn ON SOL-3
                                pwm_set_dc(1, 50);
                                                
                        }
                        // When CR1 detected but not CR2
                        else if (cr1_flag && !cr2_flag)
                        {
                                printk("CR1 Power detected!\n");
                                if (!cr1_state)
                                {
                                        // Turn ON SOL-2
                                        ret = gpio_pin_set_dt(&sol2ext, 1);
                                        if (ret)
                                        {
                                                printk("Error: %s %d set failed!\r\n", sol2ext.port->name, sol2ext.pin);
                                        }
                                        if (!cr2_state)         // if BLDC & SOL-3 not already active
                                        {
                                                // Turn ON BLDC
                                                ret = gpio_pin_set_dt(&bldc, 1);
                                                if (ret)
                                                {
                                                        printk("Error: %s %d set failed!\r\n", bldc.port->name, bldc.pin);
                                                }
                                                // Turn ON SOL-3
                                                pwm_set_dc(1, 50);
                                        }
                                        cr1_state = true;
                                }
                                

                                printk("CR2 Power not detected!\n");
                                if (cr2_state)
                                {
                                        // Turn OFF SOL-1
                                        ret = gpio_pin_set_dt(&sol1ext, 0);
                                        if (ret)
                                        {
                                                printk("Error: %s %d set failed!\r\n", sol1ext.port->name, sol1ext.pin);
                                        }

                                        cr2_state = false;
                                }
                                
                        }
                        // When CR2 detected but not CR1
                        else if (cr2_flag && !cr1_flag)
                        {
                                printk("CR2 Power detected!\n");
                                if (!cr2_state)
                                {
                                        // Turn ON SOL-1
                                        ret = gpio_pin_set_dt(&sol1ext, 1);
                                        if (ret)
                                        {
                                                printk("Error: %s %d set failed!\r\n", sol1ext.port->name, sol1ext.pin);
                                        }
                                        if (!cr1_state)         // if BLDC & SOL-3 not already active
                                        {
                                                // Turn ON BLDC
                                                ret = gpio_pin_set_dt(&bldc, 1);
                                                if (ret)
                                                {
                                                        printk("Error: %s %d set failed!\r\n", bldc.port->name, bldc.pin);
                                                }
                                                // Turn ON SOL-3
                                                pwm_set_dc(1, 50);
                                        }
                                        cr2_state = true;
                                }
                                

                                printk("CR1 Power not detected!\n");
                                if (cr1_state)
                                {
                                        // Turn OFF SOL-2
                                        ret = gpio_pin_set_dt(&sol2ext, 0);
                                        if (ret)
                                        {
                                                printk("Error: %s %d set failed!\r\n", sol2ext.port->name, sol2ext.pin);
                                        }

                                        cr1_state = false;
                                }
                                
                        }
                        
                        else
                        {
                                printk("CR1 Power not detected!\n");
                                if (cr1_state)
                                {
                                        // Turn OFF SOL-2
                                        ret = gpio_pin_set_dt(&sol2ext, 0);
                                        if (ret)
                                        {
                                                printk("Error: %s %d set failed!\r\n", sol2ext.port->name, sol2ext.pin);
                                        }
                                        // Turn OFF SOL-3
                                        pwm_set_dc(1, 0);
                                        // Turn OFF BLDC through timer
                                        k_timer_start(&timer0, K_MSEC(MTR_OFF_DELAY), K_FOREVER);

                                }
                                cr1_state = false;

                                printk("CR2 Power not detected!\n");
                                if (cr2_state)
                                {
                                        // Turn OFF SOL-1
                                        ret = gpio_pin_set_dt(&sol1ext, 0);
                                        if (ret)
                                        {
                                                printk("Error: %s %d set failed!\r\n", sol1ext.port->name, sol1ext.pin);
                                        }
                                        // Turn OFF SOL-3
                                        pwm_set_dc(1, 0);
                                        // Turn OFF BLDC through timer
                                        k_timer_start(&timer0, K_MSEC(MTR_OFF_DELAY), K_FOREVER);
                                }
                                cr2_state = false;
                        }

                }

                k_sleep(K_MSEC(EXE_SLEEP_TIME));
        }
}

K_THREAD_DEFINE(exe_thread_id, STACKSIZE, exe_thread_func, NULL, NULL, NULL, EXE_THREAD_PRIORITY, 0, 0);