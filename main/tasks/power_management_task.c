#include "EMC2101.h"
#include "EMC2302.h"
#include "TMP1075.h"
#include "INA260.h"
#include "bm1397.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "global_state.h"
#include "math.h"
#include "mining.h"
#include "nvs_config.h"
#include "serial.h"
#include "TPS546.h"
#include "vcore.h"
#include <string.h>

#define POLL_RATE 3000
#define MAX_TEMP 90.0
#define THROTTLE_TEMP 73.0
#define THROTTLE_TEMP_RANGE (MAX_TEMP - THROTTLE_TEMP)

#define VOLTAGE_START_THROTTLE 12100
#define VOLTAGE_MIN_THROTTLE 11500
#define VOLTAGE_RANGE (VOLTAGE_START_THROTTLE - VOLTAGE_MIN_THROTTLE)

#define TPS546_THROTTLE_TEMP 105.0
#define TPS546_MAX_TEMP 145.0

#define SUPRA_POWER_OFFSET 2
#define GAMMA_POWER_OFFSET 5

static const char * TAG = "power_management";

// static float _fbound(float value, float lower_bound, float upper_bound)
// {
//     if (value < lower_bound)
//         return lower_bound;
//     if (value > upper_bound)
//         return upper_bound;

//     return value;
// }

// Set the fan speed between 20% min and 100% max based on chip temperature as input.
// The fan speed increases from 20% to 100% proportionally to the temperature increase from 50 and THROTTLE_TEMP
static double automatic_fan_speed(float chip_temp, GlobalState * GLOBAL_STATE)
{
    double result = 0.0;
    double min_temp = 45.0;
    double min_fan_speed = 35.0;

    if (chip_temp < min_temp) {
        result = min_fan_speed;
    } else if (chip_temp >= 72) {
        result = 100;
    } else {
        double temp_range = 72 - min_temp;
        double fan_range = 100 - min_fan_speed;
        result = ((chip_temp - min_temp) / temp_range) * fan_range + min_fan_speed;
    }

    float perc = (float) result;
    GLOBAL_STATE->POWER_MANAGEMENT_MODULE.fan_perc = perc;
    EMC2302_set_fan_speed(0, perc);

    return result;
}

