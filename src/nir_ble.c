#include "nir_ble.h"

#include <esp_log.h>
#include <esp_nimble_hci.h>
#include <nimble/nimble_port.h>
#include <nimble/nimble_port_freertos.h>
#include <services/gap/ble_svc_gap.h>
#include <services/gatt/ble_svc_gatt.h>

// NOTE: https://github.com/espressif/esp-idf/tree/master/examples/bluetooth/nimble/blehr

extern const char *TAG;

const char* nir_device_name = "nikon-ir-remote";

// Service: Nikon IR Remote
const ble_uuid128_t nir_service_uuid = {
    .u = { .type = BLE_UUID_TYPE_128 },
    .value = { 0xFF, 0xEE, 0xDD, 0xCC, 0xBB, 0xAA, 0x99, 0x88, 0x77, 0x66, 0x55, 0x44, 0x33, 0x22, 0x11, 0x00 },
};

// Characteristic: Enabled
const ble_uuid128_t nir_enabled_uuid = {
    .u = { .type = BLE_UUID_TYPE_128 },
    .value = { 0xFF, 0xEE, 0xDD, 0xCC, 0xBB, 0xAA, 0x99, 0x88, 0x77, 0x66, 0x55, 0x44, 0x33, 0x22, 0x11, 0x01 },
};

// Characteristic: Delay MS
const ble_uuid128_t nir_delayms_uuid = {
    .u = { .type = BLE_UUID_TYPE_128 },
    .value = { 0xFF, 0xEE, 0xDD, 0xCC, 0xBB, 0xAA, 0x99, 0x88, 0x77, 0x66, 0x55, 0x44, 0x33, 0x22, 0x11, 0x02 }
};

uint8_t nir_addr_type;
uint16_t nir_conn_handle;

int nir_gatt_svr_chr_access(uint16_t conn_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt *ctxt, void *arg);
int nir_ble_gap_event(struct ble_gap_event *event, void *arg);
void nimble_error(int errno);

const struct ble_gatt_svc_def gatt_svr_svcs[] = { {
        // service: Nikon IR Remote
        .type = BLE_GATT_SVC_TYPE_PRIMARY,
        .uuid = &nir_service_uuid.u,
        .characteristics = (struct ble_gatt_chr_def[]) { {
                // characteristic: enabled
                .uuid = &nir_enabled_uuid.u,
                .access_cb = nir_gatt_svr_chr_access,
                .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_WRITE,
            }, {
                // characteristic: delay ms
                .uuid = &nir_delayms_uuid.u,
                .access_cb = nir_gatt_svr_chr_access,
                .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_WRITE,
            }, {
                0,
            },
        }
    }, {
        0,
    },
};

int nir_gatt_svr_chr_access(uint16_t conn_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt *ctxt, void *arg) {
    ESP_LOGI(TAG, "nir_enabled_gatt_svr_chr_access");

    if (ble_uuid_cmp(ctxt->chr->uuid, &nir_enabled_uuid.u) == 0) {
        ESP_LOGI(TAG, "nir_enabled_uuid");

        if (ctxt->op == BLE_GATT_ACCESS_OP_WRITE_CHR) {
            ESP_LOGI(TAG, "write");
            bool enabled;

            uint16_t om_len;
            uint16_t om_actual_len;

            om_len = OS_MBUF_PKTLEN(ctxt->om);
            if (om_len != sizeof enabled) {
                return BLE_ATT_ERR_INVALID_ATTR_VALUE_LEN;
            }

            int rc = ble_hs_mbuf_to_flat(ctxt->om, &enabled, sizeof enabled, &om_actual_len);
            ESP_LOGI(TAG, "enabled: %d", enabled);

            nir_set_enabled(enabled);
            return rc == 0 ? 0 : BLE_ATT_ERR_UNLIKELY;
        } else if (ctxt->op == BLE_GATT_ACCESS_OP_READ_CHR) {
            ESP_LOGI(TAG, "read");
            uint16_t enabled = nir_get_enabled();

            ESP_LOGI(TAG, "enabled: %d", enabled);

            int rc = os_mbuf_append(ctxt->om, &enabled, sizeof enabled);

            return rc == 0 ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;
        }
    } else if (ble_uuid_cmp(ctxt->chr->uuid, &nir_delayms_uuid.u) == 0) {
        ESP_LOGI(TAG, "nir_delayms_uuid");

        if (ctxt->op == BLE_GATT_ACCESS_OP_WRITE_CHR) {
            ESP_LOGI(TAG, "write");
            uint16_t delayms;
            uint16_t om_len;
            uint16_t om_actual_len;

            om_len = OS_MBUF_PKTLEN(ctxt->om);
            if (om_len != sizeof delayms) {
                ESP_LOGE(TAG, "invalid length: %d, expected: %d", om_len, sizeof delayms);
                return BLE_ATT_ERR_INVALID_ATTR_VALUE_LEN;
            }

            int rc = ble_hs_mbuf_to_flat(ctxt->om, &delayms, sizeof delayms, &om_actual_len);
            ESP_LOGI(TAG, "delayms: %u", delayms);

            nir_set_delayms(delayms);

            return rc == 0 ? 0 : BLE_ATT_ERR_UNLIKELY;
        } else if (ctxt->op == BLE_GATT_ACCESS_OP_READ_CHR) {
            ESP_LOGI(TAG, "read");
            
            uint16_t delayms = nir_get_delayms();
            ESP_LOGI(TAG, "delayms: %u", delayms);

            int rc = os_mbuf_append(ctxt->om, &delayms, sizeof delayms);

            return rc == 0 ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;
        }
    }

    return BLE_ATT_ERR_UNLIKELY;
}

