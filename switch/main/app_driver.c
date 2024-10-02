#include <sdkconfig.h>
#include <iot_button.h>
#include <esp_rmaker_core.h>
#include <esp_rmaker_standard_params.h>
#include <app_reset.h>
#include <ws2812_led.h>
#include <inttypes.h>
#include <nvs.h>
#include <nvs_flash.h>
#include "app_priv.h"

#define BUTTON_ACTIVE_LEVEL 0
#define RELAY_ACTIVE_LEVEL 0
#define NUM_OF_SWITCHES 4

int8_t g_relay_state[NUM_OF_SWITCHES] = {false, false, false, false};
int8_t g_button_state[NUM_OF_SWITCHES] = {true, true, true, true};

#define WIFI_RESET_BUTTON_TIMEOUT 3
#define FACTORY_RESET_BUTTON_TIMEOUT 10
static esp_err_t write_state(int is_relay, int num);

static const int relay_gpio[NUM_OF_SWITCHES] = 
{
    CONFIG_RELAY_GPIO_0,
    CONFIG_RELAY_GPIO_1,
    CONFIG_RELAY_GPIO_2,
    CONFIG_RELAY_GPIO_3
};

static const int button_gpio[NUM_OF_SWITCHES] = 
{
    CONFIG_BUTTON_GPIO_0,
    CONFIG_BUTTON_GPIO_1,
    CONFIG_BUTTON_GPIO_2,
    CONFIG_BUTTON_GPIO_3
};

esp_err_t load_state(){
    printf("OPENing NVS handle...\n");
    nvs_handle_t rb_handle;
    esp_err_t err = nvs_open("storage", NVS_READWRITE, &rb_handle);

    if(err != ESP_OK){
        printf("Error opening NVS handle : %s\n", esp_err_to_name(err));
        return err;
    }
    printf("Successfully OPENed NVS handle, now reading last state...\n");
    for(int i = 0; i < NUM_OF_SWITCHES; i++){
        char rb_name[10];
        /*Reading Relay states*/
        snprintf(rb_name, sizeof(rb_name), "relay_%d", i);
        g_relay_state[i] = 0;
        nvs_get_i8(rb_handle, rb_name, &g_relay_state[i]);

        /*Reading Button states*/
        int8_t nvs_button_state = 1;
        int8_t curr_button_state;
        snprintf(rb_name, sizeof(rb_name), "button_%d", i);
        err = nvs_get_i8(rb_handle, rb_name, &nvs_button_state);
        curr_button_state = (int8_t)gpio_get_level(button_gpio[i]);
        
        switch (err) {
            case ESP_OK:
                printf("State values successfully read\n");
                if(nvs_button_state != curr_button_state){
                    printf("Change detected in button %d i.e. GPIO %d \n", i, button_gpio[i]);
                    printf("Changing stored buttn state from %d to %d\n", nvs_button_state, curr_button_state);
                    g_button_state[i] = curr_button_state;
                    /*writing button state into nvs*/
                    write_state(0, i);
                    g_relay_state[i] = !g_relay_state[i];
                }       
                break;
            case ESP_ERR_NVS_NOT_FOUND:
                printf("The value is not initialized yet!\n");
                break;
            default :
                printf("Error (%s) reading!\n", esp_err_to_name(err));
        }
    }
    nvs_close(rb_handle);
    return ESP_OK;
}

static esp_err_t write_state(int is_relay, int num){
    nvs_handle_t rb_handle;
    esp_err_t err = nvs_open("storage", NVS_READWRITE, &rb_handle);

    if(err != ESP_OK){
        printf("\nError opening NVS handle : %s", esp_err_to_name(err));
        return err;
    }
    printf("Updating %s state in NVS ... ", is_relay ? "relay" : "button");
    char rb_name[10];
    snprintf(rb_name, sizeof(rb_name), is_relay ? "relay_%d" : "button_%d", num);
    err = nvs_set_i8(rb_handle, rb_name, is_relay ? g_relay_state[num] : g_button_state[num]);
    printf((err != ESP_OK) ? "Failed!\n" : "Done\n");

    printf("Committing updates in NVS ... ");
    err = nvs_commit(rb_handle);
    printf((err != ESP_OK) ? "Failed!\n" : "Done\n");

    nvs_close(rb_handle);
    return ESP_OK;
}

static void push_btn_cb(void *arg) {
}

