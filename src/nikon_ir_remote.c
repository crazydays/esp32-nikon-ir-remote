#include <nimble/nimble_port_freertos.h>

#include "nikon_ir_remote.h"

#include "nir_nvs.h"
#include "nir_ble.h"
#include "nir_timer.h"

static const char* _nir_enabled_key = "nir_enabled";
static const char* _nir_delayms_key = "nir_delayms";

bool _nir_enabled = 0;
uint16_t _nir_delayms = 10000;

void _nir_init_timer(void);
void _nir_init_ble(void);
void _nir_init_application_state(void);

uint64_t _ms_to_us(uint16_t ms);

void nir_init(void) {
    _nir_init_timer();
    _nir_init_ble();
    _nir_init_application_state();
}

void _nir_init_timer(void) {
    nir_timer_init();
}

void _nir_init_ble(void) {
    nir_ble_init();
    nimble_port_freertos_init(nir_ble_host_task);
}

void _nir_init_application_state(void) {
    nir_init_nvs();

    nir_set_enabled(nir_nvs_read_bool(_nir_enabled_key, false));
    nir_set_delayms(nir_nvs_read_uint16(_nir_delayms_key, 10000));
}

bool nir_get_enabled(void) {
    return _nir_enabled;
}

void nir_set_enabled(bool enabled) {
    ESP_LOGI(TAG, "nir_set_enabled(%d): %d", _nir_enabled, enabled);

    if (_nir_enabled == enabled) {
        return; // nothing to do
    }

    // update timer state
    if (_nir_enabled) {
        nir_timer_stop();
    } else {
        nir_timer_start(_ms_to_us(_nir_delayms));
    }

    // current state
    _nir_enabled = enabled;

    // store state
    nir_nvs_write_bool(_nir_enabled_key, _nir_enabled);
}

inline uint64_t _ms_to_us(uint16_t ms) {
    return ms * 1000;
}

uint16_t nir_get_delayms(void) {
    return _nir_delayms;
}

void nir_set_delayms(uint16_t delayms) {
    ESP_LOGI(TAG, "nir_set_delayms(%u): %u", _nir_delayms, delayms);

    if (_nir_delayms == delayms) {
        return; // nothing to do
    }

    bool enabled = _nir_enabled;

    // stop if originally enabled
    if (enabled) {
        nir_timer_stop();
    }

    // update current state
    _nir_delayms = delayms;

    // start if originally enabled
    if (enabled) {
        nir_timer_start(_ms_to_us(_nir_delayms));
    }

    // store state
    nir_nvs_write_uint16(_nir_delayms_key, _nir_delayms);
}
