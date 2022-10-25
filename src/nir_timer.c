#include <sys/time.h>

#include "nir_timer.h"

/// 38khz
#define MODULATING_RATE ((uint64_t) (1000000 / 38000))

/// 5 seconds
#define START_DELAY (5000000)

static void _nir_modulate_pulse(void* args);

static void _nir_pulse_on(void* arg);
static void _nir_pulse_off(void* arg);
static void _nir_trigger(void* arg);

inline void _nir_update_led(void);

typedef struct {
    esp_timer_cb_t callback;
    void* arg;
    uint64_t delayus;
} _pulse_t;

volatile uint32_t _pulse_index = 0;

static _pulse_t _pulse_definitions[8] = { {
        .callback = _nir_pulse_on,
        .arg = NULL,
        .delayus = (2000 - 10)
    }, {
        .callback = _nir_pulse_off,
        .arg = NULL,
        .delayus = (27830 - 10)
    }, {
        .callback = _nir_pulse_on,
        .arg = NULL,
        .delayus = (400 - 10)
    }, {
        .callback = _nir_pulse_off,
        .arg = NULL,
        .delayus = (1500 - 10)
    }, {
        .callback = _nir_pulse_on,
        .arg = NULL,
        .delayus = (400 - 10)
    }, {
        .callback = _nir_pulse_off,
        .arg = NULL,
        .delayus = (3500 - 10)
    }, {
        .callback = _nir_pulse_on,
        .arg = NULL,
        .delayus = (400 - 10)
    }, {
        .callback = _nir_pulse_off,
        .arg = NULL,
        .delayus = 0
    }
};

static esp_timer_handle_t _modulating_timer;
static esp_timer_handle_t _pulse_timer;

static esp_timer_create_args_t _modulating_timer_args = {
    .name = "modulating_timer",
    .callback = _nir_modulate_pulse
};

static esp_timer_create_args_t _pulse_timer_args = {
    .name = "pulse_timer",
    .callback = _nir_trigger
};

void nir_timer_init(void) {
    gpio_pad_select_gpio(LED_PIN);
    gpio_set_direction(LED_PIN, GPIO_MODE_OUTPUT);

    ESP_ERROR_CHECK(esp_timer_create(&_modulating_timer_args, &_modulating_timer));
    ESP_ERROR_CHECK(esp_timer_create(&_pulse_timer_args, &_pulse_timer));
}

volatile bool _pulse_state = false;
volatile bool _led_state = false;

static void _nir_modulate_pulse(void* args) {
    _pulse_state = !_pulse_state;

    _nir_update_led();
}

static void _nir_pulse_on(void* args) {
    _led_state = true;
}

static void _nir_pulse_off(void* args) {
    _led_state = false;
}

inline void _nir_update_led(void) {
    gpio_set_level(LED_PIN, _pulse_state && _led_state);
}

static void _nir_trigger(void* arg) {
    if (!_pulse_index) {
    }
    uint32_t index = _pulse_index;
    _pulse_index = (_pulse_index + 1) % 8;

    _pulse_definitions[index].callback(_pulse_definitions[index].arg);
    ESP_ERROR_CHECK(esp_timer_start_once(_pulse_timer, _pulse_definitions[index].delayus));
}

void nir_timer_start(uint64_t delayus) {
    ESP_LOGI(TAG, "nir_timer_start delayus: %llu", delayus);
    ESP_LOGI(TAG, "nir_timer_start modulating_rate: %llu", MODULATING_RATE);

    // set delayus on last step and reset to first step
    _pulse_definitions[7].delayus = delayus;
    _pulse_index = 0;

    ESP_ERROR_CHECK(esp_timer_start_periodic(_modulating_timer, MODULATING_RATE));
    ESP_ERROR_CHECK(esp_timer_start_once(_pulse_timer, START_DELAY));
}

void nir_timer_stop(void) {
    ESP_ERROR_CHECK(esp_timer_stop(_modulating_timer));
    ESP_ERROR_CHECK(esp_timer_stop(_pulse_timer));
}

