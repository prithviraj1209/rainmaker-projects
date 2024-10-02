#ifndef _APP_PRIV_H_
#define _APP_PRIV_H_

#include <stdbool.h>
#include <esp_err.h>

#define DEFAULT_POWER false
#define NUM_OF_SWITCHES 4
extern int8_t g_relay_state[NUM_OF_SWITCHES];
extern int8_t g_button_state[NUM_OF_SWITCHES];

extern esp_rmaker_device_t *switch_devices[NUM_OF_SWITCHES];
void app_gpio_init(void);
void app_driver_init(void);
int app_driver_set_state(int switch_num, bool state);
bool app_driver_get_state(int switch_num);
esp_err_t load_state();

#endif