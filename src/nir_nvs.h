#include <stdint.h>

#ifndef NIR_NVS_H
#define NIR_NVS_H

void nir_init_nvs(void);
void nir_deinit(void);

bool nir_nvs_read_bool(const char* key, const bool default_value);
void nir_nvs_write_bool(const char* key, const bool value);

uint16_t nir_nvs_read_uint16(const char* key, const uint16_t default_value);
void nir_nvs_write_uint16(const char* key, const uint16_t value);

#endif // NIR_NVS_H
