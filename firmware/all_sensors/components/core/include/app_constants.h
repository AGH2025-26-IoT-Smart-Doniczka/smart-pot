#ifndef APP_CONSTANTS_H
#define APP_CONSTANTS_H

#include <stdint.h>

// NVS Config
#define NVS_SENSOR_SAMPLES_N 32

// BLE Service UUID
#define BLE_SERVICE_UUID "a2909447-7a7f-d8b8-d140-68a237aa735c"

// BLE Characteristic UUIDs
#define BLE_CHAR_PASSWORD_UUID "ee0bfcd4-bdce-6898-0c4a-72321e3c6f45"
#define BLE_CHAR_SSID_UUID "88971243-7282-049b-d14a-e951055fc3a3"
#define BLE_CHAR_CONFIG_UUID "2934e2ce-26f9-4705-bd1d-dfb343f63d04"
#define BLE_CHAR_USER_ID_UUID "752ff574-058c-4ba3-8310-b6daa639ee4d"
#define BLE_CHAR_MQTT_PASS_UUID "5befb657-9ba7-4f37-8954-d8fc9ca0346c"

// Configuration limits
#define MAX_SSID_LENGTH 31
#define MAX_PASSWD_LENGTH 63
#define MAX_POT_ID_LENGTH 6

// Sensor thresholds
#define TEMP_THRESHOLD_COUNT 2
#define MOISTURE_THRESHOLD_COUNT 4

#endif // APP_CONSTANTS_H
