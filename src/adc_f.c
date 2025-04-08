#include "adc_f.h"

#define         CH2_ADC_DF           1.5        // Pressure Sensor
#define         CH4_ADC_MF           8.5      // CR1: 10k/(75k+10k) = 0.117 ~= 1/8.5
#define         CH5_ADC_MF           8.5      // CR2: 10k/(75k+10k) = 0.117 ~= 1/8.5
#define         CH6_ADC_MF           8.5      // Bat: 10k/(75k+10k) = 0.117 ~= 1/8.5   
#define         CH7_ADC_MF           1        // Therm: 10k/(10k+0) = 1

#define         ADC_MEAS_NO         10        // Number of ADC measurements for averaging
#define         ADC_MEAS_INT        2         // Interval between ADC measurements in ms

uint16_t buf;

struct adc_sequence sequence = {
	.buffer = &buf,
	/* buffer size in bytes, not number of samples */
	.buffer_size = sizeof(buf),
    .calibrate = true,
};

static const struct adc_dt_spec adc_ch0 = ADC_DT_SPEC_GET_BY_IDX(DT_PATH(zephyr_user), 0);
static const struct adc_dt_spec adc_ch2 = ADC_DT_SPEC_GET_BY_IDX(DT_PATH(zephyr_user), 1);
static const struct adc_dt_spec adc_ch4 = ADC_DT_SPEC_GET_BY_IDX(DT_PATH(zephyr_user), 2);
static const struct adc_dt_spec adc_ch5 = ADC_DT_SPEC_GET_BY_IDX(DT_PATH(zephyr_user), 3);
static const struct adc_dt_spec adc_ch6 = ADC_DT_SPEC_GET_BY_IDX(DT_PATH(zephyr_user), 4);
static const struct adc_dt_spec adc_ch7 = ADC_DT_SPEC_GET_BY_IDX(DT_PATH(zephyr_user), 5);

void adc_init(void)
{
    int err;

    if (!device_is_ready(adc_ch0.dev))
    {
    printk("ADC controller device %s not ready\n", adc_ch0.dev->name);
    }

    err = adc_channel_setup_dt(&adc_ch0);
    if (err <0)
    {
        printk("Could not setup channel #0 (%d)\n", err);
    }

    if (!device_is_ready(adc_ch2.dev))
    {
    printk("ADC controller device %s not ready\n", adc_ch2.dev->name);
    }

    err = adc_channel_setup_dt(&adc_ch2);
    if (err <0)
    {
        printk("Could not setup channel #3 (%d)\n", err);
    }

    if (!device_is_ready(adc_ch4.dev))
    {
    printk("ADC controller device %s not ready\n", adc_ch4.dev->name);
    }

    err = adc_channel_setup_dt(&adc_ch4);
    if (err <0)
    {
        printk("Could not setup channel #4 (%d)\n", err);
    }

    if (!device_is_ready(adc_ch5.dev))
    {
    printk("ADC controller device %s not ready\n", adc_ch5.dev->name);
    }

    err = adc_channel_setup_dt(&adc_ch5);
    if (err <0)
    {
        printk("Could not setup channel #5 (%d)\n", err);
    }

    if (!device_is_ready(adc_ch6.dev))
    {
    printk("ADC controller device %s not ready\n", adc_ch6.dev->name);
    }

    err = adc_channel_setup_dt(&adc_ch6);
    if (err <0)
    {
        printk("Could not setup channel #5 (%d)\n", err);
    }

    if (!device_is_ready(adc_ch7.dev))
    {
    printk("ADC controller device %s not ready\n", adc_ch7.dev->name);
    }

    err = adc_channel_setup_dt(&adc_ch7);
    if (err <0)
    {
        printk("Could not setup channel #5 (%d)\n", err);
    }

}