void nir_advertise(void) {
    ESP_LOGI(TAG, "nir_advertise");
    int rc;
    struct ble_hs_adv_fields fields;
    struct ble_gap_adv_params adv_params;

    memset(&fields, 0, sizeof fields);
    fields.name = (uint8_t *) nir_device_name;
    fields.name_len = strlen(nir_device_name);
    fields.name_is_complete = 1;

    ESP_LOGI(TAG, "ble_gap_adv_set_fields");
    rc = ble_gap_adv_set_fields(&fields);
    nimble_error(rc);

    memset(&adv_params, 0, sizeof adv_params);
    adv_params.conn_mode = BLE_GAP_CONN_MODE_UND;
    adv_params.disc_mode = BLE_GAP_DISC_MODE_GEN;

    ESP_LOGI(TAG, "ble_gap_adv_start");
    rc = ble_gap_adv_start(nir_addr_type, NULL, BLE_HS_FOREVER, &adv_params, nir_ble_gap_event, NULL);
    nimble_error(rc);
}

int nir_ble_gap_event(struct ble_gap_event *event, void *arg) {
    ESP_LOGI(TAG, "nir_ble_gap_event");

    switch (event->type) {
        case BLE_GAP_EVENT_CONNECT:
            ESP_LOGI(TAG, "BLE_GAP_EVENT_CONNECT %s status: %d",
                event->connect.status == 0 ? "established" : "failed",
                event->connect.status);

            if (event->connect.status) {
                nir_advertise();
            }

            nir_conn_handle = event->connect.conn_handle;
            break;

        case BLE_GAP_EVENT_DISCONNECT:
            ESP_LOGI(TAG, "BLE_GAP_EVENT_DISCONNECT reason: %d", event->disconnect.reason);

            nir_advertise();
            break;

        case BLE_GAP_EVENT_CONN_UPDATE:
            ESP_LOGW(TAG, "BLE_GAP_EVENT_CONN_UPDATE status: %d", event->conn_update.status);
            break;

        case BLE_GAP_EVENT_CONN_UPDATE_REQ:
            ESP_LOGW(TAG, "BLE_GAP_EVENT_CONN_UPDATE_REQ unhandled");
            break;

        case BLE_GAP_EVENT_L2CAP_UPDATE_REQ:
            ESP_LOGW(TAG, "BLE_GAP_EVENT_L2CAP_UPDATE_REQ unhandled");
            break;

        case BLE_GAP_EVENT_TERM_FAILURE:
            ESP_LOGW(TAG, "BLE_GAP_EVENT_TERM_FAILURE unhandled");
            break;

        case BLE_GAP_EVENT_DISC:
            ESP_LOGW(TAG, "BLE_GAP_EVENT_DISC unhandled");
            break;

        case BLE_GAP_EVENT_DISC_COMPLETE:
            ESP_LOGW(TAG, "BLE_GAP_EVENT_DISC_COMPLETE unhandled");
            break;

        case BLE_GAP_EVENT_ADV_COMPLETE:
            ESP_LOGW(TAG, "BLE_GAP_EVENT_ADV_COMPLETE unhandled");
            break;

        case BLE_GAP_EVENT_ENC_CHANGE:
            ESP_LOGW(TAG, "BLE_GAP_EVENT_ENC_CHANGE unhandled");
            break;

        case BLE_GAP_EVENT_PASSKEY_ACTION:
            ESP_LOGW(TAG, "BLE_GAP_EVENT_PASSKEY_ACTION unhandled");
            break;

        case BLE_GAP_EVENT_NOTIFY_RX:
            ESP_LOGW(TAG, "BLE_GAP_EVENT_NOTIFY_RX unhandled");
            break;

        case BLE_GAP_EVENT_NOTIFY_TX:
            ESP_LOGW(TAG, "BLE_GAP_EVENT_NOTIFY_TX unhandled");
            break;

        case BLE_GAP_EVENT_SUBSCRIBE:
            ESP_LOGW(TAG, "BLE_GAP_EVENT_SUBSCRIBE unhandled");
            break;

        case BLE_GAP_EVENT_MTU:
            ESP_LOGW(TAG, "BLE_GAP_EVENT_MTU unhandled");
            break;

        case BLE_GAP_EVENT_IDENTITY_RESOLVED:
            ESP_LOGW(TAG, "BLE_GAP_EVENT_IDENTITY_RESOLVED unhandled");
            break;

        case BLE_GAP_EVENT_REPEAT_PAIRING:
            ESP_LOGW(TAG, "BLE_GAP_EVENT_REPEAT_PAIRING unhandled");
            break;

        case BLE_GAP_EVENT_PHY_UPDATE_COMPLETE:
            ESP_LOGW(TAG, "BLE_GAP_EVENT_PHY_UPDATE_COMPLETE unhandled");
            break;

        case BLE_GAP_EVENT_EXT_DISC:
            ESP_LOGW(TAG, "BLE_GAP_EVENT_EXT_DISC unhandled");
            break;

        case BLE_GAP_EVENT_PERIODIC_SYNC:
            ESP_LOGW(TAG, "BLE_GAP_EVENT_PERIODIC_SYNC unhandled");
            break;

        case BLE_GAP_EVENT_PERIODIC_REPORT:
            ESP_LOGW(TAG, "BLE_GAP_EVENT_PERIODIC_REPORT unhandled");
            break;

        case BLE_GAP_EVENT_PERIODIC_SYNC_LOST:
            ESP_LOGW(TAG, "BLE_GAP_EVENT_PERIODIC_SYNC_LOST unhandled");
            break;

        case BLE_GAP_EVENT_SCAN_REQ_RCVD:
            ESP_LOGW(TAG, "BLE_GAP_EVENT_SCAN_REQ_RCVD unhandled");
            break;

        case BLE_GAP_EVENT_PERIODIC_TRANSFER:
            ESP_LOGW(TAG, "BLE_GAP_EVENT_PERIODIC_TRANSFER unhandled");
            break;

        default:
            ESP_LOGE(TAG, "Unknown event type: %d", event->type);
            break;
    }

    return 0;
}

