#include "remote.h"
#include "common.h"

extern uint16_t adc_val;

static uint16_t bt_read_data;

struct bt_conn *my_conn = NULL;

static struct felk_ble_cb ble_cb;

static bool indicate_enabled;
static bool notify_mysensor_enabled;
static struct bt_gatt_indicate_params ind_params;

typedef struct adv_mfg_data {
	uint16_t company_code;	    /* Company Identifier Code. */
	uint16_t adc_value;      /* Number of times Button 1 is pressed*/
} adv_mfg_data_type;

/* Declare and set manufacturing data
 first byte is manufacturing company ID & second byte is the data
 which gets updated in advertising data */
static adv_mfg_data_type adv_mfg_data = {COMPANY_ID_CODE,0x00};

// Scan response data byte wirh URI byte
static unsigned char url_data[] ={0x17,'/','/','a','c','a','d','e','m','y','.',
                                 'n','o','r','d','i','c','s','e','m','i','.',
                                 'c','o','m'};
/*
// Declaring and setting adv parameters
static struct bt_le_adv_param *adv_param =
	BT_LE_ADV_PARAM(BT_LE_ADV_OPT_NONE,
	800,
	801,
	NULL);
*/

// Declaring and setting adv parameters
static struct bt_le_adv_param *adv_param = BT_LE_ADV_PARAM((BT_LE_ADV_OPT_CONNECTABLE|BT_LE_ADV_OPT_USE_IDENTITY), 
                BT_GAP_ADV_FAST_INT_MIN_1, /*Min Advertising Interval 500ms (800*0.625ms) */
                BT_GAP_ADV_FAST_INT_MAX_1, /*Max Advertising Interval 500.625ms (801*0.625ms)*/
                NULL); /* Set to NULL for undirected advertising*/

// Advertising data declaration and setting
// static const struct bt_data ad[] = {
// 	/* STEP 4.1.2 - Set the advertising flags */
//     /* non-connectable */
//     BT_DATA_BYTES(BT_DATA_FLAGS, BT_LE_AD_NO_BREDR),
//     /* STEP 4.1.3 - Set the advertising packet data  */
//     BT_DATA(BT_DATA_NAME_COMPLETE, DEVICE_NAME, DEVICE_NAME_LEN),
//     BT_DATA(BT_DATA_MANUFACTURER_DATA,(unsigned char *)&adv_mfg_data, sizeof(adv_mfg_data)),
// };

// Advertising data declaration and setting
static const struct bt_data ad[] = {
	/* STEP 4.1.2 - Set the advertising flags */
    //BT_DATA_BYTES(BT_DATA_FLAGS, BT_LE_AD_NO_BREDR),
    BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
	/* STEP 4.1.3 - Set the advertising packet data  */
    BT_DATA(BT_DATA_NAME_COMPLETE, DEVICE_NAME, DEVICE_NAME_LEN),
    BT_DATA(BT_DATA_MANUFACTURER_DATA,(unsigned char *)&adv_mfg_data, sizeof(adv_mfg_data)),
};


// Scan response data declaration and setting
// static const struct bt_data sd[] = {
//         /* 4.2.3 Include the URL data in the scan response packet*/
//     BT_DATA(BT_DATA_URI, url_data,sizeof(url_data)),
// };

// Scan response data declaration and setting
static const struct bt_data sd[] = {
        /* 4.2.3 Include the URL data in the scan response packet*/
    BT_DATA_BYTES(BT_DATA_UUID128_ALL, BT_UUID_128_ENCODE(0x00001523, 0x1212, 0xefde, 0x1523, 0x785feabcd123)),
    //BT_DATA(BT_DATA_URI, url_data,sizeof(url_data)),
};

void on_connected(struct bt_conn *conn, uint8_t err)
{
    if (err) {
		printk("Connection error %d\n", err);
        return err;
    }
	printk("BLE Connected\n");
    my_conn = bt_conn_ref(conn);
}

void on_disconnected(struct bt_conn *conn, uint8_t reason)
{
	printk("BLE Disconnected. Reason %d\n", reason);
    bt_conn_unref(my_conn);

}

struct bt_conn_cb connection_callbacks = {
    .connected              = on_connected,
    .disconnected           = on_disconnected,
};

static ssize_t read_value(struct bt_conn *conn,
			  const struct bt_gatt_attr *attr,
			  void *buf,
			  uint16_t len,
			  uint16_t offset)
{
	//get a pointer to bt_read_data which is passed in the BT_GATT_CHARACTERISTIC() and stored in attr->user_data
	const uint16_t *value = attr->user_data;

	printk("Attribute read, handle: %u, conn: %p \n", attr->handle, (void *)conn);

    if (ble_cb.data_cb) {
		// Call the application callback function to update the get the current value of the button
		bt_read_data = 0xf0f0;//ble_cb.data_cb;//read_adc();
		return bt_gatt_attr_read(conn, attr, buf, len, offset, value,
					 sizeof(*value));
	}

	return 0;
}