uint16_t read_adc(uint8_t ch)
{
    int err; 
    int32_t val_mv;
    uint8_t i;
    int32_t tmp_buf = 0;

    switch(ch)
    {
        case 0:
            printk("ADC reading of device %s, channel %d: ", 
                adc_ch0.dev->name, adc_ch0.channel_id);
            (void)adc_sequence_init_dt(&adc_ch0, &sequence);
            err = adc_read(adc_ch0.dev, &sequence);
            if (err <0)
            {
                printk("Could not read (%d)\n", err);
            } else {
                val_mv = (int32_t)buf;
            }
        
            printk("%"PRId32"\n", val_mv);
            // err = adc_raw_to_millivolts_dt(&adc_ch0, &val_mv);
            
            // if (err < 0) {
            //     printk(" (value in mV not available)\n");
            // } else {
            //     printk(" = %"PRId32" mV\n", val_mv);
            // }
            break;
        case 2:
            printk("ADC reading of device %s, channel %d: ", 
                adc_ch2.dev->name, adc_ch2.channel_id);
            (void)adc_sequence_init_dt(&adc_ch2, &sequence);
            
            for (i = 0; i < ADC_MEAS_NO; i++)
            {
                err = adc_read(adc_ch2.dev, &sequence);
                k_msleep(ADC_MEAS_INT);
                if (err <0)
                {
                    printk("Could not read (%d)\n", err);
                } else {
                    tmp_buf += (int32_t)buf;
                }
            }
            val_mv = (int32_t) tmp_buf / ADC_MEAS_NO;
            printk("%"PRId32, val_mv);
            err = adc_raw_to_millivolts_dt(&adc_ch2, &val_mv);
            
            val_mv = (int32_t)(val_mv / CH2_ADC_DF);

            if (err < 0) {
                printk(" (value in mV not available)\n");
            } else {
                printk(" = %"PRId32" mV\n", val_mv);
            }
            break;
        case 4:
            printk("ADC reading of device %s, channel %d: ", 
                adc_ch4.dev->name, adc_ch4.channel_id);
            (void)adc_sequence_init_dt(&adc_ch4, &sequence);
            for (i = 0; i < ADC_MEAS_NO; i++)
            {
                err = adc_read(adc_ch4.dev, &sequence);
                k_msleep(ADC_MEAS_INT);
                if (err <0)
                {
                    printk("Could not read (%d)\n", err);
                } else {
                    tmp_buf += (int32_t)buf;
                }
            }
            val_mv = (int32_t) tmp_buf / ADC_MEAS_NO;
        
            printk("%"PRId32, val_mv);
            err = adc_raw_to_millivolts_dt(&adc_ch4, &val_mv);
        
            val_mv = (int32_t)(val_mv * CH4_ADC_MF);

            if (val_mv > 50000)
            {
                val_mv = 0;
            }

            if (err < 0) {
                printk(" (value in mV not available)\n");
            } else {
                printk(" = %"PRId32" mV\n", val_mv);
            }
            break;
        case 5:
            printk("ADC reading of device %s, channel %d: ", 
                adc_ch5.dev->name, adc_ch5.channel_id);
            (void)adc_sequence_init_dt(&adc_ch5, &sequence);
            for (i = 0; i < ADC_MEAS_NO; i++)
            {
                err = adc_read(adc_ch5.dev, &sequence);
                k_msleep(ADC_MEAS_INT);
                if (err <0)
                {
                    printk("Could not read (%d)\n", err);
                } else {
                    tmp_buf += (int32_t)buf;
                }
            }
            val_mv = (int32_t) tmp_buf / ADC_MEAS_NO;
            printk("%"PRId32, val_mv);
            err = adc_raw_to_millivolts_dt(&adc_ch5, &val_mv);
        
            val_mv = (int32_t)(val_mv * CH5_ADC_MF);

            if (val_mv > 50000)
            {
                val_mv = 0;
            }

            if (err < 0) {
                printk(" (value in mV not available)\n");
            } else {
                printk(" = %"PRId32" mV\n", val_mv);
            }
            break;
        case 6:
            printk("ADC reading of device %s, channel %d: ", 
                adc_ch6.dev->name, adc_ch6.channel_id);
            (void)adc_sequence_init_dt(&adc_ch6, &sequence);
            err = adc_read(adc_ch6.dev, &sequence);
            if (err <0)
            {
                printk("Could not read (%d)\n", err);
            } else {
                val_mv = (int32_t)buf;
            }
        
            printk("%"PRId32, val_mv);
            err = adc_raw_to_millivolts_dt(&adc_ch6, &val_mv);
        
            val_mv = (int32_t)(val_mv * CH6_ADC_MF);

            if (err < 0) {
                printk(" (value in mV not available)\n");
            } else {
                printk(" = %"PRId32" mV\n", val_mv);
            }
            break;
        case 7:
            printk("ADC reading of device %s, channel %d: ", 
                adc_ch7.dev->name, adc_ch7.channel_id);
            (void)adc_sequence_init_dt(&adc_ch7, &sequence);
            
            for (i = 0; i < ADC_MEAS_NO; i++)
            {
                err = adc_read(adc_ch7.dev, &sequence);
                k_msleep(ADC_MEAS_INT);
                if (err <0)
                {
                    printk("Could not read (%d)\n", err);
                } else {
                    tmp_buf += (int32_t)buf;
                }
            }
            val_mv = (int32_t) tmp_buf / ADC_MEAS_NO;
            printk("%"PRId32"\n", val_mv);
            // err = adc_raw_to_millivolts_dt(&adc_ch7, &val_mv);
        
            // val_mv = (int32_t)(val_mv * CH7_ADC_MF);

            // if (err < 0) {
            //     printk(" (value in mV not available)\n");
            // } else {
            //     printk(" = %"PRId32" mV\n", val_mv);
            // }
            break;
        default:
            printk("Invalid channel\n");
            return;
    }

    return (uint16_t)val_mv;

  
}
