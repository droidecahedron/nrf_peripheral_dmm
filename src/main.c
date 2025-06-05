#include <dk_buttons_and_leds.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/settings/settings.h>

// add a log module to see outputs
LOG_MODULE_REGISTER(main, LOG_LEVEL_INF);

#define DK_STATUS_LED DK_LED1
#define BLE_STATE_LED DK_LED2

// create message queue between adc thread and ble write thread.
// can also use zbus, or pass with work container w/ kernel work item.
struct adc_sample_msg {
  int32_t channel_mv[2];
};
K_MSGQ_DEFINE(adc_msgq, sizeof(struct adc_sample_msg), 8,
              4); // 8 messages, 4-byte alignment

//~~~~ BLE ~~~~//
// Declaration of custom GATT service and characteristics UUIDs
#define PMIC_HUB_SERVICE_UUID                                                  \
  BT_UUID_128_ENCODE(0x69e5204b, 0x8445, 0x5fca, 0xb332, 0xc13064b9dea2)
#define BOOST_MV_CHARACTERISTIC_UUID                                           \
  BT_UUID_128_ENCODE(0xB0057000, 0x7ea8, 0x4008, 0xb432, 0xb46096c049ba)
#define LSLDO_MV_CHARACTERISTIC_UUID                                           \
  BT_UUID_128_ENCODE(0x757D0111, 0xd8ee, 0x4faf, 0x956b, 0xafb01c17d0be)

#define BT_UUID_PMIC_HUB BT_UUID_DECLARE_128(PMIC_HUB_SERVICE_UUID)
#define BT_UUID_PMIC_HUB_BOOST_MV                                              \
  BT_UUID_DECLARE_128(BOOST_MV_CHARACTERISTIC_UUID)
#define BT_UUID_PMIC_HUB_LSLDO_MV                                              \
  BT_UUID_DECLARE_128(LSLDO_MV_CHARACTERISTIC_UUID)

#define DEVICE_NAME CONFIG_BT_DEVICE_NAME // from prj.conf
#define DEVICE_NAME_LEN (sizeof(DEVICE_NAME) - 1)

#define BLE_NOTIFY_INTERVAL K_MSEC(500)
#define BLE_THREAD_STACK_SIZE 1024
#define BLE_THREAD_PRIORITY 5

