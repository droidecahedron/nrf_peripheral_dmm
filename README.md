# BLE
## Progression Path
```mermaid
graph LR;
  main-->adc;
  adc-->npm_powerup;
  npm_powerup-->ipc;
  ipc-->ble*;
```
> `*` == your current location
## Preface
In this section, we will add the final touch -- making our 54L15 device connectable via our smartphone so that we can see the nPM2100 regulator values in our phone instead of a terminal.

> Note: if you recall from the block diagram in the first readme of the workshop, the 2100 is also a fuel gauge! So it is also feasible to add an i2c bus and be able to send the battery percentage to your phone, but that is not covered in this workshop.
> *However, this workshop equips you with all the tools necessary to accomplish that.*

We will also add a custom service. 

! This section quickly goes over BLE sections quickly, if you wish to dive deeper or re-visit BLE fundamentals, visit the following: [🔗Nordic DevAcademy Bluetooth Low Energy Fundamentals](https://academy.nordicsemi.com/courses/bluetooth-low-energy-fundamentals/).

## Step 1
Configure the project to enable BLE features.
- in `prj.conf`, add the following lines:
```
# Bluetooth LE
CONFIG_BT=y
CONFIG_BT_PERIPHERAL=y
# Replace ZZZZZ with a unique identifier to make scanning for advertisers easier
CONFIG_BT_DEVICE_NAME="ZZZZZ_PMIC"
CONFIG_BT_MAX_CONN=1
```
As the comment notes, replace ZZZZZ with something unique to you to make scanning for your particular device easier.

## Step 2
Add BLE libraries
- Add the following `#includes` to `main.c`:
```c
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/uuid.h>
```

## Step 3
Define a custom GATT service and characteristic UUIDs for collecting the regulator information.
- Add the following close to the BLE thread `#define`s.
```c
// Declaration of custom GATT service and characteristics UUIDs
#define PMIC_HUB_SERVICE_UUID BT_UUID_128_ENCODE(0x69e5204b, 0x8445, 0x5fca, 0xb332, 0xc13064b9dea2)
#define BOOST_MV_CHARACTERISTIC_UUID BT_UUID_128_ENCODE(0xB0057000, 0x7ea8, 0x4008, 0xb432, 0xb46096c049ba)
#define LSLDO_MV_CHARACTERISTIC_UUID BT_UUID_128_ENCODE(0x757D0111, 0xd8ee, 0x4faf, 0x956b, 0xafb01c17d0be)

#define BT_UUID_PMIC_HUB BT_UUID_DECLARE_128(PMIC_HUB_SERVICE_UUID)
#define BT_UUID_PMIC_HUB_BOOST_MV BT_UUID_DECLARE_128(BOOST_MV_CHARACTERISTIC_UUID)
#define BT_UUID_PMIC_HUB_LSLDO_MV BT_UUID_DECLARE_128(LSLDO_MV_CHARACTERISTIC_UUID)

#define DEVICE_NAME CONFIG_BT_DEVICE_NAME // from prj.conf
#define DEVICE_NAME_LEN (sizeof(DEVICE_NAME) - 1)
```

## Step 4
Prepare advertising, scan response, and corresponding packets, as well as CCCD callback.
- Add the following code after the defines:
```c
static const struct bt_le_adv_param *adv_param =
    BT_LE_ADV_PARAM((BT_LE_ADV_OPT_CONN | BT_LE_ADV_OPT_USE_IDENTITY), /* Connectable advertising and use
                                                                          identity address */
                    800,   /* Min Advertising Interval 500ms (800*0.625ms) 16383 max*/
                    801,   /* Max Advertising Interval 500.625ms (801*0.625ms) 16384 max*/
                    NULL); /* Set to NULL for undirected advertising */

static struct k_work adv_work;
static const struct bt_data ad[] = {
    BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
    BT_DATA(BT_DATA_NAME_COMPLETE, DEVICE_NAME, DEVICE_NAME_LEN),
};

static const struct bt_data sd[] = {
    BT_DATA_BYTES(BT_DATA_UUID128_ALL, PMIC_HUB_SERVICE_UUID),
};
```

## Step 5
Define GATT service and CCCD callback.
- Add the following after step 4's code:
```c
/*This function is called whenever the Client Characteristic Control Descriptor
(CCCD) has been changed by the GATT client, for each of the characteristics*/
static void on_cccd_changed(const struct bt_gatt_attr *attr, uint16_t value)
{
    ARG_UNUSED(attr);
    switch (value)
    {
    case BT_GATT_CCC_NOTIFY:
        break;
    case 0:
        break;
    default:
        LOG_ERR("Error, CCCD has been set to an invalid value");
    }
}

BT_GATT_SERVICE_DEFINE(pmic_hub, BT_GATT_PRIMARY_SERVICE(BT_UUID_PMIC_HUB),

                       BT_GATT_CHARACTERISTIC(BT_UUID_PMIC_HUB_BOOST_MV, BT_GATT_CHRC_NOTIFY, BT_GATT_PERM_READ, NULL,
                                              NULL, NULL),
                       BT_GATT_CCC(on_cccd_changed, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),

                       BT_GATT_CHARACTERISTIC(BT_UUID_PMIC_HUB_LSLDO_MV, BT_GATT_CHRC_NOTIFY, BT_GATT_PERM_READ, NULL,
                                              NULL, NULL),
                       BT_GATT_CCC(on_cccd_changed, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE), );
```

## Step 6
Add Bluetooth connection callbacks and globals to handle the various states and actions the peripheral requires.
This also uses [Kernel Work Queues](https://docs.zephyrproject.org/latest/kernel/services/threads/workqueue.html) for the process of starting advertising.
One of the globals that will be particularly for us later in the code is the connection handle.
Earlier in the workshop we `#define`d a `BLE_STATE_LED`. Now we can use it in our `connected` and `disconnected` callbacks to have an indicator on the DK let us know our connection state!
- Add the following code:
```c
// BT globals and callbacks
struct bt_conn *m_connection_handle = NULL;
static void adv_work_handler(struct k_work *work)
{
    int err = bt_le_adv_start(adv_param, ad, ARRAY_SIZE(ad), sd, ARRAY_SIZE(sd));
    if (err)
    {
        LOG_INF("Advertising failed to start (err %d)", err);
        return;
    }

    LOG_INF("Advertising successfully started");
}

static void advertising_start(void)
{
    k_work_submit(&adv_work);
}

static void recycled_cb(void)
{
    LOG_INF("Connection object available from previous conn. Disconnect is "
            "complete!");
    advertising_start();
}

static void connected(struct bt_conn *conn, uint8_t err)
{
    if (err)
    {
        LOG_WRN("Connection failed (err %u)", err);
        return;
    }
    m_connection_handle = conn;
    LOG_INF("Connected");
    dk_set_led_on(BLE_STATE_LED);
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
    LOG_INF("Disconnected (reason %u)", reason);
    m_connection_handle = NULL;
    dk_set_led_off(BLE_STATE_LED);
}

struct bt_conn_cb connection_callbacks = {
    .connected = connected,
    .disconnected = disconnected,
    .recycled = recycled_cb,
};
```

## Step 7
Since we've already gotten to the step that we can modify the regulator outputs with nPM PowerUP, and seen that those values are collected in the adc thread, then passed to the BLE Write thread, we will now add helper functions that will report the regulator values to the mobile device via notifications. 
We also want to make sure that the mobile side is prepared to receive the notifications, so we can add some conditionals to help facilitate that.
- Add the following functions:
```c
static void ble_report_boost_mv(struct bt_conn *conn, const uint32_t *data, uint16_t len)
{
    const struct bt_gatt_attr *attr = &pmic_hub.attrs[2];
    struct bt_gatt_notify_params params = {
        .uuid = BT_UUID_PMIC_HUB_BOOST_MV, .attr = attr, .data = data, .len = len, .func = NULL};

    if (bt_gatt_is_subscribed(conn, attr, BT_GATT_CCC_NOTIFY))
    {
        if (bt_gatt_notify_cb(conn, &params))
        {
            LOG_ERR("Error, unable to send notification");
        }
    }
    else
    {
        LOG_WRN("Warning, notification not enabled for boost mv characteristic");
    }
}

static void ble_report_lsldo_mv(struct bt_conn *conn, const uint32_t *data, uint16_t len)
{
    const struct bt_gatt_attr *attr = &pmic_hub.attrs[5];

    struct bt_gatt_notify_params params = {
        .uuid = BT_UUID_PMIC_HUB_LSLDO_MV, .attr = attr, .data = data, .len = len, .func = NULL};

    if (bt_gatt_is_subscribed(conn, attr, BT_GATT_CCC_NOTIFY))
    {
        // Send the notification
        if (bt_gatt_notify_cb(conn, &params))
        {
            LOG_ERR("Error, unable to send notification");
        }
    }
    else
    {
        LOG_WRN("Warning, notification not enabled for lsldo mv characteristic");
    }
}
```

## Step 8
Now we need to modify our `ble_write_thread` to send notifications *if* the device is connected!
- Update the ble_write_thread to use the helper functions previously defined if there is a valid connection context. Here is a pasteable example:
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

        if (m_connection_handle) // if ble connection present
        {
            ble_report_boost_mv(m_connection_handle, &msg.channel_mv[0], sizeof(msg.channel_mv[0]));
            ble_report_lsldo_mv(m_connection_handle, &msg.channel_mv[1], sizeof(msg.channel_mv[1]));
        }
        else
        {
            LOG_INF("BLE Thread does not detect an active BLE connection");
        }

        k_sleep(BLE_NOTIFY_INTERVAL);
    }
}
```

## Step 9
Now we need to have our main thread initialize BLE and get the show going!
- Modify `main()` thread with BLE inits, initialization of the `k_work` item, and register the relevant callbacks. The main thread should look like the following:
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

    // Setting up Bluetooth
    err = bt_enable(NULL);
    if (err)
    {
        LOG_ERR("Bluetooth init failed (err %d)", err);
        return -1;
    }
    LOG_INF("Bluetooth initialized");
    if (IS_ENABLED(CONFIG_SETTINGS))
    {
        settings_load();
    }
    bt_conn_cb_register(&connection_callbacks);
    k_work_init(&adv_work, adv_work_handler);
    advertising_start();

    for (;;)
    {
        dk_set_led(DK_STATUS_LED, (++blink) % 2);
        k_sleep(K_MSEC(2000));
    }
    return 0;
}
```

