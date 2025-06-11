#include <zephyr/kernel.h>
#include <dk_buttons_and_leds.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/settings/settings.h>

LOG_MODULE_REGISTER(main, LOG_LEVEL_INF);
#define DK_STATUS_LED DK_LED1
#define BLE_STATE_LED DK_LED2

//~~~~ ADC ~~~~//
#define ADC_THREAD_STACK_SIZE 1024
#define ADC_THREAD_PRIORITY 5
#define ADC_SAMPLE_INTERVAL K_SECONDS(1)

// define variable for each channel, get ADC channel specs from devicetree
#define DT_SPEC_AND_COMMA(node_id, prop, idx) ADC_DT_SPEC_GET_BY_IDX(node_id, idx),

static const struct adc_dt_spec adc_channels[] = {
    DT_FOREACH_PROP_ELEM(DT_PATH(zephyr_user), io_channels, DT_SPEC_AND_COMMA)};

// Task dedicated to sampling the ADC
void adc_sample_thread(void)
{
    int err;
    int16_t buf[2];
    struct adc_sequence sequence = {
        .channels = BIT(adc_channels[0].channel_id) | BIT(adc_channels[1].channel_id),
        .buffer = buf,
        .buffer_size = sizeof(buf),
        .resolution = 14,
        .calibrate = true,
    };

    // Setup each channel
    for (size_t i = 0; i < ARRAY_SIZE(adc_channels); i++)
    {
        if (!adc_is_ready_dt(&adc_channels[i]))
        {
            LOG_ERR("ADC controller device %s not ready", adc_channels[i].dev->name);
            return;
        }
        err = adc_channel_setup_dt(&adc_channels[i]);
        if (err < 0)
        {
            LOG_ERR("Could not setup channel #%d (%d)", i, err);
            return;
        }
    }

    for (;;)
    {
        err = adc_read(adc_channels->dev, &sequence);
        if (err < 0)
        {
            LOG_ERR("Could not read both channels (%d)", err);
        }
        else
        {
            // Convert raw values to mV for both channels
            for (size_t i = 0; i < ARRAY_SIZE(adc_channels); i++)
            {
                int32_t val_mv = (int32_t)buf[i]; // You can check buf[i] for a raw sample.
                err = adc_raw_to_millivolts_dt(&adc_channels[i], &val_mv);
                LOG_INF("CH%d: %d mV", i, val_mv);
            }
        }
        k_sleep(ADC_SAMPLE_INTERVAL);
    }
}

int main(void)
{
    int err;
    int blink = 0;

    err = dk_leds_init();
    if (err)
    {
        LOG_ERR("LEDs init failed (err %d)", err);
        return -1;
    }

    for (;;)
    {
        dk_set_led(DK_STATUS_LED, (++blink) % 2);
        k_sleep(K_MSEC(2000));
    }
    return 0;
}

// add a thread for SAADC sampling
K_THREAD_DEFINE(adc_sample_thread_id, ADC_THREAD_STACK_SIZE, adc_sample_thread, NULL, NULL, NULL, ADC_THREAD_PRIORITY,
                0, 0);
