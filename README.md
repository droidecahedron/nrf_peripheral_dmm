<p align="center">
  <img src="https://github.com/user-attachments/assets/f9970b40-8853-4226-a039-70d478c86104">
</p>

# IPC
In this section, we will add some basic messaging between two threads.
Looking forward, this is because we will have another task in the next section that will let us be able to see the regulator outputs on something that is not a terminal.

# Step 1
Create a [message queue](https://docs.zephyrproject.org/latest/kernel/services/data_passing/message_queues.html) struct to pass messages from our ADC thread to another thread.
- Add the following code to `main.c`, you can place it below the LED defines.
```c
// can also use zbus, or pass with work container w/ kernel work item.
struct adc_sample_msg
{
    int32_t channel_mv[2];
};
K_MSGQ_DEFINE(adc_msgq, sizeof(struct adc_sample_msg), 8,
              4); // 8 messages, 4-byte alignment
```

# Step 2
Create another thread to be the recipient of the message, as well some thread parameters like before.
- Add the following `#define`s, you can place them below the aforementioned `adc_sample_msg` struct.
```c
#define BLE_NOTIFY_INTERVAL K_MSEC(500)
#define BLE_THREAD_STACK_SIZE 1024
#define BLE_THREAD_PRIORITY 5
```

# Step 3
Modify `adc_sample_thread` to now populate and send a message.
- Declare a `adc_sample_msg` struct within the thread, and within the main loop populate the message, log what you populated, send the message, then sleep.

Here is a short snippet highlighting how to accomplish this, followed by a copy-pasteable answer.
```c
void adc_sample_thread(void) {
    //...declarations...
    struct adc_sample_msg msg;
    //...initializations...
    for(;;) {
        if(...) {}
        else {
            msg.channel_mv[i] = (err < 0) ? -1 : val_mv;
        }
        LOG_INF("ADC Thread sent: Ch0=%d mV, Ch1=%d mV", msg.channel_mv[0], msg.channel_mv[1]);
        k_msgq_put(&adc_msgq, &msg, K_FOREVER);
        k_sleep(ADC_SAMPLE_INTERVAL);
    }
}
```

> The thread should now look similar to the following:
```c
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
    struct adc_sample_msg msg;

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
                msg.channel_mv[i] = (err < 0) ? -1 : val_mv;
            }
        }
        LOG_INF("ADC Thread sent: Ch0=%d mV, Ch1=%d mV", msg.channel_mv[0], msg.channel_mv[1]);
        k_msgq_put(&adc_msgq, &msg, K_FOREVER);
        k_sleep(ADC_SAMPLE_INTERVAL);
    }
}
```

# Step 4
- Create a new thread `ble_write_thread`. This thread, for now, will just wait to receive the message from the adc thread, echo it to the terminal, and sleep for a period. Add the following code close to your BLE defines:
```c
void ble_write_thread(void)
{
    struct adc_sample_msg msg;
    for (;;)
    {
        // Wait indefinitely for a new ADC sample message from the queue
        k_msgq_get(&adc_msgq, &msg, K_FOREVER);
        // At this point, msg.channel_mv[0] and msg.channel_mv[1] contain the
        // latest ADC results
        LOG_INF("BLE thread received: Ch0(BOOST)=%d mV, Ch1(LDOLS)=%d mV", msg.channel_mv[0], msg.channel_mv[1]);
        k_sleep(BLE_NOTIFY_INTERVAL);
    }
}
```
along with its accompanying setup macro at the bottom of the file next to the other `K_THREAD_DEFINE`.
```c
K_THREAD_DEFINE(ble_write_thread_id, BLE_THREAD_STACK_SIZE, ble_write_thread, NULL, NULL, NULL, BLE_THREAD_PRIORITY, 0,
                0);
```

# Result
You should now have one thread that sends the message (and reports that it did to the log), and one thread that waits to receive the message, and reports what it received over the log as well.
```
[00:00:33.008,993] <inf> main: ADC Thread sent: Ch0=3058 mV, Ch1=801 mV
[00:00:33.009,014] <inf> main: BLE thread received: Ch0(BOOST)=3058 mV, Ch1(LDOLS)=801 mV
[00:00:34.009,172] <inf> main: ADC Thread sent: Ch0=3069 mV, Ch1=805 mV
[00:00:34.009,196] <inf> main: BLE thread received: Ch0(BOOST)=3069 mV, Ch1(LDOLS)=805 mV
```