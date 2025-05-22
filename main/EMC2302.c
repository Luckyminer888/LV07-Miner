#include <stdio.h>
#include "esp_log.h"

#include "i2c_lucky.h"
#include "EMC2302.h" // 修改为EMC2302的头文件

static const char * TAG = "EMC2302"; // 更新为EMC2302

static i2c_master_dev_handle_t emc2302_dev_handle;

/**
 * @brief Initialize the EMC2302 sensor.
 *
 * @return esp_err_t ESP_OK on success, or an error code on failure.
 */
esp_err_t EMC2302_init(bool invertPolarity) {

    if (i2c_lucky_add_device(EMC2302_I2CADDR_DEFAULT, &emc2302_dev_handle) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to add device");
        return ESP_FAIL;
    }

    // 配置寄存器设置
    ESP_ERROR_CHECK(i2c_lucky_register_write_byte(emc2302_dev_handle, EMC2302_FAN1_CONFIG1, 0x00));
    ESP_ERROR_CHECK(i2c_lucky_register_write_byte(emc2302_dev_handle, EMC2302_FAN2_CONFIG1, 0x00));

    return ESP_OK;
}

// 设置风扇速度百分比
void EMC2302_set_fan_speed(uint8_t devicenum, float percent)
{
    // 计算 PWM 设置值 (假设 PWM 范围为 0-255)
    uint8_t pwm_set = (uint8_t)(255.0 * (percent / 100) + 0.5);

    // 计算 PWM 寄存器地址，假设偏移量是 0x10，确保与新版 EMC2302 的文档一致
    uint8_t PWM_REG = EMC2302_FAN1_SETTING + (devicenum * 0x10);

    ESP_LOGI(TAG, "Set Fan %d to %d%% PWM %d", devicenum, (int)percent, pwm_set);

    // 使用 ESP_ERROR_CHECK 和 i2c_lucky_register_write_byte 来写入 PWM 寄存器
    ESP_ERROR_CHECK(i2c_lucky_register_write_byte(emc2302_dev_handle, PWM_REG, pwm_set));
}


// 计算 RPM
uint16_t EMC2302_get_fan_speed(uint8_t devicenum)
{
    uint8_t tach_lsb, tach_msb;
    uint16_t tach_counts = 1;
    uint16_t rpm;
    uint8_t TACH_LSB_REG = EMC2302_TACH1_LSB + (devicenum * 0x10);
    uint8_t TACH_MSB_REG = EMC2302_TACH1_MSB + (devicenum * 0x10);

    int poles = 4;     // motor poles
    int edges = 5;     // motor edges
    int ftach = 32768; // tach clock frequency

    ESP_ERROR_CHECK(i2c_lucky_register_read(emc2302_dev_handle, TACH_MSB_REG, &tach_msb, 1));
    ESP_ERROR_CHECK(i2c_lucky_register_read(emc2302_dev_handle, TACH_LSB_REG, &tach_lsb, 1));
    // ESP_LOGI(TAG, "GET-Tach counts reg: %02x %02x", tach_msb, tach_lsb);

    if (tach_msb == 0xff) {
        rpm = 0;
    } else {
        tach_counts = (tach_msb << 5) + ((tach_lsb >> 3) & 0x1F);
        rpm = (uint16_t) ((edges - 1) / poles * (1.0f / tach_counts) * ftach * 60);
        //rpm = (uint16_t) (3 * (ftach / (tach_counts * poles)) * 60);
    }
    ESP_LOGI(TAG, "GET-Fan Speed[%d] %d = %d RPM, MSB 0x%02X", devicenum, tach_counts, rpm, tach_msb);

    return rpm;
}

float EMC2302_get_external_temp(void)
{
    return 0; // 视EMC2302是否支持外部温度测量
}

uint8_t EMC2302_get_internal_temp(void)
{
    return 0; // 视EMC2302是否支持内部温度测量
}
