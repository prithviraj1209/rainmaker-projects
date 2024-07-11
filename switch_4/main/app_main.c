#include <string.h>
#include <inttypes.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_log.h>
#include <esp_event.h>
#include <nvs_flash.h>
#include <esp_wifi.h> 
#include <driver/gpio.h>
#include <esp_rmaker_core.h>
#include <esp_rmaker_standard_types.h>
#include <esp_rmaker_standard_params.h>
#include <esp_rmaker_standard_devices.h>
#include <esp_rmaker_schedule.h>
#include <esp_rmaker_scenes.h>
#include <esp_rmaker_console.h>
#include <esp_rmaker_ota.h>
#include <esp_rmaker_common_events.h>
#include <app_wifi.h>
#include <app_insights.h>

#include "app_priv.h"

#define NUM_OF_SWITCHES 4                               //NUMBER OF SWITCHES TO BE CREATED.
static const char *TAG = "app_main";                    //TAG, USEFUL WHILE DEBUGGING.
esp_rmaker_device_t *switch_devices[NUM_OF_SWITCHES];   //CREATING POINTER TO SWITCH DEVICES.

/*Function for returning the index of the device that is given as argument*/
int switch_index(const esp_rmaker_device_t* device){    
    for(int i = 0; i < NUM_OF_SWITCHES; i++){
        if(strcmp(esp_rmaker_device_get_name(switch_devices[i]), esp_rmaker_device_get_name(device)) == 0) {
            return i;
        }
    }
    return -1;      
}

/*Function for handling the data received from RainMaker CLOUD*/
static esp_err_t write_cb(const esp_rmaker_device_t *device, const esp_rmaker_param_t *param, const esp_rmaker_param_val_t val, void *priv_data, esp_rmaker_write_ctx_t *ctx){
    int switch_num = switch_index(device);
    if (ctx) {
        ESP_LOGI(TAG, "Received write request via : %s", esp_rmaker_device_cb_src_to_str(ctx->src));
        printf("\n----> %d \n", switch_num);
    }
    if (strcmp(esp_rmaker_param_get_name(param), ESP_RMAKER_DEF_POWER_NAME) == 0) {
        ESP_LOGI(TAG, "Received value = %s for %s - %s", val.val.b ? "true" : "false", esp_rmaker_device_get_name(device), esp_rmaker_param_get_name(param));
        printf("\n----> %d \n", switch_num);
        app_driver_set_state(switch_num, val.val.b);
        esp_rmaker_param_update_and_report(param, val);
    }
    return ESP_OK;
}

/* Event handler for catching RainMaker events */
static void event_handler(void* arg, esp_event_base_t event_base,
                          int32_t event_id, void* event_data)
{
    if (event_base == RMAKER_EVENT) {
        switch (event_id) {
            case RMAKER_EVENT_INIT_DONE:
                ESP_LOGI(TAG, "RainMaker Initialised.");
                break;
            case RMAKER_EVENT_CLAIM_STARTED:
                ESP_LOGI(TAG, "RainMaker Claim Started.");
                break;
            case RMAKER_EVENT_CLAIM_SUCCESSFUL:
                ESP_LOGI(TAG, "RainMaker Claim Successful.");
                break;
            case RMAKER_EVENT_CLAIM_FAILED:
                ESP_LOGI(TAG, "RainMaker Claim Failed.");
                break;
            case RMAKER_EVENT_LOCAL_CTRL_STARTED:
                ESP_LOGI(TAG, "Local Control Started.");
                break;
            case RMAKER_EVENT_LOCAL_CTRL_STOPPED:
                ESP_LOGI(TAG, "Local Control Stopped.");
                break;
            default:
                ESP_LOGW(TAG, "Unhandled RainMaker Event: %"PRIi32, event_id);
        }
    } else if (event_base == RMAKER_COMMON_EVENT) {
        switch (event_id) {
            case RMAKER_EVENT_REBOOT:
                ESP_LOGI(TAG, "Rebooting in %d seconds.", *((uint8_t *)event_data));
                break;
            case RMAKER_EVENT_WIFI_RESET:
                ESP_LOGI(TAG, "Wi-Fi credentials reset.");
                break;
            case RMAKER_EVENT_FACTORY_RESET:
                ESP_LOGI(TAG, "Node reset to factory defaults.");
                break;
            case RMAKER_MQTT_EVENT_CONNECTED:
                ESP_LOGI(TAG, "MQTT Connected.");
                break;
            case RMAKER_MQTT_EVENT_DISCONNECTED:
                ESP_LOGI(TAG, "MQTT Disconnected.");
                break;
            case RMAKER_MQTT_EVENT_PUBLISHED:
                ESP_LOGI(TAG, "MQTT Published. Msg id: %d.", *((int *)event_data));
                break;
            default:
                ESP_LOGW(TAG, "Unhandled RainMaker Common Event: %"PRIi32, event_id);
        }
    } else if (event_base == APP_WIFI_EVENT) {
        switch (event_id) {
            case APP_WIFI_EVENT_QR_DISPLAY:
                ESP_LOGI(TAG, "Provisioning QR : %s", (char *)event_data);
                break;
            case APP_WIFI_EVENT_PROV_TIMEOUT:
                ESP_LOGI(TAG, "Provisioning Timed Out. Please reboot.");
                break;
            case APP_WIFI_EVENT_PROV_RESTART:
                ESP_LOGI(TAG, "Provisioning has restarted due to failures.");
                break;
            default:
                ESP_LOGW(TAG, "Unhandled App Wi-Fi Event: %"PRIi32, event_id);
                break;
        }
    } else if(event_base == WIFI_EVENT){
        switch(event_id) {
            case WIFI_EVENT_STA_START:
                ESP_LOGI(TAG, "Wi-Fi started");
                break;
            case WIFI_EVENT_STA_CONNECTED:
                gpio_set_level(2, true);
                ESP_LOGI(TAG, "Wi-Fi connected ");
                break;
            case WIFI_EVENT_STA_DISCONNECTED:
                gpio_set_level(2, false);
                break;
        }
    }else if (event_base == RMAKER_OTA_EVENT) {
        switch(event_id) {
            case RMAKER_OTA_EVENT_STARTING:
                ESP_LOGI(TAG, "Starting OTA.");
                break;
            case RMAKER_OTA_EVENT_IN_PROGRESS:
                ESP_LOGI(TAG, "OTA is in progress.");
                break;
            case RMAKER_OTA_EVENT_SUCCESSFUL:
                ESP_LOGI(TAG, "OTA successful.");
                break;
            case RMAKER_OTA_EVENT_FAILED:
                ESP_LOGI(TAG, "OTA Failed.");
                break;
            case RMAKER_OTA_EVENT_REJECTED:
                ESP_LOGI(TAG, "OTA Rejected.");
                break;
            case RMAKER_OTA_EVENT_DELAYED:
                ESP_LOGI(TAG, "OTA Delayed.");
                break;
            case RMAKER_OTA_EVENT_REQ_FOR_REBOOT:
                ESP_LOGI(TAG, "Firmware image downloaded. Please reboot your device to apply the upgrade.");
                break;
            default:
                ESP_LOGW(TAG, "Unhandled OTA Event: %"PRIi32, event_id);
                break;
        }
    } else {
        ESP_LOGW(TAG, "Invalid event received!");
    }
}

