#include <stdbool.h>
#include <stdint.h>

#ifndef NIKON_IR_REMOTE_H
#define NIKON_IR_REMOTE_H

#define LED_PIN 1

void nir_init(void);

bool nir_get_enabled(void);
void nir_set_enabled(bool enabled);

uint16_t nir_get_delayms(void);
void nir_set_delayms(uint16_t delayms);

#endif // NIKON_IR_REMOTE_H
