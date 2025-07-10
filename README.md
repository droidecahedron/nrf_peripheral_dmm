# ADC
## Progression Path
```mermaid
graph LR;
  main-->adc*;
  adc*-->npm_powerup;
  npm_powerup-->ipc;
  ipc-->ble;
```
> `*` == your current location

## Preface
We will be using two SAADC channels on the 54L15 to read the regulator outputs from the nPM2100.
The nRF54L15 sports a SAADC peripheral, or a differential successive approximation register (SAR) analog-to-digital converter.
 The main features of SAADC are the following:
- Four accuracy modes
  - 10-bit mode with a maximum sample rate of 2 Msps
  - 12-bit mode with a sample rate of ksps
  - 14-bit mode with a sample rate of 31.25 ksps
  - Oversampling mode with configurable sample rate
- 8/10/12-bit resolution, 14-bit resolution with oversampling
- Multiple analog inputs (up to eight input chs)
- GPIO pins with analog function (input range 0 to VDD)
- VDD (divided down to a valid range)
- Up to eight input channels
- One input per single-ended channel, and two inputs per differential channel
- Scan mode can be configured with both single-ended inputs and differential inputs
- Each channel can be configured to select any of the above analog inputs
- Sampling triggered by a task from software or a DPPI channel for full flexibility on sample frequency source from low-power 32.768 kHz RTC or more accurate 1/16 MHz timers
- One-shot conversion mode to sample a single channel
- Scan mode to sample a series of channels in sequence with configurable sample delay
- Support for direct sample transfer to RAM using EasyDMA
- Interrupts on single sample and full buffer events
- Samples stored as 16-bit two's complement values for differential and single-ended sampling
- Continuous sampling without the need of an external timer
- On-the-fly limit checking

<p align="center">
  <img src="https://github.com/user-attachments/assets/5b1fc448-3f18-4582-b479-4c7959612e81" width="45%"/>
  <img src="https://sp-ao.shortpixel.ai/client/to_webp,q_glossy,ret_img,w_1280,h_1080/https://academy.nordicsemi.com/wp-content/uploads/2024/03/Successive-Approximation.gif" width="45%" alt="animated" />
</p>

