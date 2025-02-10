#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/sys/printk.h>
#include <zephyr/drivers/pwm.h>

int pwms_init(void);
int pwm_set_dc(uint8_t ch, uint8_t duty);