void POWER_MANAGEMENT_task(void * pvParameters)
{
    GlobalState * GLOBAL_STATE = (GlobalState *) pvParameters;

    PowerManagementModule * power_management = &GLOBAL_STATE->POWER_MANAGEMENT_MODULE;

    power_management->frequency_multiplier = 1;

    power_management->HAS_POWER_EN =
        GLOBAL_STATE->board_version == 202 || GLOBAL_STATE->board_version == 203 || GLOBAL_STATE->board_version == 204;
    power_management->HAS_PLUG_SENSE = GLOBAL_STATE->board_version == 204;

    // int last_frequency_increase = 0;
    uint16_t frequency_target = nvs_config_get_u16(NVS_CONFIG_ASIC_FREQ, CONFIG_ASIC_FREQUENCY);
    uint16_t voltage_target = nvs_config_get_u16(NVS_CONFIG_ASIC_VOLTAGE, CONFIG_ASIC_VOLTAGE);

    uint16_t auto_fan_speed = nvs_config_get_u16(NVS_CONFIG_AUTO_FAN_SPEED, 1);
    uint16_t change_fan_time = 0;

    vTaskDelay(500 / portTICK_PERIOD_MS);
    uint16_t last_core_voltage = 0.0;
    uint16_t last_asic_frequency = power_management->frequency_value;

    while (1) {

        float power0 = 0.0;

        TPS546_set_addr(TPS_DEV_1);
        float vout1 = TPS546_get_vout();
        float iout1 = TPS546_get_iout();
        float power1 = (vout1 * iout1) + SUPRA_POWER_OFFSET;
        ESP_LOGI(TAG, "24 vout: %.2f, iout: %.2f", vout1, iout1);

        // 计算总功率
        float total_power = power0 + power1;

        // 读取电压和电流
        float vin = TPS546_get_vin();

        power_management->voltage = vin * 1000;
        power_management->current = iout1 * 1000;
        // calculate regulator power (in milliwatts)
        power_management->power = total_power;

        power_management->fan_rpm = EMC2302_get_fan_speed(0);

        power_management->chip_temp_avg = GLOBAL_STATE->ASIC_initalized ? (float) TMP1075_read_temperature(1) : 0;
        ESP_LOGI(TAG, "Board Temp: %d, %d", TMP1075_read_temperature(0), TMP1075_read_temperature(1));
        power_management->vr_temp = (float) TPS546_get_temperature();

        TPS546_set_addr(TPS_DEV_1);
        ESP_LOGI(TAG, "TMP546_1 Temp: %d", TPS546_get_temperature());

        if (power_management->voltage < TPS546_INIT_VOUT_MIN) {
            break;
        }

        // overheat mode if the voltage regulator or ASIC is too hot
        if ((power_management->chip_temp_avg > THROTTLE_TEMP) &&
            (power_management->frequency_value > 50 || power_management->voltage > 1000)) {
            ESP_LOGE(TAG, "OVERHEAT! VR: %fC ASIC %fC", power_management->vr_temp, power_management->chip_temp_avg);

            if (frequency_target >= 575) {
                if (voltage_target >= 1250) {
                    nvs_config_set_u16(NVS_CONFIG_ASIC_FREQ, 575);
                    nvs_config_set_u16(NVS_CONFIG_ASIC_VOLTAGE, 1200);
                    exit(EXIT_FAILURE);
                } else {
                    if (power_management->chip_temp_avg > (THROTTLE_TEMP + 3)) {
                        nvs_config_set_u16(NVS_CONFIG_ASIC_FREQ, 550);
                        nvs_config_set_u16(NVS_CONFIG_ASIC_VOLTAGE, 1200);
                        exit(EXIT_FAILURE);
                    }
                }
            } else if (frequency_target >= 550) {
                if (power_management->chip_temp_avg > (THROTTLE_TEMP + 5)) {
                    nvs_config_set_u16(NVS_CONFIG_ASIC_VOLTAGE, 1200);
                    nvs_config_set_u16(NVS_CONFIG_ASIC_FREQ, 500);
                    exit(EXIT_FAILURE);
                }
            } else {
                // Turn off core voltage
                VCORE_set_voltage(0.0, GLOBAL_STATE);
                nvs_config_set_u16(NVS_CONFIG_ASIC_VOLTAGE, 1000);
                nvs_config_set_u16(NVS_CONFIG_ASIC_FREQ, 50);
                nvs_config_set_u16(NVS_CONFIG_FAN_SPEED, 100);
                nvs_config_set_u16(NVS_CONFIG_AUTO_FAN_SPEED, 0);
                nvs_config_set_u16(NVS_CONFIG_OVERHEAT_MODE, 1);
                exit(EXIT_FAILURE);
            }
        }

        if (auto_fan_speed == 1) {
            if (change_fan_time >= 5) {
                change_fan_time = 0;
                power_management->fan_perc = (float) automatic_fan_speed(power_management->chip_temp_avg, GLOBAL_STATE);
            } else {
                change_fan_time = change_fan_time + 1;
            }
        } else {
            if (change_fan_time == 0) {
                change_fan_time = 1;
                float fs = (float) nvs_config_get_u16(NVS_CONFIG_FAN_SPEED, 100);
                power_management->fan_perc = fs;
                EMC2302_set_fan_speed(0, (float) fs);
                EMC2302_set_fan_speed(1, (float) fs);
            }
        }

        // Read the state of GPIO12
        if (power_management->HAS_PLUG_SENSE) {
            int gpio12_state = gpio_get_level(GPIO_NUM_12);
            if (gpio12_state == 0) {
                // turn ASIC off
                gpio_set_level(GPIO_NUM_10, 1);
            }
        }

        // New voltage and frequency adjustment code
        uint16_t core_voltage = nvs_config_get_u16(NVS_CONFIG_ASIC_VOLTAGE, CONFIG_ASIC_VOLTAGE);
        uint16_t asic_frequency = nvs_config_get_u16(NVS_CONFIG_ASIC_FREQ, CONFIG_ASIC_FREQUENCY);

        if (core_voltage != last_core_voltage) {
            ESP_LOGI(TAG, "setting new vcore voltage to %umV", core_voltage);
            VCORE_set_voltage((double) core_voltage / 1000.0, GLOBAL_STATE);
            last_core_voltage = core_voltage;
        }

        if (asic_frequency != last_asic_frequency) {
            ESP_LOGI(TAG, "New ASIC frequency requested: %uMHz (current: %uMHz)", asic_frequency, last_asic_frequency);
            if (do_frequency_transition((float) asic_frequency)) {
                power_management->frequency_value = (float) asic_frequency;
                ESP_LOGI(TAG, "Successfully transitioned to new ASIC frequency: %uMHz", asic_frequency);
            } else {
                ESP_LOGE(TAG, "Failed to transition to new ASIC frequency: %uMHz", asic_frequency);
            }
            last_asic_frequency = asic_frequency;
        }

        vTaskDelay(POLL_RATE / portTICK_PERIOD_MS);
    }
}