void nimble_error(int errno) {
    switch(errno) {
        case 0:
            ESP_LOGI(TAG, "Success");
            break;
        case BLE_HS_EAGAIN:
            ESP_LOGE(TAG, "Error BLE_HS_EAGAIN");
            break;
        case BLE_HS_EALREADY:
            ESP_LOGE(TAG, "Error BLE_HS_EALREADY");
            break;
        case BLE_HS_EINVAL:
            ESP_LOGE(TAG, "Error BLE_HS_EINVAL");
            break;
        case BLE_HS_EMSGSIZE:
            ESP_LOGE(TAG, "Error BLE_HS_EMSGSIZE");
            break;
        case BLE_HS_ENOENT:
            ESP_LOGE(TAG, "Error BLE_HS_ENOENT");
            break;
        case BLE_HS_ENOMEM:
            ESP_LOGE(TAG, "Error BLE_HS_ENOMEM");
            break;
        case BLE_HS_ENOTCONN:
            ESP_LOGE(TAG, "Error BLE_HS_ENOTCONN");
            break;
        case BLE_HS_ENOTSUP:
            ESP_LOGE(TAG, "Error BLE_HS_ENOTSUP");
            break;
        case BLE_HS_EAPP:
            ESP_LOGE(TAG, "Error BLE_HS_EAPP");
            break;
        case BLE_HS_EBADDATA:
            ESP_LOGE(TAG, "Error BLE_HS_EBADDATA");
            break;
        case BLE_HS_EOS:
            ESP_LOGE(TAG, "Error BLE_HS_EOS");
            break;
        case BLE_HS_ECONTROLLER:
            ESP_LOGE(TAG, "Error BLE_HS_ECONTROLLER");
            break;
        case BLE_HS_ETIMEOUT:
            ESP_LOGE(TAG, "Error BLE_HS_ETIMEOUT");
            break;
        case BLE_HS_EDONE:
            ESP_LOGE(TAG, "Error BLE_HS_EDONE");
            break;
        case BLE_HS_EBUSY:
            ESP_LOGE(TAG, "Error BLE_HS_EBUSY");
            break;
        case BLE_HS_EREJECT:
            ESP_LOGE(TAG, "Error BLE_HS_EREJECT");
            break;
        case BLE_HS_EUNKNOWN:
            ESP_LOGE(TAG, "Error BLE_HS_EUNKNOWN");
            break;
        case BLE_HS_EROLE:
            ESP_LOGE(TAG, "Error BLE_HS_EROLE");
            break;
        case BLE_HS_ETIMEOUT_HCI:
            ESP_LOGE(TAG, "Error BLE_HS_ETIMEOUT_HCI");
            break;
        case BLE_HS_ENOMEM_EVT:
            ESP_LOGE(TAG, "Error BLE_HS_ENOMEM_EVT");
            break;
        case BLE_HS_ENOADDR:
            ESP_LOGE(TAG, "Error BLE_HS_ENOADDR");
            break;
        case BLE_HS_ENOTSYNCED:
            ESP_LOGE(TAG, "Error BLE_HS_ENOTSYNCED");
            break;
        case BLE_HS_EAUTHEN:
            ESP_LOGE(TAG, "Error BLE_HS_EAUTHEN");
            break;
        case BLE_HS_EAUTHOR:
            ESP_LOGE(TAG, "Error BLE_HS_EAUTHOR");
            break;
        case BLE_HS_EENCRYPT:
            ESP_LOGE(TAG, "Error BLE_HS_EENCRYPT");
            break;
        case BLE_HS_EENCRYPT_KEY_SZ:
            ESP_LOGE(TAG, "Error BLE_HS_EENCRYPT_KEY_SZ");
            break;
        case BLE_HS_ESTORE_CAP:
            ESP_LOGE(TAG, "Error BLE_HS_ESTORE_CAP");
            break;
        case BLE_HS_ESTORE_FAIL:
            ESP_LOGE(TAG, "Error BLE_HS_ESTORE_FAIL");
            break;
        case BLE_HS_EPREEMPTED:
            ESP_LOGE(TAG, "Error BLE_HS_EPREEMPTED");
            break;
        case BLE_HS_EDISABLED:
            ESP_LOGE(TAG, "Error BLE_HS_EDISABLED");
            break;
        case BLE_HS_ESTALLED:
            ESP_LOGE(TAG, "Error BLE_HS_ESTALLED");
            break;
        default:
            ESP_LOGE(TAG, "Error %d", errno);
            break;
    }
}

