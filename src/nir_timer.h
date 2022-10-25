#include <driver/gpio.h>
#include <driver/timer.h>
#include <esp_system.h>
#include <esp_timer.h>
#include <esp_log.h>

#include "nikon_ir_remote.h"

#ifndef NIR_TIMER_H
#define NIR_TIMER_H

extern const char *TAG;

void nir_timer_init(void);
void nir_timer_start(uint64_t delayus);
void nir_timer_stop(void);

#endif // NIR_TIMER_H
