#include <stdio.h>
#include "esp_log.h"
#include "i2c_lucky.h"

#include "TMP1075.h"

// static const char *TAG = "TMP1075.c";

static i2c_master_dev_handle_t tmp1075_dev_handle_1;
static i2c_master_dev_handle_t tmp1075_dev_handle_2;

/**
 * @brief Initialize the TMP1075 sensor.
 *
 * @return esp_err_t ESP_OK on success, or an error code on failure.
 */
esp_err_t TMP1075_init(void) {
    esp_err_t result;
    result = i2c_lucky_add_device(TMP1075_I2CADDR_DEFAULT, &tmp1075_dev_handle_1);
    if (result != ESP_OK) {
        return result;
    }

    result = i2c_lucky_add_device(TMP1075_I2CADDR_DEFAULT + 1, &tmp1075_dev_handle_2);
    return result;
}


bool TMP1075_installed(int device_index)
{
    uint8_t data[2];
    esp_err_t result;
    i2c_master_dev_handle_t handle = (device_index == 0) ? tmp1075_dev_handle_1 : tmp1075_dev_handle_2;

    result = i2c_lucky_register_read(handle, TMP1075_CONFIG_REG, data, 2);

    return (result == ESP_OK ? true : false);
}

uint8_t TMP1075_read_temperature(int device_index)
{
    uint8_t data[2];
    i2c_master_dev_handle_t handle = (device_index == 0) ? tmp1075_dev_handle_1 : tmp1075_dev_handle_2;

    ESP_ERROR_CHECK(i2c_lucky_register_read(handle, TMP1075_TEMP_REG, data, 2));
    return data[0];
}

