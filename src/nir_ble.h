#include <host/ble_hs.h>

#include "nikon_ir_remote.h"

#ifndef NIR_BLE_H
#define NIR_BLE_H

void nir_ble_init(void);
void nir_ble_host_task(void *param);

#endif // NIR_BLE_H
