#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <driver/gpio.h>
#include <driver/timer.h>
#include <esp_log.h>
#include <esp_system.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <nimble/nimble_port_freertos.h>
#include <nvs_flash.h>

#include "nir_ble.h"
#include "nir_nvs.h"
#include "nir_timer.h"

const char *TAG = "XXX";

void setup_esp32(void) {
    ESP_LOGI(TAG, "setup_esp32");

    esp_chip_info_t chip_info;
    esp_chip_info(&chip_info);

    ESP_LOGI(TAG, "ESP32 w/%d cores, WiFi%s%s, ",
        chip_info.cores,
        (chip_info.features & CHIP_FEATURE_BT) ? "/BT" : "",
        (chip_info.features & CHIP_FEATURE_BLE) ? "/BLE" : "");

    ESP_LOGI(TAG, "Silicon revision %d, ", chip_info.revision);

    ESP_LOGI(TAG, "%dMB %s flash", spi_flash_get_chip_size() / (1024 * 1024),
        (chip_info.features & CHIP_FEATURE_EMB_FLASH) ? "embedded" : "external");
}

void wait_for_serial(int wait_seconds) {
    for (int i = wait_seconds; i >= 0; i--) {
        ESP_LOGI(TAG, "Waiting to start in %d seconds...", i);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}

void app_main() {
    wait_for_serial(10);
    ESP_LOGI(TAG, "app_main");

    setup_esp32();

    nir_init();

    ESP_LOGI(TAG, "/app_main");
}