## Step 10 / Result
From here, all the pieces are together, now we can flash it and see on our app!
- Flash the device
- Your log will say something like the following until you connect with your phone:
  ```
  [00:01:54.027,752] <inf> main: ADC Thread sent: Ch0=3072 mV, Ch1=805 mV
  [00:01:54.027,777] <inf> main: BLE thread received: Ch0(BOOST)=3072 mV, Ch1(LDOLS)=805 mV
  [00:01:54.027,797] <wrn> main: Warning, notification not enabled for boost mv characteristic
  [00:01:54.027,812] <wrn> main: Warning, notification not enabled for lsldo mv characteristic
  ```
  Since we are not connected!
- Open nRF Connect mobile app on your iOS or Android device
- Press the dropdown for the scanner's Filter, enable 'name' filtering and enter `ZZZZZ`, where `ZZZZZ` is the unique identifier specific to you from step 1. _(Ultimately it does not matter what you choose for a name, just that it's unique, and is sufficiently short.)_
- Connect
  
  <img src="https://github.com/user-attachments/assets/bd2ee1c0-cce7-4da7-b2b3-fc5bf15bd5cd" width="25%">
- Navigate to the characteristics tab, enable notifications for the `757D0....` characteristic and the `B0057....` characteristic (Which are `LSLDO` and `BOOST` respectively) with the down arrow logo. _If you are on android, the equivalent button icon is the 3 down arrows._
- Change the number format with the `"` logo to uint32 (matching the data type of the helper functions).