void app_main(){

    esp_rmaker_console_init();
    /*NVS initialization*/
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);

    app_gpio_init();
    /*Loading Button and Relay states*/
    load_state();
    for(int i = 0; i < NUM_OF_SWITCHES; i++){
        app_driver_set_state(i, g_relay_state[i]);
    }
    
    app_driver_init();

    /* Initialize Wi-Fi. Note that, this should be called before esp_rmaker_node_init()*/
    app_wifi_init();

    /* Register an event handler to catch RainMaker events */
    ESP_ERROR_CHECK(esp_event_handler_register(RMAKER_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(RMAKER_COMMON_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(APP_WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(RMAKER_OTA_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL));
    /* Initialize the ESP RainMaker Agent.
     * Note that this should be called after app_wifi_init() but before app_wifi_start()
     */
    esp_rmaker_config_t rainmaker_cfg = {
        .enable_time_sync = false,
    };
    //CHANGE "MASTER BEDROOM" ACCORDING TO YOUR NEED.
    esp_rmaker_node_t *node = esp_rmaker_node_init(&rainmaker_cfg, "Master Bedroom", "switch");     
    if (!node) {
        ESP_LOGE(TAG, "Could not initialise node. Aborting!!!");
        vTaskDelay(5000 / portTICK_PERIOD_MS);
        abort();
    }
    /*Creating the adding SWITCH DEVICES to the node*/
    for (int i = 0; i < NUM_OF_SWITCHES; i++) {
        char device_name[10];
        snprintf(device_name, sizeof(device_name), "Switch_%d", i);
        switch_devices[i] = esp_rmaker_device_create(device_name, ESP_RMAKER_DEVICE_SWITCH, NULL);
        esp_rmaker_device_add_cb(switch_devices[i], write_cb, (void *)i);
        esp_rmaker_device_add_param(switch_devices[i], esp_rmaker_name_param_create(ESP_RMAKER_DEF_NAME_PARAM, device_name));
        esp_rmaker_param_t *power_param = esp_rmaker_power_param_create(ESP_RMAKER_DEF_POWER_NAME, g_relay_state[i]);
        esp_rmaker_device_add_param(switch_devices[i], power_param);
        esp_rmaker_device_assign_primary_param(switch_devices[i], power_param);
        esp_rmaker_node_add_device(node, switch_devices[i]);
        esp_rmaker_param_update_and_report(esp_rmaker_device_get_param_by_name(switch_devices[i], ESP_RMAKER_DEF_POWER_NAME), esp_rmaker_bool(g_relay_state[i]));
        
    }

    esp_rmaker_ota_enable_default();
    esp_rmaker_timezone_service_enable();
    esp_rmaker_schedule_enable();
    esp_rmaker_scenes_enable();
    app_insights_enable();
    esp_rmaker_start();

    err = app_wifi_start(POP_TYPE_RANDOM);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start Wi-Fi. Aborting!!!");
        vTaskDelay(5000 / portTICK_PERIOD_MS);
        abort();
    }

}

