#include <stdio.h>
#include <math.h>
#include <string.h>
#include "esp_log.h"

#include "vcore.h"
#include "adc.h"
#include "TPS546.h"

#define TPS40305_VFB 0.6

// DS4432U Transfer function constants for Bitaxe board
// #define BITAXE_RFS 80000.0     // R16
// #define BITAXE_IFS ((DS4432_VRFS * 127.0) / (BITAXE_RFS * 16))
#define BITAXE_IFS 0.000098921 // (Vrfs / Rfs) x (127/16)  -> Vrfs = 0.997, Rfs = 80000
#define BITAXE_RA 4750.0       // R14
#define BITAXE_RB 3320.0       // R15
#define BITAXE_VNOM 1.451   // this is with the current DAC set to 0. Should be pretty close to (VFB*(RA+RB))/RB
#define BITAXE_VMAX 2.39
#define BITAXE_VMIN 0.046

static const char *TAG = "vcore.c";

uint8_t VCORE_init(GlobalState * global_state) {
    uint8_t result = 0;
    TPS546_set_addr(TPS_DEV_1);
    TPS546_init();
    return result;
}

bool VCORE_set_voltage(float core_voltage, GlobalState * global_state)
{
    ESP_LOGI(TAG, "Set ASIC voltage = %.3fV", core_voltage);
    float want_vcore = core_voltage * (float)global_state->voltage_domain;
    TPS546_set_addr(TPS_DEV_1);
    TPS546_set_vout(want_vcore);
    return true;
}

uint16_t VCORE_get_voltage_mv(GlobalState * global_state) {
    return ADC_get_vcore() / global_state->voltage_domain;
}
