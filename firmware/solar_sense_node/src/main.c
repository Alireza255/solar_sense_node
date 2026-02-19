#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "nvs_flash.h"
#include "esp_log.h"

#include "driver/gpio.h"
#include "esp_http_server.h"

#define WIFI_SSID      "ESP32_Test"
#define WIFI_PASS      "12345678"
#define MAX_STA_CONN   4
#define LED_GPIO       GPIO_NUM_2   // Built-in LED

static const char *TAG = "WIFI_LED";

// HTML page served to phone
static const char* html_page = "<!DOCTYPE html>"
"<html>"
"<head><title>ESP32 LED Control</title></head>"
"<body>"
"<h2>LED Control</h2>"
"<button onclick=\"fetch('/on')\">ON</button>"
"<button onclick=\"fetch('/off')\">OFF</button>"
"</body>"
"</html>";

// LED control handlers
esp_err_t led_on_handler(httpd_req_t *req) {
    gpio_set_level(LED_GPIO, 1);
    httpd_resp_sendstr(req, "LED ON");
    return ESP_OK;
}

esp_err_t led_off_handler(httpd_req_t *req) {
    gpio_set_level(LED_GPIO, 0);
    httpd_resp_sendstr(req, "LED OFF");
    return ESP_OK;
}

esp_err_t root_get_handler(httpd_req_t *req) {
    httpd_resp_sendstr(req, html_page);
    return ESP_OK;
}

// Setup HTTP server
httpd_handle_t start_webserver(void) {
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    httpd_handle_t server = NULL;
    if (httpd_start(&server, &config) == ESP_OK) {
        // URI handlers
        httpd_uri_t root = {
            .uri       = "/",
            .method    = HTTP_GET,
            .handler   = root_get_handler,
            .user_ctx  = NULL
        };
        httpd_register_uri_handler(server, &root);

        httpd_uri_t on_uri = {
            .uri       = "/on",
            .method    = HTTP_GET,
            .handler   = led_on_handler,
            .user_ctx  = NULL
        };
        httpd_register_uri_handler(server, &on_uri);

        httpd_uri_t off_uri = {
            .uri       = "/off",
            .method    = HTTP_GET,
            .handler   = led_off_handler,
            .user_ctx  = NULL
        };
        httpd_register_uri_handler(server, &off_uri);
    }
    return server;
}

// Wi-Fi AP setup (same as before)
static void wifi_event_handler(void* arg,
                               esp_event_base_t event_base,
                               int32_t event_id,
                               void* event_data) {
    // Optional: LED blink on Wi-Fi connect
}

void wifi_init_softap(void) {
    esp_netif_init();
    esp_event_loop_create_default();
    esp_netif_create_default_wifi_ap();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);

    wifi_config_t wifi_config = {
        .ap = {
            .ssid = WIFI_SSID,
            .ssid_len = strlen(WIFI_SSID),
            .password = WIFI_PASS,
            .max_connection = MAX_STA_CONN,
            .authmode = WIFI_AUTH_WPA_WPA2_PSK,
            .channel = 1
        },
    };

    if (strlen(WIFI_PASS) == 0) wifi_config.ap.authmode = WIFI_AUTH_OPEN;

    esp_wifi_set_mode(WIFI_MODE_AP);
    esp_wifi_set_config(WIFI_IF_AP, &wifi_config);
    esp_wifi_start();

    ESP_LOGI(TAG, "Wi-Fi AP started. SSID:%s password:%s", WIFI_SSID, WIFI_PASS);
}

// Main application
void app_main(void) {
    // NVS
    nvs_flash_init();

    // LED
    gpio_reset_pin(LED_GPIO);
    gpio_set_direction(LED_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_level(LED_GPIO, 0);

    // Start Wi-Fi AP
    wifi_init_softap();

    // Start web server
    start_webserver();

    while(1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
