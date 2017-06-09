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
#include "esp_system.h"
#include "nvs_flash.h"
#include "driver/uart.h"
#include "freertos/queue.h"
#include "soc/uart_struct.h"
#include "dht22.h"
#include "bt.h"

#define EXT_WAKEUP_PIN 25
#define DHT_PIN 26
#define BUF_SIZE (1024)
#define EX_UART_NUM UART_NUM_0

static QueueHandle_t uart0_queue;

static RTC_DATA_ATTR struct timeval sleep_enter_time;
static const char *tag_ble = "BLE_ADV";

// Default time between two timer wakeups
#define WAKEUP_TIME_MIN 24
#define WAKEUP_TIME_MS (WAKEUP_TIME_MIN * 60UL * 1000UL)

// Time counter between two reported detection
// Must be stored in RTC slow memory(kept in sleep mode)
static size_t RTC_DATA_ATTR ts_counter = WAKEUP_TIME_MS;

static void write_commands();

int temp, hum;

void app_main()
{
    uart_config_t uart_config = {
        .baud_rate = 9600,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .rx_flow_ctrl_thresh = 122,
    };

    uart_param_config(EX_UART_NUM, &uart_config);
    uart_set_pin(EX_UART_NUM, 4, 5, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    uart_driver_install(EX_UART_NUM, BUF_SIZE * 2, BUF_SIZE * 2, 10, &uart0_queue, 0);

    set_DHT_pin(DHT_PIN);
    hum = (int)get_hum();
    temp = (int)get_tempc();

    struct timeval now;
    gettimeofday(&now, NULL);
    int sleep_time_ms = (now.tv_sec - sleep_enter_time.tv_sec) * 1000
        + (now.tv_usec - sleep_enter_time.tv_usec) / 1000;

    switch (esp_deep_sleep_get_wakeup_cause()) {
        case ESP_DEEP_SLEEP_WAKEUP_EXT0: {

            printf("\n%s %d\n%s %d%s\n%s\n", "Wake up from GPIO ", EXT_WAKEUP_PIN,
                    "Time spent in deep sleep: ", sleep_time_ms, "ms",
                    "MOTION DETECTED!");

            if (ts_counter >= (WAKEUP_TIME_MS)) {

                xTaskCreate(write_commands, "write_commands", 2048, NULL, 12, NULL);

                printf("%s\n", "REPORTED!");
                ts_counter = 0;
            }
            else {
                ts_counter += sleep_time_ms;
            }

            printf("%s %d\n%s\n", "Last report: ", ts_counter,
                    "Delay to avoid multiple detection of the same motion");

            vTaskDelay(4000 / portTICK_PERIOD_MS);

            break;
        }

        case ESP_DEEP_SLEEP_WAKEUP_TIMER: {

            printf("\nWake up from timer.\nTime spent in deep sleep: %dms\n",
                    sleep_time_ms);
            xTaskCreate(write_commands, "write_commands", 2048, NULL, 12, NULL);
            break;
        }

        case ESP_DEEP_SLEEP_WAKEUP_UNDEFINED:
        default:
            printf("Not a deep sleep reset\n");
    }

    vTaskDelay(1000 / portTICK_PERIOD_MS);

    // Timer wakeup
    printf("Enabling timer wakeup, %dm\n", WAKEUP_TIME_MIN);
    esp_deep_sleep_enable_timer_wakeup(WAKEUP_TIME_MS * 1000); // microsec here !

    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();

    if (esp_bt_controller_init(&bt_cfg) != ESP_OK) {
        ESP_LOGI(tag_ble, "Bluetooth controller initialize failed");
        return;
    }

    if (esp_bt_controller_enable(ESP_BT_MODE_BTDM) != ESP_OK) {
        ESP_LOGI(tag_ble, "Bluetooth controller enable failed");
        return;
    }

    // GPIO wakeup
    printf("Enabling EXT1 wakeup on pins GPIO%d\n", EXT_WAKEUP_PIN);
    esp_deep_sleep_enable_ext0_wakeup(EXT_WAKEUP_PIN, 1);

    printf("Entering deep sleep\n");
    gettimeofday(&sleep_enter_time, NULL);

    esp_deep_sleep_start();
}

static void write_commands()
{
    char msg[15];
    sprintf(msg, "AT$SS=%d %d %1.2d\r", temp, hum, 1);

    uart_write_bytes(EX_UART_NUM, msg, strlen(msg));

    vTaskDelay(1000 / portTICK_PERIOD_MS);

    vTaskDelete(NULL);
}