> [!NOTE]
> If you are on android, long-tap the characteristic to change number format to a bit width integer matching your helper functions. If you cannot format to int on your android device, the byte numbers are read as little endian. i.e. `(0x)21-03` on the **757d011** characteristic translates to `0x321`, or decimal `801` mV.

  <img height="401" src="https://github.com/user-attachments/assets/48f599f9-a29f-4a21-ba30-d43d39dabb11" width="25%"> <img width="403" height="401" alt="image" src="https://github.com/user-attachments/assets/7234727b-488b-49f1-ba1b-3f6a91d05391" />


- Your phone should now be receiving the boost and ldo/ls regulator output voltages in mv, corresponding with your log and the gui! (Your log should also no longer be complaining about the lack of connection)

  <img src="https://github.com/user-attachments/assets/d5c6dda3-74a9-41f9-8b15-2b007f5d94b2" width="25%">

```
[00:02:09.030,876] <inf> main: ADC Thread sent: Ch0=3002 mV, Ch1=805 mV
[00:02:09.030,894] <inf> main: BLE thread received: Ch0(BOOST)=3002 mV, Ch1(LDOLS)=805 mV
``` 

### Move to the ble_soln branch if you are stuck and need a lift: [🫱LINK](https://github.com/droidecahedron/nrf_peripheral_dmm/tree/ble_soln)

## 🎊Congratulations! You are done!🎊
![image](https://github.com/user-attachments/assets/3dec1a74-baf5-4712-9472-629a97aa0c97) ![image](https://github.com/user-attachments/assets/5a8485f8-1c36-4582-bc08-2401e5cd4e3b)

> The nPM2100 is also a fuel gauge! The code of this workshop is very close to being able to give you a battery measurement. You can see an example of that [here](https://github.com/droidecahedron/npm2100_nrf54l15_BFG).
> 
> If you want to see some examples on reading i2c information, piping it out over BLE, and being a lower power device, check the following out: [🔗LINK](https://github.com/droidecahedron/i2c_ble_peripheral/tree/main)
>
> If you wish to see a SW example of multiple channel SAADC without as much CPU involvement using PPI, check the following out: [🔗LINK](https://github.com/droidecahedron/nrf_adcppimulti/tree/main)
 
