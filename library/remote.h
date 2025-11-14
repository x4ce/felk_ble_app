#include <zephyr/kernel.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/gap.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/addr.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/types.h>

#define DEVICE_NAME CONFIG_BT_DEVICE_NAME
#define DEVICE_NAME_LEN (sizeof(DEVICE_NAME) - 1)

#define COMPANY_ID_CODE            0x0059 // Nordic Semiconductor company

#define BT_UUID_FELK_VAL \
	BT_UUID_128_ENCODE(0x5337b1b1, 0x0e10, 0x47d7, 0x868e, 0x7aae0e5b45e1)

/** @brief TX Characteristic UUID. */
#define BT_UUID_FELK_TX_VAL \
	BT_UUID_128_ENCODE(0x5337b1b2, 0x0e10, 0x47d7, 0x868e, 0x7aae0e5b45e1)

/** @brief RX Characteristic UUID. */
#define BT_UUID_FELK_RX_VAL \
	BT_UUID_128_ENCODE(0x5337b1b3, 0x0e10, 0x47d7, 0x868e, 0x7aae0e5b45e1)

/** @brief ST Characteristic UUID. */
#define BT_UUID_FELK_ST_VAL \
	BT_UUID_128_ENCODE(0x5337b1b4, 0x0e10, 0x47d7, 0x868e, 0x7aae0e5b45e1)

/** @brief SEN Characteristic UUID. */
#define BT_UUID_FELK_SENSOR_VAL \
	BT_UUID_128_ENCODE(0x5337b1b5, 0x0e10, 0x47d7, 0x868e, 0x7aae0e5b45e1)

/** @brief SENS Characteristic UUID. */
#define BT_UUID_FELK_SENSORS_VAL \
BT_UUID_128_ENCODE(0x5337b1b6, 0x0e10, 0x47d7, 0x868e, 0x7aae0e5b45e1)

/** @brief CR Characteristic UUID. */
#define BT_UUID_FELK_CR_VAL \
	BT_UUID_128_ENCODE(0x5337b1b7, 0x0e10, 0x47d7, 0x868e, 0x7aae0e5b45e1)

/** @brief BLDC CMD (RX) Characteristic UUID. */
#define BT_UUID_FELK_BLDC_RX_VAL \
	BT_UUID_128_ENCODE(0x5337b1b8, 0x0e10, 0x47d7, 0x868e, 0x7aae0e5b45e1)

/** @brief BLDC DATA (TX) Characteristic UUID. */
#define BT_UUID_FELK_BLDC_TX_VAL \
	BT_UUID_128_ENCODE(0x5337b1b9, 0x0e10, 0x47d7, 0x868e, 0x7aae0e5b45e1)

#define BT_UUID_FELK				BT_UUID_DECLARE_128(BT_UUID_FELK_VAL)
#define BT_UUID_FELK_TX     		BT_UUID_DECLARE_128(BT_UUID_FELK_TX_VAL)
#define BT_UUID_FELK_RX     		BT_UUID_DECLARE_128(BT_UUID_FELK_RX_VAL)
#define BT_UUID_FELK_ST     		BT_UUID_DECLARE_128(BT_UUID_FELK_ST_VAL)
#define BT_UUID_FELK_SENSOR			BT_UUID_DECLARE_128(BT_UUID_FELK_SENSOR_VAL)
#define BT_UUID_FELK_SENSORS		BT_UUID_DECLARE_128(BT_UUID_FELK_SENSORS_VAL)
#define BT_UUID_FELK_CR     		BT_UUID_DECLARE_128(BT_UUID_FELK_CR_VAL)
#define BT_UUID_FELK_BLDC_RX     	BT_UUID_DECLARE_128(BT_UUID_FELK_BLDC_RX_VAL)
#define BT_UUID_FELK_BLDC_TX     	BT_UUID_DECLARE_128(BT_UUID_FELK_BLDC_TX_VAL)

/** @brief Callback type for when an byte is received. */
typedef void (*cmd_cb_t)(const uint8_t cmd);

/** @brief Callback type for when an 16 bit CR value is received. */
typedef void (*cr_cb_t)(const uint16_t crval);

/** @brief Callback type for when the data is pulled. */
typedef uint16_t (*data_cb_t)(void);

/** @brief Callback type for when the data is pulled. */
typedef uint16_t (*status_cb_t)(void);

/** @brief Callback type for when an BLDC byte is received. */
typedef void (*bldc_rx_cb_t)(const uint8_t cmd);

/** @brief Callback type for when the BLDC data is pulled. */
typedef uint16_t (*bldc_data_cb_t)(void);

/** @brief Callback struct used by the Felk BLE Service. */
struct felk_ble_cb {
	/** cmd callback. */
	cmd_cb_t cmd_cb;
	/** cmd callback. */
	cr_cb_t cr_cb;
	/** read data callback. */
	data_cb_t data_cb;
	/** read status callback. */
	status_cb_t status_cb;
	/** cmd callback. */
	bldc_rx_cb_t bldc_rx_cb;
	/** read status callback. */
	bldc_data_cb_t bldc_data_cb;
};

/** @brief Initialize the LBS Service.
 *
 * This function registers application callback functions with the My LBS
 * Service
 *
 * @param[in] callbacks Struct containing pointers to callback functions
 *			used by the service. This pointer can be NULL
 *			if no callback functions are defined.
 *
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a (negative) error code is returned.
 */
int felk_ble_init(struct felk_ble_cb *callbacks);

int bluetooth_init(void);

void bt_update_ad(void);

int read_val_indicate(uint16_t val);

int felk_send_sensor_notify(uint16_t sensor_value);