static ssize_t cmd_value(struct bt_conn *conn,
			 const struct bt_gatt_attr *attr,
			 const void *buf,
			 uint16_t len, uint16_t offset, uint8_t flags)
{
	printk("Attribute write, handle: %u, conn: %p \n", attr->handle, (void *)conn);

	if (len != 1U) {
		printk("Write cmd: Incorrect data length\n");
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	}

	if (offset != 0) {
		printk("Write cmd: Incorrect data offset\n");
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	}

	if (ble_cb.cmd_cb) {
		//Read the received value 
		uint8_t val = *((uint8_t *)buf);

		if (val) {
			//Call the application callback function to execute cmd
			ble_cb.cmd_cb(val);
		} else {
			printk("Write led: Incorrect value\n");
			return BT_GATT_ERR(BT_ATT_ERR_VALUE_NOT_ALLOWED);
		}
	}

	return len;
}

static void felk_ccc_sensor_cfg_changed(const struct bt_gatt_attr *attr,
				  uint16_t value)
{
	notify_mysensor_enabled = (value == BT_GATT_CCC_NOTIFY);
}

static void read_ccc_cfg_changed(const struct bt_gatt_attr *attr,
				  uint16_t value)
{
	indicate_enabled = (value == BT_GATT_CCC_INDICATE);
}

static void indicate_cb(struct bt_conn *conn, struct bt_gatt_indicate_params *params, uint8_t err)
{
	printk("Indication %s\n", err != 0U ? "fail" : "success");
}

/* A function to register application callbacks for the LED and Button characteristics  */
int felk_ble_init(struct felk_ble_cb *callbacks)
{
	if (callbacks) {
		ble_cb.cmd_cb = callbacks->cmd_cb;
		ble_cb.data_cb = callbacks->data_cb;
	}

	return 0;
}

int bluetooth_init(void)
{
    int err;

    bt_addr_le_t addr;
    err = bt_addr_le_from_str("FF:EE:DD:CC:BB:AA", "random", &addr);
    if (err) {
		printk("Error: Invalid BT address (err %d)\n", err);
        return err;
    }

    err = bt_id_create(&addr, NULL);
    if (err < 0) {
		printk("Error: Creating new ID failed (err %d)\n", err);
        return err;
    }

    err = bt_enable(NULL);
    if (err) {
		printk("Error: Enabling BT!\r\n");
        return err;
    }
    
    err = bt_le_adv_start(adv_param, ad, ARRAY_SIZE(ad), sd, ARRAY_SIZE(sd));
    if (err) {
		printk("Error: Starting BLE advertisement!\r\n");
        return err;
    }

    bt_conn_cb_register(&connection_callbacks);

    return 0;
}

void bt_update_ad(void)
{
    //adv_mfg_data.number_press += 1;
    adv_mfg_data.adc_value = 0xf0f0;//adc_val;
    bt_le_adv_update_data(ad, ARRAY_SIZE(ad), sd, ARRAY_SIZE(sd));
}

BT_GATT_SERVICE_DEFINE(felk_ble_svc,
BT_GATT_PRIMARY_SERVICE(BT_UUID_FELK),
/* Characteristic for reading ADC value */
	BT_GATT_CHARACTERISTIC(BT_UUID_FELK_TX,
			       BT_GATT_CHRC_READ | BT_GATT_CHRC_INDICATE,
			       BT_GATT_PERM_READ, read_value, NULL,
			       &bt_read_data),
/* Client Characteristic Configurator Declaration */

	BT_GATT_CCC(read_ccc_cfg_changed,
					BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),
/* Characteristic for command reception */
	BT_GATT_CHARACTERISTIC(BT_UUID_FELK_RX,
			       BT_GATT_CHRC_WRITE,
			       BT_GATT_PERM_WRITE,
			       NULL, cmd_value, NULL),
	BT_GATT_CHARACTERISTIC(BT_UUID_FELK_SENSOR,
			       BT_GATT_CHRC_NOTIFY,
			       BT_GATT_PERM_NONE, NULL, NULL,
			       NULL),

	BT_GATT_CCC(felk_ccc_sensor_cfg_changed,
		    BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),
				   
);

int read_val_indicate(uint16_t val)
{
	if (!indicate_enabled) {
		return -EACCES;
	}

	ind_params.attr = &felk_ble_svc.attrs[2];
	ind_params.func = indicate_cb;
	ind_params.destroy = NULL;
	ind_params.data = &val;
	ind_params.len = sizeof(val);
	return bt_gatt_indicate(NULL, &ind_params);

}

int felk_send_sensor_notify(uint16_t sensor_value)
{
	if (!notify_mysensor_enabled) {
		return -EACCES;
	}

	return bt_gatt_notify(NULL, &felk_ble_svc.attrs[7], 
			      &sensor_value,
			      sizeof(sensor_value));
}
