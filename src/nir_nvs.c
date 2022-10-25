#include <esp_system.h>
#include <nvs.h>
#include <esp_log.h>
#include <nvs_flash.h>

#include "nir_nvs.h"

extern const char *TAG;

const char* NIR_NVS_NAMESPACE = "nikon_ir_remote";

nvs_handle_t _nir_nvs_handle;

void nir_init_nvs(void) {
    ESP_LOGI(TAG, "nvs_flash_init");
    switch (nvs_flash_init()) {
        case ESP_OK:
            ESP_LOGI(TAG, "NVS OK");
            break;
        case ESP_ERR_NVS_NEW_VERSION_FOUND:
            ESP_LOGW(TAG, "NVS New Version Found");
            nvs_flash_erase();
            nvs_flash_init();
            break;
        case ESP_ERR_NVS_NO_FREE_PAGES:
            ESP_LOGW(TAG, "NVS No Free Pages");
            nvs_flash_erase();
            nvs_flash_init();
            break;
        case ESP_ERR_NOT_FOUND:
            ESP_LOGE(TAG, "NVS Not Found");
            break;
        case ESP_ERR_NO_MEM:
            ESP_LOGE(TAG, "NVS No Memory");
            break;
        default:
            ESP_LOGE(TAG, "WTFBBQ!");
            break;
    }

    ESP_LOGI(TAG, "nvs_open");
    switch (nvs_open(NIR_NVS_NAMESPACE, NVS_READWRITE, &_nir_nvs_handle)) {
        case ESP_OK:
            ESP_LOGI(TAG, "NVS OK");
            break;
        case ESP_ERR_NVS_NOT_INITIALIZED:
            ESP_LOGE(TAG, "NVS Not Initialized");
            break;
        case ESP_ERR_NVS_PART_NOT_FOUND:
            ESP_LOGE(TAG, "NVS Part Not Found");
            break;
        case ESP_ERR_NVS_NOT_FOUND:
            ESP_LOGE(TAG, "NVS Not Found");
            break;
        case ESP_ERR_NVS_INVALID_NAME:
            ESP_LOGE(TAG, "NVS Invalid Name");
            break;
        case ESP_ERR_NO_MEM:
            ESP_LOGE(TAG, "NVS No Memory");
            break;
        default:
            ESP_LOGE(TAG, "WTFBBQ!");
            break;
    }
}

void nir_deinit(void) {
    nvs_close(_nir_nvs_handle);
}

bool nir_nvs_read_bool(const char* key, const bool default_value) {
    bool value;
    size_t len;

    ESP_LOGI(TAG, "nvs_get_blob");
    switch (nvs_get_blob(_nir_nvs_handle, key, &value, &len)) {
        case ESP_OK:
            ESP_LOGI(TAG, "NVS OK");
            break;
        case ESP_ERR_NVS_NOT_FOUND:
            ESP_LOGW(TAG, "NVS Not Found");
            value = default_value;
            break;
        case ESP_ERR_NVS_INVALID_HANDLE:
            ESP_LOGE(TAG, "NVS Invalid Handle");
            break;
        case ESP_ERR_NVS_INVALID_NAME:
            ESP_LOGE(TAG, "NVS Invalid Name");
            break;
        case ESP_ERR_NVS_INVALID_LENGTH:
            ESP_LOGE(TAG, "NVS Invalid Length");
            break;
        default:
            ESP_LOGE(TAG, "WTFBBQ!");
            break;
    }

    return value;
}

void nir_nvs_write_bool(const char* key, const bool value) {
    ESP_LOGI(TAG, "nvs_set_blob");
    switch (nvs_set_blob(_nir_nvs_handle, key, &value, sizeof value)) {
        case ESP_OK:
            ESP_LOGI(TAG, "NVS OK");
            break;
        case ESP_ERR_NVS_INVALID_HANDLE:
            ESP_LOGE(TAG, "NVS Invalid Handle");
            break;
        case ESP_ERR_NVS_READ_ONLY:
            ESP_LOGE(TAG, "NVS Read Only");
            break;
        case ESP_ERR_NVS_INVALID_NAME:
            ESP_LOGE(TAG, "NVS Invalid Name");
            break;
        case ESP_ERR_NVS_NOT_ENOUGH_SPACE:
            ESP_LOGE(TAG, "NVS Not Enough Space");
            break;
        case ESP_ERR_NVS_REMOVE_FAILED:
            ESP_LOGE(TAG, "NVS Remove Failed");
            break;
        default:
            ESP_LOGE(TAG, "WTFBBQ!");
            break;
    }

    ESP_LOGI(TAG, "nvs_commit");
    switch (nvs_commit(_nir_nvs_handle)) {
        case ESP_OK:
            ESP_LOGI(TAG, "NVS OK");
            break;
        case ESP_ERR_NVS_INVALID_HANDLE:
            ESP_LOGE(TAG, "NVS Invalid Handle");
            break;
        default:
            ESP_LOGE(TAG, "WTFBBQ!");
            break;
    }
}

uint16_t nir_nvs_read_uint16(const char* key, const uint16_t default_value) {
    uint16_t value;

    ESP_LOGI(TAG, "nvs_get_u16");
    switch (nvs_get_u16(_nir_nvs_handle, key, &value)) {
        case ESP_OK:
            ESP_LOGI(TAG, "NVS OK");
            break;
        case ESP_ERR_NVS_NOT_FOUND:
            ESP_LOGW(TAG, "NVS Not Found");
            value = default_value;
            break;
        case ESP_ERR_NVS_INVALID_HANDLE:
            ESP_LOGE(TAG, "NVS Invalid Handle");
            break;
        case ESP_ERR_NVS_INVALID_NAME:
            ESP_LOGE(TAG, "NVS Invalid Name");
            break;
        case ESP_ERR_NVS_INVALID_LENGTH:
            ESP_LOGE(TAG, "NVS Invalid Length");
            break;
        default:
            ESP_LOGE(TAG, "WTFBBQ!");
            break;
    }

    return value;
}

void nir_nvs_write_uint16(const char* key, const uint16_t value) {
    ESP_LOGI(TAG, "nvs_set_u16");
    switch (nvs_set_u16(_nir_nvs_handle, key, value)) {
        case ESP_OK:
            ESP_LOGI(TAG, "NVS OK");
            break;
        case ESP_ERR_NVS_INVALID_HANDLE:
            ESP_LOGE(TAG, "NVS Invalid Handle");
            break;
        case ESP_ERR_NVS_READ_ONLY:
            ESP_LOGE(TAG, "NVS Read Only");
            break;
        case ESP_ERR_NVS_INVALID_NAME:
            ESP_LOGE(TAG, "NVS Invalid Name");
            break;
        case ESP_ERR_NVS_NOT_ENOUGH_SPACE:
            ESP_LOGE(TAG, "NVS Not Enough Space");
            break;
        case ESP_ERR_NVS_REMOVE_FAILED:
            ESP_LOGE(TAG, "NVS Remove Failed");
            break;
        default:
            ESP_LOGE(TAG, "WTFBBQ!");
            break;
    }

    ESP_LOGI(TAG, "nvs_commit");
    switch (nvs_commit(_nir_nvs_handle)) {
        case ESP_OK:
            ESP_LOGI(TAG, "NVS OK");
            break;
        case ESP_ERR_NVS_INVALID_HANDLE:
            ESP_LOGE(TAG, "NVS Invalid Handle");
            break;
        default:
            ESP_LOGE(TAG, "WTFBBQ!");
            break;
    }
}