static void btn_push(void *arg) {
    int switch_num = (int)arg;
    app_driver_set_state(switch_num, true);
    esp_rmaker_param_update_and_report(
        esp_rmaker_device_get_param_by_name(switch_devices[switch_num], ESP_RMAKER_DEF_POWER_NAME),
        esp_rmaker_bool(true));
    g_button_state[switch_num] = 0;
    write_state(0, switch_num);
}

static void btn_release(void *arg) {
    int switch_num = (int)arg;
    app_driver_set_state(switch_num, false);
    esp_rmaker_param_update_and_report(
        esp_rmaker_device_get_param_by_name(switch_devices[switch_num], ESP_RMAKER_DEF_POWER_NAME),
        esp_rmaker_bool(false));
    g_button_state[switch_num] = 1;
    write_state(0, switch_num);
}

static void set_power_state(int switch_num, bool state) {
    if(RELAY_ACTIVE_LEVEL  == 1){
        gpio_set_level(relay_gpio[switch_num], state);    
    }else{
        gpio_set_level(relay_gpio[switch_num], !state);
    }
}

void app_gpio_init() {
    if(RELAY_ACTIVE_LEVEL == 0){
        for (int i = 0; i < NUM_OF_SWITCHES; i++) {
            // Configure relay_gpio
            gpio_config_t relay_conf = {
                .mode = GPIO_MODE_INPUT_OUTPUT,
                .pull_up_en = 1,
                .pin_bit_mask = ((uint64_t)1 << relay_gpio[i])
            };
            gpio_config(&relay_conf);
            
            // Configure button_gpio
            gpio_config_t button_conf = {
                .mode = GPIO_MODE_INPUT_OUTPUT,
                .pull_up_en = 1,
                .pin_bit_mask = ((uint64_t)1 << button_gpio[i])
            };
            gpio_config(&button_conf);
            gpio_set_level(button_gpio[i], true);
        }
    }else{
        for (int i = 0; i < NUM_OF_SWITCHES; i++) {
            // Configure relay_gpio with pull-down enabled
            gpio_config_t relay_conf = {
                .mode = GPIO_MODE_INPUT_OUTPUT,
                .pull_down_en = 1,
                .pin_bit_mask = ((uint64_t)1 << relay_gpio[i])
            };
            gpio_config(&relay_conf);
            
            // Configure button_gpio with pull-up enabled
            gpio_config_t button_conf = {
                .mode = GPIO_MODE_INPUT_OUTPUT,
                .pull_up_en = 1,
                .pin_bit_mask = ((uint64_t)1 << button_gpio[i])
            };
            gpio_config(&button_conf);
            gpio_set_level(button_gpio[i], true);
        }

    }
    /**/
    gpio_config_t io_conf = {
            .mode = GPIO_MODE_INPUT_OUTPUT,
            .pull_down_en = 1,
        };
        io_conf.pin_bit_mask = ((uint64_t)1 << CONFIG_WIFI_LED);
        gpio_config(&io_conf);
}

void app_driver_init() {
    button_handle_t btn_handle = iot_button_create(CONFIG_BOOT_BUTTON, BUTTON_ACTIVE_LEVEL);
    if (btn_handle) {
        iot_button_set_evt_cb(btn_handle, BUTTON_CB_TAP, push_btn_cb, (void *)0);
        app_reset_button_register(btn_handle, WIFI_RESET_BUTTON_TIMEOUT, FACTORY_RESET_BUTTON_TIMEOUT);
    }
    for (int i = 0; i < NUM_OF_SWITCHES; i++) {
        button_handle_t btn_handle = iot_button_create(button_gpio[i], BUTTON_ACTIVE_LEVEL);
        if (btn_handle) {
            iot_button_set_evt_cb(btn_handle, BUTTON_CB_PUSH, btn_push, (void *)i);
            iot_button_set_evt_cb(btn_handle, BUTTON_CB_RELEASE, btn_release, (void *)i);
        }
        // set_power_state(i, g_relay_state[i]);
    }
}

int IRAM_ATTR app_driver_set_state(int switch_num, bool state) {
    printf("app_driver_set_state called for %d to be set %d\n", switch_num, (int)state);
    if (g_relay_state[switch_num] != state) {
        g_relay_state[switch_num] = state;
        set_power_state(switch_num, g_relay_state[switch_num]);
        write_state(1, switch_num);
    } else {
        set_power_state(switch_num, g_relay_state[switch_num]);
    }
    return ESP_OK;
}

bool app_driver_get_state(int switch_num) {
    return g_relay_state[switch_num];
}
