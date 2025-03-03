#include "pwm_f.h"

uint8_t pwm_ch1_dc;

static const struct pwm_dt_spec pwm_sol3 = PWM_DT_SPEC_GET(DT_NODELABEL(pwm_sol3));
static const struct pwm_dt_spec pwm_buzz = PWM_DT_SPEC_GET(DT_NODELABEL(pwm_buzz));

int pwms_init(void)
{
    int ret;
    printk("Initializing PWM!\n");
    if (!device_is_ready(pwm_sol3.dev)) {
        printk("Error: PWM device %s is not ready!\n", pwm_sol3.dev->name);
        return 31;
    }
    
    if (!device_is_ready(pwm_buzz.dev)) {
        printk("Error: PWM device %s is not ready!\n", pwm_buzz.dev->name);
        return 32;
    }
    
    ret = pwm_set_pulse_dt(&pwm_sol3, 0);
    if (ret < 0) {
        printk("Error %d: failed to set pulse width.\n", ret);
        return 33;
    }

    ret = pwm_set_pulse_dt(&pwm_buzz, 0);
    if (ret < 0) {
        printk("Error %d: failed to set pulse width.\n", ret);
        return 34;
    }

    return 0;

}

int pwm_set_dc(uint8_t ch, uint8_t duty)
{
    if (duty <0 || duty > 100)
    {
        printk("Error: Invalid duty cycle!\n");
        return 35;
    }

    if (ch < 1 || ch > 2)
    {
        printk("Error: Invalid channel!\n");
        return 36;
    }

    int ret;
    uint32_t pulse_width;

    if (ch == 1)
    {
        float tmp = (duty/100.0) * pwm_sol3.period;
        pulse_width = (uint32_t) tmp;
        ret = pwm_set_pulse_dt(&pwm_sol3, pulse_width);
        if (ret != 0)
        {
            printk("Error: Failed to set pulse width!\n");
            return 37;
        }
        pwm_ch1_dc = duty;
    }
    else if (ch == 2)
    {
        float tmp = (duty/100.0) * pwm_buzz.period;
        pulse_width = (uint32_t) tmp;
        ret = pwm_set_pulse_dt(&pwm_buzz, pulse_width);
        if (ret != 0)
        {
            printk("Error: Failed to set pulse width!\n");
            return 38;
        }
    }
    else
    {
        return -1;
    }

    return 0;
}
