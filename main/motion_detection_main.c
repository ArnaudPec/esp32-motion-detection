#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_deep_sleep.h"
#include "esp_log.h"
#include "driver/touch_pad.h"
#include "driver/adc.h"
#include "driver/rtc_io.h"
#include "soc/rtc_cntl_reg.h"
#include "soc/sens_reg.h"
#include "soc/rtc.h"
#include "bt.h"

#define EXT_WAKEUP_PIN 25

static RTC_DATA_ATTR struct timeval sleep_enter_time;
static const char *tag = "BLE_ADV";

void app_main()
{
    struct timeval now;
    gettimeofday(&now, NULL);

    switch (esp_deep_sleep_get_wakeup_cause()) {
        case ESP_DEEP_SLEEP_WAKEUP_EXT0: {
                printf("Wake up from GPIO %d\n", EXT_WAKEUP_PIN);
                printf("%s\n", "Motion detected!");
            break;
        }
        case ESP_DEEP_SLEEP_WAKEUP_UNDEFINED:
        default:
            printf("Not a deep sleep reset\n");
    }

    vTaskDelay(1000 / portTICK_PERIOD_MS);

    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();

    if (esp_bt_controller_init(&bt_cfg) != ESP_OK) {
        ESP_LOGI(tag, "Bluetooth controller initialize failed");
        return;
    }

    if (esp_bt_controller_enable(ESP_BT_MODE_BTDM) != ESP_OK) {
        ESP_LOGI(tag, "Bluetooth controller enable failed");
        return;
    }

    printf("Enabling EXT1 wakeup on pins GPIO%d\n", EXT_WAKEUP_PIN);
    esp_deep_sleep_enable_ext0_wakeup(EXT_WAKEUP_PIN, 1);

    printf("Entering deep sleep\n");
    gettimeofday(&sleep_enter_time, NULL);

    esp_deep_sleep_start();
}