static const struct bt_le_adv_param *adv_param = BT_LE_ADV_PARAM(
    (BT_LE_ADV_OPT_CONN |
     BT_LE_ADV_OPT_USE_IDENTITY), /* Connectable advertising and use
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

/*This function is called whenever the Client Characteristic Control Descriptor
(CCCD) has been changed by the GATT client, for each of the characteristics*/
static void on_cccd_changed(const struct bt_gatt_attr *attr, uint16_t value) {
  ARG_UNUSED(attr);
  switch (value) {
  case BT_GATT_CCC_NOTIFY:
    break;
  case 0:
    break;
  default:
    LOG_ERR("Error, CCCD has been set to an invalid value");
  }
}

BT_GATT_SERVICE_DEFINE(
    pmic_hub, BT_GATT_PRIMARY_SERVICE(BT_UUID_PMIC_HUB),

    BT_GATT_CHARACTERISTIC(BT_UUID_PMIC_HUB_BOOST_MV, BT_GATT_CHRC_NOTIFY,
                           BT_GATT_PERM_READ, NULL, NULL, NULL),
    BT_GATT_CCC(on_cccd_changed, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),

    BT_GATT_CHARACTERISTIC(BT_UUID_PMIC_HUB_LSLDO_MV, BT_GATT_CHRC_NOTIFY,
                           BT_GATT_PERM_READ, NULL, NULL, NULL),
    BT_GATT_CCC(on_cccd_changed, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE), );

// BT globals and callbacks
struct bt_conn *m_connection_handle = NULL;
static void adv_work_handler(struct k_work *work) {
  int err = bt_le_adv_start(adv_param, ad, ARRAY_SIZE(ad), sd, ARRAY_SIZE(sd));
  if (err) {
    LOG_INF("Advertising failed to start (err %d)", err);
    return;
  }

  LOG_INF("Advertising successfully started");
}

static void advertising_start(void) { k_work_submit(&adv_work); }

static void recycled_cb(void) {
  LOG_INF("Connection object available from previous conn. Disconnect is "
          "complete!");
  advertising_start();
}

static void connected(struct bt_conn *conn, uint8_t err) {
  if (err) {
    LOG_WRN("Connection failed (err %u)", err);
    return;
  }
  m_connection_handle = conn;
  LOG_INF("Connected");
  dk_set_led_on(BLE_STATE_LED);
}

static void disconnected(struct bt_conn *conn, uint8_t reason) {
  LOG_INF("Disconnected (reason %u)", reason);
  m_connection_handle = NULL;
  dk_set_led_off(BLE_STATE_LED);
}

struct bt_conn_cb connection_callbacks = {
    .connected = connected,
    .disconnected = disconnected,
    .recycled = recycled_cb,
};

static void ble_report_boost_mv(struct bt_conn *conn, const uint32_t *data,
                                uint16_t len) {
  const struct bt_gatt_attr *attr = &pmic_hub.attrs[2];
  struct bt_gatt_notify_params params = {.uuid = BT_UUID_PMIC_HUB_BOOST_MV,
                                         .attr = attr,
                                         .data = data,
                                         .len = len,
                                         .func = NULL};

  if (bt_gatt_is_subscribed(conn, attr, BT_GATT_CCC_NOTIFY)) {
    if (bt_gatt_notify_cb(conn, &params)) {
      LOG_ERR("Error, unable to send notification");
    }
  } else {
    LOG_WRN("Warning, notification not enabled for boost mv characteristic");
  }
}

static void ble_report_lsldo_mv(struct bt_conn *conn, const uint32_t *data,
                                uint16_t len) {
  const struct bt_gatt_attr *attr = &pmic_hub.attrs[5];

  struct bt_gatt_notify_params params = {.uuid = BT_UUID_PMIC_HUB_LSLDO_MV,
                                         .attr = attr,
                                         .data = data,
                                         .len = len,
                                         .func = NULL};

  if (bt_gatt_is_subscribed(conn, attr, BT_GATT_CCC_NOTIFY)) {
    // Send the notification
    if (bt_gatt_notify_cb(conn, &params)) {
      LOG_ERR("Error, unable to send notification");
    }
  } else {
    LOG_WRN("Warning, notification not enabled for lsldo mv characteristic");
  }
}

void ble_write_thread(void) {
  struct adc_sample_msg msg;
  for (;;) {
    // Wait indefinitely for a new ADC sample message from the queue
    k_msgq_get(&adc_msgq, &msg, K_FOREVER);
    // At this point, msg.channel_mv[0] and msg.channel_mv[1] contain the
    // latest ADC results
    LOG_INF("BLE thread received: Ch0(BOOST)=%d mV, Ch1(LDOLS)=%d mV",
            msg.channel_mv[0], msg.channel_mv[1]);

    if (m_connection_handle) // if ble connection present
    {
      ble_report_boost_mv(m_connection_handle, &msg.channel_mv[0],
                          sizeof(msg.channel_mv[0]));
      ble_report_lsldo_mv(m_connection_handle, &msg.channel_mv[1],
                          sizeof(msg.channel_mv[1]));
    } else {
      LOG_INF("BLE Thread does not detect an active BLE connection");
    }

    k_sleep(BLE_NOTIFY_INTERVAL);
  }
}

//~~~~ ADC ~~~~//
#define ADC_THREAD_STACK_SIZE 1024
#define ADC_THREAD_PRIORITY 5
#define ADC_SAMPLE_INTERVAL K_SECONDS(1)

// define variable for each channel, get ADC channel specs from devicetree
#define DT_SPEC_AND_COMMA(node_id, prop, idx)                                  \
  ADC_DT_SPEC_GET_BY_IDX(node_id, idx),

static const struct adc_dt_spec adc_channels[] = {
    DT_FOREACH_PROP_ELEM(DT_PATH(zephyr_user), io_channels, DT_SPEC_AND_COMMA)};

// Task dedicated to sampling the ADC
void adc_sample_thread(void) {
  int err;
  int16_t buf[2];
  struct adc_sequence sequence = {
      .channels =
          BIT(adc_channels[0].channel_id) | BIT(adc_channels[1].channel_id),
      .buffer = buf,
      .buffer_size = sizeof(buf),
      .resolution = 14,
      .calibrate = true,
  };
  struct adc_sample_msg msg;

  // Setup each channel
  for (size_t i = 0; i < ARRAY_SIZE(adc_channels); i++) {
    if (!adc_is_ready_dt(&adc_channels[i])) {
      LOG_ERR("ADC controller device %s not ready", adc_channels[i].dev->name);
      return;
    }
    err = adc_channel_setup_dt(&adc_channels[i]);
    if (err < 0) {
      LOG_ERR("Could not setup channel #%d (%d)", i, err);
      return;
    }
  }

  for (;;) {
    err = adc_read(adc_channels->dev, &sequence);
    if (err < 0) {
      LOG_ERR("Could not read both channels (%d)", err);
    } else {
      // Convert raw values to mV for both channels
      for (size_t i = 0; i < ARRAY_SIZE(adc_channels); i++) {
        int32_t val_mv =
            (int32_t)buf[i]; // You can check buf[i] for a raw sample.
        err = adc_raw_to_millivolts_dt(&adc_channels[i], &val_mv);
        msg.channel_mv[i] = (err < 0) ? -1 : val_mv;
      }
    }
    LOG_INF("ADC Thread sent: Ch0=%d mV, Ch1=%d mV", msg.channel_mv[0],
            msg.channel_mv[1]);
    k_msgq_put(&adc_msgq, &msg, K_FOREVER);
    k_sleep(ADC_SAMPLE_INTERVAL);
  }
}

// main thread
int main(void) {
  int err;
  int blink = 0;

  err = dk_leds_init();
  if (err) {
    LOG_ERR("LEDs init failed (err %d)", err);
    return -1;
  }

  // Setting up Bluetooth
  err = bt_enable(NULL);
  if (err) {
    LOG_ERR("Bluetooth init failed (err %d)", err);
    return -1;
  }
  LOG_INF("Bluetooth initialized");
  if (IS_ENABLED(CONFIG_SETTINGS)) {
    settings_load();
  }
  bt_conn_cb_register(&connection_callbacks);
  k_work_init(&adv_work, adv_work_handler);
  advertising_start();

  for (;;) {
    dk_set_led(DK_STATUS_LED, (++blink) % 2);
    k_sleep(K_MSEC(2000));
  }
  return 0;
}

// add a thread for SAADC sampling
K_THREAD_DEFINE(adc_sample_thread_id, ADC_THREAD_STACK_SIZE, adc_sample_thread,
                NULL, NULL, NULL, ADC_THREAD_PRIORITY, 0, 0);
// BLE thread for providing data to notify the bluetooth service
K_THREAD_DEFINE(ble_write_thread_id, BLE_THREAD_STACK_SIZE, ble_write_thread,
                NULL, NULL, NULL, BLE_THREAD_PRIORITY, 0, 0);