## Step 1
- Navigate to the overlay file, called `nrf54l15dk_nrf54l15_cpuapp.overlay`, in the `boards/` directory.
The overlay will enable the adc node on the chip and define some of its parameters.
(If you need a refresher on devicetree: [LINK](https://academy.nordicsemi.com/courses/nrf-connect-sdk-fundamentals/lessons/lesson-2-reading-buttons-and-controlling-leds/topic/devicetree/))

- Add the following code to the overlay file. We will be using two AINs, so two channels, one per regulator output on the nPM2100.
```
/ {
	zephyr,user {
		io-channels = <&adc 0>, <&adc 1>;
	};
};

&adc {
	#address-cells = <1>;
	#size-cells = <0>;
	status = "okay";
	channel@0 {
		reg = <0>;
		zephyr,gain = "ADC_GAIN_1_4";
		zephyr,reference = "ADC_REF_INTERNAL";
		zephyr,acquisition-time = <ADC_ACQ_TIME_DEFAULT>;
		zephyr,input-positive = <NRF_SAADC_AIN4>; /* P1.11 for the nRF54L15 DK, BOOST/VOUT on pmic */
		zephyr,resolution = <14>;
	};
	channel@1 {
		reg = <1>;
		zephyr,gain = "ADC_GAIN_1_4";
		zephyr,reference = "ADC_REF_INTERNAL";
		zephyr,acquisition-time = <ADC_ACQ_TIME_DEFAULT>;
		zephyr,input-positive = <NRF_SAADC_AIN5>; /* Example: P1.12 for the nRF54L15 DK, LSLDO OUT on pmic */
		zephyr,resolution = <14>;
	};
};
```
- Save the `.overlay` file with your modifications.

## Step 2
Configure the project and add the button and LED lib for the DK, logging, and ADC support.
- Navigate to the main dir of the project, where `prj.conf` is located. This is the configuration file.
- Add the following code to the `prj.conf` file and save the changes.
```
# Button and LED library
CONFIG_DK_LIBRARY=y

# Add logging
CONFIG_LOG=y

# Add ADC support
CONFIG_ADC=y

# Increase stack size for the main thread and System Workqueue
CONFIG_SYSTEM_WORKQUEUE_STACK_SIZE=2048
CONFIG_MAIN_STACK_SIZE=2048
```

## Step 3
Include libraries in our (mostly) empty `main.c` source file, and set up a logger module.
- Navigate to `main.c` and add the following includes:
```c
#include <dk_buttons_and_leds.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/settings/settings.h>
```

And set up a logger module, as well as add some useful defines for blinking in the next few steps.
```c
LOG_MODULE_REGISTER(main, LOG_LEVEL_INF);
#define DK_STATUS_LED DK_LED1
#define BLE_STATE_LED DK_LED2
```

## Step 4
Retrieve the API-specific device structure for the ADC channel
- Add API specific dts structure. We'll also define some thread creation options and sleep interval here as well.
```c
//~~~~ ADC ~~~~//
#define ADC_THREAD_STACK_SIZE 1024
#define ADC_THREAD_PRIORITY 5
#define ADC_SAMPLE_INTERVAL K_SECONDS(1)

// define variable for each channel, get ADC channel specs from devicetree
#define DT_SPEC_AND_COMMA(node_id, prop, idx) ADC_DT_SPEC_GET_BY_IDX(node_id, idx),

static const struct adc_dt_spec adc_channels[] = {
    DT_FOREACH_PROP_ELEM(DT_PATH(zephyr_user), io_channels, DT_SPEC_AND_COMMA)};
```

## Step 5
Add blinky to the main thread.
- Modify `main()` to add some proof of life by blinking an LED.
```c
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
```



## Step 6
Set up another thread to initialize and sample the ADC.

- Add the adc thread function. Here, we will initialize the adc with the devicetree and sample it periodically, printing the output to the console.
```c
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
```

## Step 7
Create the ADC thread.
- At the bottom of `main.c`, add the following snippet to define the thread.
```
// add a thread for SAADC sampling
K_THREAD_DEFINE(adc_sample_thread_id, ADC_THREAD_STACK_SIZE, adc_sample_thread, NULL, NULL, NULL, ADC_THREAD_PRIORITY,
                0, 0);
```

## Step 8
- Run a pristine build.

<img width="161" height="58" alt="image" src="https://github.com/user-attachments/assets/5853a50c-d9e4-46d2-ba1c-c395a0da596d" />

- Flash your device.
- Click the following button to flash your detected DK.

<img width="234" height="163" alt="image" src="https://github.com/user-attachments/assets/e423befe-bfb1-4c11-bc71-3c5165d55931" />

## Step 9
Connect to the log output com port. (*Make sure you've disconnected the 54L15DK from the board configurator and the serial port is free!*)
- The default UART settings are `115200,8,n,1,N`. The VSC Extension GUI will give you a single button click for this in the top center of your screen after you click the 'connect' button.
  
  ![image](https://github.com/user-attachments/assets/b42af1fa-e641-4601-a252-cc53a4a373c8)

  ![image](https://github.com/user-attachments/assets/a61ac40e-ef29-4cd0-a2f2-203306f9cb10)


# Result
You should have an LED toggle every two seconds after flashing, and when you connect to VCOM1 of the DK via USB at `115200,8,n,1,N`, you should see it print the sample results for each channel. It is OK that nothing is hooked up and those pins are floating -- we are just setting it up for the next step.

> Your output should look like this...
```
*** Booting nRF Connect SDK v3.0.1-9eb5615da66b ***
*** Using Zephyr OS v4.0.99-77f865b8f8d0 ***
[00:00:00.002,295] <inf> main: CH0: 147 mV
[00:00:00.002,301] <inf> main: CH1: 0 mV
[00:00:01.002,535] <inf> main: CH0: 151 mV
[00:00:01.002,545] <inf> main: CH1: 0 mV
[00:00:02.002,706] <inf> main: CH0: 151 mV
```


## Move to the npm_powerup branch for the next set of instructions: [➡️LINK](https://github.com/droidecahedron/nrf_peripheral_dmm/tree/npm_powerup)
### Move to the adc_soln branch if you are stuck and need a lift: [🫱LINK](https://github.com/droidecahedron/nrf_peripheral_dmm/tree/adc_soln)