void nir_ble_hs_sync(void) {
    ESP_LOGI(TAG, "nir_ble_hs_sync");
    int rc;

    ESP_LOGI(TAG, "ble_hs_id_infer_auto");
    rc = ble_hs_id_infer_auto(0, &nir_addr_type);
    nimble_error(rc);
    ESP_LOGI(TAG, "ble_hs_id_infer_auto: %u", nir_addr_type);
    
    uint8_t addr_val[6] = {0};
    ESP_LOGI(TAG, "ble_hs_id_copy_addr");
    rc = ble_hs_id_copy_addr(nir_addr_type, addr_val, NULL);
    nimble_error(rc);
    ESP_LOGI(TAG, "ble_hs_id_copy_addr: %02x:%02x:%02x:%02x:%02x:%02x",
        addr_val[0], addr_val[1], addr_val[2], addr_val[3], addr_val[4], addr_val[5]);

    nir_advertise();
}

void nir_ble_hs_reset(int reason) {
    ESP_LOGI(TAG, "nir_ble_hs_reset reason: %d", reason);
}

void nir_ble_init(void) {
    ESP_LOGI(TAG, "nir_setup_ble");

    int rc;

    ESP_LOGI(TAG, "esp_nimble_hci_and_controller_init");
    rc = esp_nimble_hci_and_controller_init();
    nimble_error(rc);

    ESP_LOGI(TAG, "nimble_port_init");
    nimble_port_init();

    ble_hs_cfg.sync_cb = nir_ble_hs_sync;
    ble_hs_cfg.reset_cb = nir_ble_hs_reset;

    ble_svc_gap_init();
    ble_svc_gatt_init();

    ESP_LOGI(TAG, "ble_gatts_count_cfg");
    rc = ble_gatts_count_cfg(gatt_svr_svcs);
    nimble_error(rc);

    ESP_LOGI(TAG, "ble_gatts_add_svcs");
    rc = ble_gatts_add_svcs(gatt_svr_svcs);
    nimble_error(rc);

    ESP_LOGI(TAG, "ble_svc_gap_device_name_set: %s", nir_device_name);
    rc = ble_svc_gap_device_name_set(nir_device_name);
    nimble_error(rc);
}

void nir_ble_host_task(void *param) {
    ESP_LOGI(TAG, "nir_host_task");

    nimble_port_run();

    nimble_port_freertos_deinit();
}
