#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <stdint.h>

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/semphr.h"
#include "freertos/task.h"

#include "esp_crt_bundle.h"
#include "esp_err.h"
#include "esp_event.h"
#include "esp_http_client.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_sntp.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "nvs_flash.h"
#include "cJSON.h"

#include "bsp/esp-bsp.h"
#include "lvgl.h"
#include "sdkconfig.h"

#define DISPLAY_WIDTH 466
#define WEATHER_REFRESH_SECONDS (30 * 60)
#define HTTP_RESPONSE_LIMIT 8192
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_STARTED_BIT BIT1

static const char *TAG = "iris";

typedef struct {
    char date[11];
    float minimum;
    float maximum;
    int weather_code;
} forecast_day_t;

typedef struct {
    bool valid;
    float temperature;
    int humidity;
    float wind_speed;
    int weather_code;
    forecast_day_t days[3];
    time_t updated_at;
    char status[96];
} weather_snapshot_t;

typedef struct {
    char data[HTTP_RESPONSE_LIMIT + 1];
    size_t length;
} http_buffer_t;

typedef enum { VIEW_CLOCK = 0, VIEW_WEATHER } app_view_t;

typedef struct {
    EventGroupHandle_t events;
    SemaphoreHandle_t weather_mutex;
    weather_snapshot_t weather;
    volatile bool refresh_requested;
    bool wifi_configured;
    bool wifi_scanning;
    bool time_sync_started;
    esp_err_t wifi_scan_error;
    char wifi_ssid[33];
    char wifi_password[65];
    char location_name[48];
    char latitude[16];
    char longitude[16];
    char timezone[64];
    wifi_ap_record_t *networks;
    uint16_t network_count;
    char selected_ssid[33];
    bool selected_network_is_open;
    app_view_t view;
    lv_obj_t *screen;
    lv_obj_t *time_label;
    lv_obj_t *date_label;
    lv_obj_t *location_label;
    lv_obj_t *connection_label;
    lv_obj_t *clock_panel;
    lv_obj_t *weather_panel;
    lv_obj_t *temperature_label;
    lv_obj_t *condition_label;
    lv_obj_t *details_label;
    lv_obj_t *forecast_label;
    lv_obj_t *hint_label;
    lv_obj_t *wifi_button;
    lv_obj_t *setup_overlay;
    lv_obj_t *setup_status_label;
    lv_obj_t *password_textarea;
    lv_obj_t *password_keyboard;
} app_context_t;

static app_context_t app;

static void start_wifi_scan(void);
static void show_wifi_picker(void);
static void wifi_network_event(lv_event_t *event);
static void wifi_rescan_event(lv_event_t *event);
static void wifi_cancel_event(lv_event_t *event);
static void wifi_connect_event(lv_event_t *event);
static void password_keyboard_event(lv_event_t *event);

static void copy_config_string(char *destination, size_t destination_size,
                               const char *source)
{
    snprintf(destination, destination_size, "%s", source ? source : "");
}

static void load_runtime_config(void)
{
    copy_config_string(app.wifi_ssid, sizeof(app.wifi_ssid), CONFIG_IRIS_WIFI_SSID);
    copy_config_string(app.wifi_password, sizeof(app.wifi_password), CONFIG_IRIS_WIFI_PASSWORD);
    copy_config_string(app.location_name, sizeof(app.location_name), CONFIG_IRIS_LOCATION_NAME);
    copy_config_string(app.latitude, sizeof(app.latitude), CONFIG_IRIS_LATITUDE);
    copy_config_string(app.longitude, sizeof(app.longitude), CONFIG_IRIS_LONGITUDE);
    copy_config_string(app.timezone, sizeof(app.timezone), CONFIG_IRIS_TIMEZONE);
    const bool use_build_wifi = app.wifi_ssid[0] != '\0';

    nvs_handle_t storage;
    if (nvs_open("iris", NVS_READONLY, &storage) != ESP_OK) return;
    size_t length;
    if (!use_build_wifi && nvs_get_str(storage, "ssid", NULL, &length) == ESP_OK && length <= sizeof(app.wifi_ssid))
        nvs_get_str(storage, "ssid", app.wifi_ssid, &length);
    if (!use_build_wifi && nvs_get_str(storage, "password", NULL, &length) == ESP_OK && length <= sizeof(app.wifi_password))
        nvs_get_str(storage, "password", app.wifi_password, &length);
    if (nvs_get_str(storage, "location", NULL, &length) == ESP_OK && length <= sizeof(app.location_name))
        nvs_get_str(storage, "location", app.location_name, &length);
    if (nvs_get_str(storage, "latitude", NULL, &length) == ESP_OK && length <= sizeof(app.latitude))
        nvs_get_str(storage, "latitude", app.latitude, &length);
    if (nvs_get_str(storage, "longitude", NULL, &length) == ESP_OK && length <= sizeof(app.longitude))
        nvs_get_str(storage, "longitude", app.longitude, &length);
    if (nvs_get_str(storage, "timezone", NULL, &length) == ESP_OK && length <= sizeof(app.timezone))
        nvs_get_str(storage, "timezone", app.timezone, &length);
    nvs_close(storage);
}

static esp_err_t save_runtime_config(const char *ssid, const char *password,
                                     const char *location, const char *latitude,
                                     const char *longitude, const char *timezone)
{
    nvs_handle_t storage;
    esp_err_t err = nvs_open("iris", NVS_READWRITE, &storage);
    if (err != ESP_OK) return err;
    err = nvs_set_str(storage, "ssid", ssid);
    if (err == ESP_OK) err = nvs_set_str(storage, "password", password);
    if (err == ESP_OK) err = nvs_set_str(storage, "location", location);
    if (err == ESP_OK) err = nvs_set_str(storage, "latitude", latitude);
    if (err == ESP_OK) err = nvs_set_str(storage, "longitude", longitude);
    if (err == ESP_OK) err = nvs_set_str(storage, "timezone", timezone);
    if (err == ESP_OK) err = nvs_commit(storage);
    nvs_close(storage);
    return err;
}

static const char *weather_description(int code)
{
    if (code == 0) return "Clear sky";
    if (code <= 3) return "Partly cloudy";
    if (code == 45 || code == 48) return "Foggy";
    if (code >= 51 && code <= 57) return "Drizzle";
    if (code >= 61 && code <= 67) return "Rain";
    if (code >= 71 && code <= 77) return "Snow";
    if (code >= 80 && code <= 82) return "Rain showers";
    if (code >= 85 && code <= 86) return "Snow showers";
    if (code >= 95 && code <= 99) return "Thunderstorm";
    return "Unknown conditions";
}

static const char *weather_short(int code)
{
    if (code == 0) return "Clear";
    if (code <= 3) return "Cloudy";
    if (code == 45 || code == 48) return "Fog";
    if (code >= 51 && code <= 67) return "Rain";
    if (code >= 71 && code <= 77) return "Snow";
    if (code >= 80 && code <= 82) return "Showers";
    if (code >= 85 && code <= 86) return "Snow";
    if (code >= 95) return "Storm";
    return "--";
}

static bool json_number(cJSON *object, const char *name, double *value)
{
    cJSON *item = cJSON_GetObjectItemCaseSensitive(object, name);
    if (!cJSON_IsNumber(item)) return false;
    *value = item->valuedouble;
    return true;
}

static bool json_array_number(cJSON *array, int index, double *value)
{
    cJSON *item = cJSON_GetArrayItem(array, index);
    if (!cJSON_IsNumber(item)) return false;
    *value = item->valuedouble;
    return true;
}

static bool parse_weather_json(const char *payload, weather_snapshot_t *out)
{
    cJSON *root = cJSON_ParseWithLength(payload, strlen(payload));
    if (root == NULL) return false;

    bool ok = false;
    cJSON *current = cJSON_GetObjectItemCaseSensitive(root, "current");
    cJSON *daily = cJSON_GetObjectItemCaseSensitive(root, "daily");
    cJSON *daily_time = daily ? cJSON_GetObjectItemCaseSensitive(daily, "time") : NULL;
    cJSON *daily_min = daily ? cJSON_GetObjectItemCaseSensitive(daily, "temperature_2m_min") : NULL;
    cJSON *daily_max = daily ? cJSON_GetObjectItemCaseSensitive(daily, "temperature_2m_max") : NULL;
    cJSON *daily_code = daily ? cJSON_GetObjectItemCaseSensitive(daily, "weather_code") : NULL;
    double number;

    if (current == NULL || daily == NULL || !json_number(current, "temperature_2m", &number)) goto done;
    out->temperature = (float)number;
    if (json_number(current, "relative_humidity_2m", &number)) out->humidity = (int)number;
    if (json_number(current, "wind_speed_10m", &number)) out->wind_speed = (float)number;
    if (json_number(current, "weather_code", &number)) out->weather_code = (int)number;

    if (!cJSON_IsArray(daily_time) || !cJSON_IsArray(daily_min) ||
        !cJSON_IsArray(daily_max) || !cJSON_IsArray(daily_code)) goto done;
    for (int i = 0; i < 3; i++) {
        cJSON *date = cJSON_GetArrayItem(daily_time, i);
        if (!cJSON_IsString(date) || date->valuestring == NULL) goto done;
        snprintf(out->days[i].date, sizeof(out->days[i].date), "%.10s", date->valuestring);
        if (!json_array_number(daily_min, i, &number)) goto done;
        out->days[i].minimum = (float)number;
        if (!json_array_number(daily_max, i, &number)) goto done;
        out->days[i].maximum = (float)number;
        if (!json_array_number(daily_code, i, &number)) goto done;
        out->days[i].weather_code = (int)number;
    }
    out->valid = true;
    out->updated_at = time(NULL);
    snprintf(out->status, sizeof(out->status), "Updated just now");
    ok = true;

done:
    cJSON_Delete(root);
    return ok;
}

static esp_err_t http_event_handler(esp_http_client_event_t *event)
{
    http_buffer_t *buffer = (http_buffer_t *)event->user_data;
    if (event->event_id == HTTP_EVENT_ON_DATA && buffer != NULL && event->data_len > 0) {
        size_t available = HTTP_RESPONSE_LIMIT - buffer->length;
        size_t copy_length = (size_t)event->data_len < available ? (size_t)event->data_len : available;
        memcpy(buffer->data + buffer->length, event->data, copy_length);
        buffer->length += copy_length;
        buffer->data[buffer->length] = '\0';
    }
    return ESP_OK;
}

static bool fetch_weather(weather_snapshot_t *snapshot)
{
    char url[512];
    snprintf(url, sizeof(url),
             "https://api.open-meteo.com/v1/forecast?latitude=%s&longitude=%s"
             "&current=temperature_2m,relative_humidity_2m,weather_code,wind_speed_10m"
             "&daily=weather_code,temperature_2m_max,temperature_2m_min&forecast_days=3&timezone=auto",
             app.latitude, app.longitude);

    /* This payload is deliberately heap-backed: it is larger than a safe task stack frame. */
    http_buffer_t *buffer = calloc(1, sizeof(*buffer));
    if (buffer == NULL) return false;
    esp_http_client_config_t config = {
        .url = url,
        .method = HTTP_METHOD_GET,
        .event_handler = http_event_handler,
        .user_data = buffer,
        .timeout_ms = 15000,
        .buffer_size = 2048,
        .crt_bundle_attach = esp_crt_bundle_attach,
        .keep_alive_enable = true,
    };
    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (client == NULL) {
        free(buffer);
        return false;
    }
    esp_err_t err = esp_http_client_perform(client);
    int status = esp_http_client_get_status_code(client);
    esp_http_client_cleanup(client);
    weather_snapshot_t parsed = {0};
    bool success = err == ESP_OK && status == 200 && buffer->length > 0 &&
                   parse_weather_json(buffer->data, &parsed);
    free(buffer);
    if (!success) return false;
    *snapshot = parsed;
    return true;
}

static void set_weather_status(const char *status)
{
    if (xSemaphoreTake(app.weather_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        snprintf(app.weather.status, sizeof(app.weather.status), "%s", status);
        xSemaphoreGive(app.weather_mutex);
    }
}

static void weather_task(void *arg)
{
    (void)arg;
    TickType_t next_fetch = 0;
    for (;;) {
        bool connected = (xEventGroupGetBits(app.events) & WIFI_CONNECTED_BIT) != 0;
        TickType_t now = xTaskGetTickCount();
        if (app.wifi_configured && connected && (app.refresh_requested || now >= next_fetch)) {
            app.refresh_requested = false;
            set_weather_status("Fetching weather...");
            weather_snapshot_t result = {0};
            if (fetch_weather(&result)) {
                if (xSemaphoreTake(app.weather_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
                    app.weather = result;
                    xSemaphoreGive(app.weather_mutex);
                }
                next_fetch = now + pdMS_TO_TICKS(WEATHER_REFRESH_SECONDS * 1000);
                ESP_LOGI(TAG, "Weather refreshed");
            } else {
                set_weather_status(app.weather.valid ? "Refresh failed; showing last reading" : "Weather request failed");
                next_fetch = now + pdMS_TO_TICKS(60 * 1000);
                ESP_LOGW(TAG, "Weather request failed");
            }
        } else if (!app.wifi_configured) {
            set_weather_status("Choose Wi-Fi on Iris");
        } else if (!connected) {
            set_weather_status("Waiting for Wi-Fi...");
        }
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

static void wifi_event_handler(void *arg, esp_event_base_t event_base,
                               int32_t event_id, void *event_data)
{
    (void)arg;
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        xEventGroupSetBits(app.events, WIFI_STARTED_BIT);
        if (app.wifi_configured) {
            esp_err_t err = esp_wifi_connect();
            if (err != ESP_OK) ESP_LOGW(TAG, "Could not start Wi-Fi connection: %s", esp_err_to_name(err));
        }
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        xEventGroupClearBits(app.events, WIFI_CONNECTED_BIT);
        const wifi_event_sta_disconnected_t *disconnected = event_data;
        ESP_LOGW(TAG, "Wi-Fi disconnected (reason %d)", disconnected ? disconnected->reason : -1);
        if (app.wifi_configured) {
            esp_err_t err = esp_wifi_connect();
            if (err != ESP_OK) ESP_LOGW(TAG, "Could not retry Wi-Fi: %s", esp_err_to_name(err));
        }
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        xEventGroupSetBits(app.events, WIFI_CONNECTED_BIT);
        ESP_LOGI(TAG, "Wi-Fi connected");
    }
}

static void init_network_stack(void)
{
    ESP_ERROR_CHECK(esp_netif_init());
    esp_err_t event_result = esp_event_loop_create_default();
    if (event_result != ESP_OK && event_result != ESP_ERR_INVALID_STATE) ESP_ERROR_CHECK(event_result);
    wifi_init_config_t wifi_init_config = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&wifi_init_config));
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL));
    esp_netif_create_default_wifi_sta();
}

static void set_station_credentials(void)
{
    wifi_config_t wifi_config = {0};
    size_t ssid_length = strlen(app.wifi_ssid);
    if (ssid_length > sizeof(wifi_config.sta.ssid)) ssid_length = sizeof(wifi_config.sta.ssid);
    memcpy(wifi_config.sta.ssid, app.wifi_ssid, ssid_length);
    size_t password_length = strlen(app.wifi_password);
    if (password_length > sizeof(wifi_config.sta.password)) password_length = sizeof(wifi_config.sta.password);
    memcpy(wifi_config.sta.password, app.wifi_password, password_length);
    wifi_config.sta.threshold.authmode = WIFI_AUTH_OPEN;
    wifi_config.sta.pmf_cfg.capable = true;
    wifi_config.sta.pmf_cfg.required = false;
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
}

static void start_wifi(void)
{
    app.wifi_configured = app.wifi_ssid[0] != '\0';
    init_network_stack();
    if (app.wifi_configured) set_station_credentials();
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());
    if (!app.wifi_configured) start_wifi_scan();
}

static void start_time_sync(void)
{
    setenv("TZ", app.timezone, 1);
    tzset();
    if (!app.wifi_configured || app.time_sync_started) return;
    esp_sntp_setoperatingmode(SNTP_OPMODE_POLL);
    esp_sntp_setservername(0, "pool.ntp.org");
    esp_sntp_init();
    app.time_sync_started = true;
}

static lv_obj_t *make_label(lv_obj_t *parent, const lv_font_t *font, lv_color_t color)
{
    lv_obj_t *label = lv_label_create(parent);
    lv_obj_set_style_text_font(label, font, LV_PART_MAIN);
    lv_obj_set_style_text_color(label, color, LV_PART_MAIN);
    lv_label_set_long_mode(label, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(label, DISPLAY_WIDTH - 56);
    return label;
}

static lv_obj_t *make_button(lv_obj_t *parent, const char *text, int width)
{
    lv_obj_t *button = lv_button_create(parent);
    lv_obj_set_size(button, width, 42);
    lv_obj_set_style_bg_color(button, lv_color_hex(0x16324f), LV_PART_MAIN);
    lv_obj_t *label = lv_label_create(button);
    lv_label_set_text(label, text);
    lv_obj_center(label);
    lv_obj_set_style_text_color(label, lv_color_hex(0xf3f7ff), LV_PART_MAIN);
    return button;
}

static void close_wifi_overlay(void)
{
    if (app.setup_overlay != NULL) {
        lv_obj_delete(app.setup_overlay);
        app.setup_overlay = NULL;
    }
    app.setup_status_label = NULL;
    app.password_textarea = NULL;
    app.password_keyboard = NULL;
}

static void show_wifi_picker(void)
{
    close_wifi_overlay();
    const lv_color_t primary = lv_color_hex(0xf3f7ff);
    const lv_color_t secondary = lv_color_hex(0x9db4cf);
    app.setup_overlay = lv_obj_create(app.screen);
    lv_obj_set_size(app.setup_overlay, 430, 410);
    lv_obj_center(app.setup_overlay);
    lv_obj_set_style_bg_color(app.setup_overlay, lv_color_hex(0x0d1c2d), LV_PART_MAIN);
    lv_obj_set_style_border_color(app.setup_overlay, lv_color_hex(0x58d7c4), LV_PART_MAIN);
    lv_obj_set_style_radius(app.setup_overlay, 16, LV_PART_MAIN);

    lv_obj_t *title = make_label(app.setup_overlay, &lv_font_montserrat_24, primary);
    lv_label_set_text(title, "Choose Wi-Fi");
    lv_obj_set_width(title, 380);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 10);
    lv_obj_set_style_text_align(title, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);

    app.setup_status_label = make_label(app.setup_overlay, &lv_font_montserrat_16, secondary);
    lv_obj_set_width(app.setup_status_label, 380);
    lv_obj_align(app.setup_status_label, LV_ALIGN_TOP_MID, 0, 42);
    lv_obj_set_style_text_align(app.setup_status_label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    if (app.wifi_scan_error != ESP_OK) {
        lv_label_set_text(app.setup_status_label, "Scan failed. Tap Rescan.");
    } else if (app.network_count == 0) {
        lv_label_set_text(app.setup_status_label, "No networks found. Try scanning again.");
    } else {
        lv_label_set_text_fmt(app.setup_status_label, "%u network%s found", app.network_count,
                              app.network_count == 1 ? "" : "s");
    }

    lv_obj_t *list = lv_list_create(app.setup_overlay);
    lv_obj_set_size(list, 390, 250);
    lv_obj_align(list, LV_ALIGN_TOP_MID, 0, 72);
    lv_obj_set_style_bg_color(list, lv_color_hex(0x07111f), LV_PART_MAIN);
    lv_obj_set_style_pad_all(list, 6, LV_PART_MAIN);
    for (uint16_t index = 0; index < app.network_count; index++) {
        lv_obj_t *network = lv_button_create(list);
        lv_obj_set_width(network, LV_PCT(100));
        lv_obj_set_height(network, 43);
        lv_obj_set_style_bg_color(network, lv_color_hex(0x16324f), LV_PART_MAIN);
        lv_obj_t *label = lv_label_create(network);
        lv_label_set_text_fmt(label, "%s  %s  %d dBm", (const char *)app.networks[index].ssid,
                              app.networks[index].authmode == WIFI_AUTH_OPEN ? "Open" : "Locked",
                              app.networks[index].rssi);
        lv_obj_set_width(label, LV_PCT(94));
        lv_obj_align(label, LV_ALIGN_LEFT_MID, 8, 0);
        lv_label_set_long_mode(label, LV_LABEL_LONG_DOT);
        lv_obj_add_event_cb(network, wifi_network_event, LV_EVENT_CLICKED, (void *)(intptr_t)index);
    }

    lv_obj_t *rescan = make_button(app.setup_overlay, "Rescan", 120);
    lv_obj_align(rescan, LV_ALIGN_BOTTOM_LEFT, 10, -8);
    lv_obj_add_event_cb(rescan, wifi_rescan_event, LV_EVENT_CLICKED, NULL);
    lv_obj_t *cancel = make_button(app.setup_overlay, "Close", 120);
    lv_obj_align(cancel, LV_ALIGN_BOTTOM_RIGHT, -10, -8);
    lv_obj_add_event_cb(cancel, wifi_cancel_event, LV_EVENT_CLICKED, NULL);
}

static void show_password_prompt(void)
{
    close_wifi_overlay();
    const lv_color_t primary = lv_color_hex(0xf3f7ff);
    const lv_color_t secondary = lv_color_hex(0x9db4cf);
    app.setup_overlay = lv_obj_create(app.screen);
    lv_obj_set_size(app.setup_overlay, 446, 446);
    lv_obj_center(app.setup_overlay);
    lv_obj_set_style_bg_color(app.setup_overlay, lv_color_hex(0x0d1c2d), LV_PART_MAIN);
    lv_obj_set_style_border_color(app.setup_overlay, lv_color_hex(0x58d7c4), LV_PART_MAIN);
    lv_obj_set_style_radius(app.setup_overlay, 16, LV_PART_MAIN);

    lv_obj_t *title = make_label(app.setup_overlay, &lv_font_montserrat_24, primary);
    lv_label_set_text(title, "Connect to Wi-Fi");
    lv_obj_set_width(title, 400);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 10);
    lv_obj_set_style_text_align(title, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    app.setup_status_label = make_label(app.setup_overlay, &lv_font_montserrat_16, secondary);
    lv_label_set_text(app.setup_status_label, app.selected_ssid);
    lv_obj_set_width(app.setup_status_label, 400);
    lv_obj_align(app.setup_status_label, LV_ALIGN_TOP_MID, 0, 43);
    lv_obj_set_style_text_align(app.setup_status_label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);

    app.password_textarea = lv_textarea_create(app.setup_overlay);
    lv_obj_set_size(app.password_textarea, 395, 46);
    lv_obj_align(app.password_textarea, LV_ALIGN_TOP_MID, 0, 78);
    lv_textarea_set_one_line(app.password_textarea, true);
    lv_textarea_set_max_length(app.password_textarea, 64);
    lv_textarea_set_placeholder_text(app.password_textarea,
                                     app.selected_network_is_open ? "Open network — no password needed" : "Wi-Fi password");
    lv_textarea_set_password_mode(app.password_textarea, !app.selected_network_is_open);
    if (app.selected_network_is_open) lv_obj_add_state(app.password_textarea, LV_STATE_DISABLED);

    lv_obj_t *connect = make_button(app.setup_overlay, "Connect", 132);
    lv_obj_align(connect, LV_ALIGN_TOP_RIGHT, -22, 133);
    lv_obj_add_event_cb(connect, wifi_connect_event, LV_EVENT_CLICKED, NULL);
    lv_obj_t *back = make_button(app.setup_overlay, "Back", 100);
    lv_obj_align(back, LV_ALIGN_TOP_LEFT, 22, 133);
    lv_obj_add_event_cb(back, wifi_cancel_event, LV_EVENT_CLICKED, NULL);

    app.password_keyboard = lv_keyboard_create(app.setup_overlay);
    lv_obj_set_size(app.password_keyboard, 420, 240);
    lv_obj_align(app.password_keyboard, LV_ALIGN_BOTTOM_MID, 0, -8);
    lv_keyboard_set_textarea(app.password_keyboard, app.password_textarea);
    lv_obj_add_event_cb(app.password_keyboard, password_keyboard_event, LV_EVENT_READY, NULL);
    lv_obj_add_event_cb(app.password_keyboard, password_keyboard_event, LV_EVENT_CANCEL, NULL);
}

static void wifi_scan_task(void *arg)
{
    (void)arg;
    EventBits_t bits = xEventGroupWaitBits(app.events, WIFI_STARTED_BIT, pdFALSE, pdTRUE,
                                            pdMS_TO_TICKS(5000));
    wifi_scan_config_t scan_config = {0};
    esp_err_t err = (bits & WIFI_STARTED_BIT) ? esp_wifi_scan_start(&scan_config, true) : ESP_ERR_TIMEOUT;
    app.network_count = 0;
    if (err == ESP_OK) {
        uint16_t record_count = 0;
        err = esp_wifi_scan_get_ap_num(&record_count);
        if (err == ESP_OK && record_count > 0) {
            wifi_ap_record_t *records = calloc(record_count, sizeof(*records));
            if (records == NULL) {
                err = ESP_ERR_NO_MEM;
            } else {
                err = esp_wifi_scan_get_ap_records(&record_count, records);
            }
            if (err == ESP_OK) {
                free(app.networks);
                app.networks = records;
                for (uint16_t record = 0; record < record_count; record++) {
                    if (records[record].ssid[0] == '\0') continue;
                    bool duplicate = false;
                    for (uint16_t saved = 0; saved < app.network_count; saved++) {
                        if (strcmp((const char *)records[record].ssid,
                                   (const char *)app.networks[saved].ssid) == 0) {
                            duplicate = true;
                            break;
                        }
                    }
                    if (!duplicate) {
                        app.networks[app.network_count++] = records[record];
                    }
                }
            } else {
                free(records);
            }
        }
    }
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "Wi-Fi scan failed: %s", esp_err_to_name(err));
    }
    app.wifi_scanning = false;
    app.wifi_scan_error = err;
    set_weather_status(err == ESP_OK ? "Choose a Wi-Fi network" : "Wi-Fi scan failed; try again");
    if (bsp_display_lock(UINT32_MAX) == ESP_OK) {
        show_wifi_picker();
        bsp_display_unlock();
    }
    vTaskDelete(NULL);
}

static void start_wifi_scan(void)
{
    if (app.wifi_scanning) return;
    app.wifi_scanning = true;
    app.wifi_scan_error = ESP_OK;
    set_weather_status("Scanning for Wi-Fi...");
    if (xTaskCreate(wifi_scan_task, "wifi_scan", 6144, NULL, 4, NULL) != pdPASS) {
        app.wifi_scanning = false;
        set_weather_status("Could not start Wi-Fi scan");
    }
}

static void wifi_network_event(lv_event_t *event)
{
    uint16_t index = (uint16_t)(intptr_t)lv_event_get_user_data(event);
    if (index >= app.network_count) return;
    copy_config_string(app.selected_ssid, sizeof(app.selected_ssid), (const char *)app.networks[index].ssid);
    app.selected_network_is_open = app.networks[index].authmode == WIFI_AUTH_OPEN;
    show_password_prompt();
}

static void wifi_rescan_event(lv_event_t *event)
{
    (void)event;
    close_wifi_overlay();
    start_wifi_scan();
}

static void wifi_cancel_event(lv_event_t *event)
{
    (void)event;
    close_wifi_overlay();
}

static void wifi_connect_event(lv_event_t *event)
{
    (void)event;
    const char *password = app.selected_network_is_open ? "" : lv_textarea_get_text(app.password_textarea);
    if (save_runtime_config(app.selected_ssid, password, app.location_name, app.latitude,
                            app.longitude, app.timezone) != ESP_OK) {
        lv_label_set_text(app.setup_status_label, "Could not save Wi-Fi settings");
        return;
    }
    copy_config_string(app.wifi_ssid, sizeof(app.wifi_ssid), app.selected_ssid);
    copy_config_string(app.wifi_password, sizeof(app.wifi_password), password);
    app.wifi_configured = true;
    set_station_credentials();
    start_time_sync();
    close_wifi_overlay();
    lv_label_set_text_fmt(app.connection_label, "Connecting to %s...", app.wifi_ssid);
    esp_wifi_disconnect();
    esp_wifi_connect();
}

static void password_keyboard_event(lv_event_t *event)
{
    if (lv_event_get_code(event) == LV_EVENT_READY) {
        wifi_connect_event(event);
    } else if (lv_event_get_code(event) == LV_EVENT_CANCEL) {
        show_wifi_picker();
    }
}

static void refresh_button_event(lv_event_t *event)
{
    if (lv_event_get_code(event) == LV_EVENT_CLICKED) {
        app.refresh_requested = true;
        if (app.connection_label != NULL) lv_label_set_text(app.connection_label, "Refreshing...");
    }
}

static void wifi_button_event(lv_event_t *event)
{
    (void)event;
    start_wifi_scan();
}

static void screen_event(lv_event_t *event)
{
    if (lv_event_get_code(event) != LV_EVENT_CLICKED) return;
    if (app.view == VIEW_CLOCK) {
        app.view = VIEW_WEATHER;
        lv_obj_add_flag(app.clock_panel, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(app.weather_panel, LV_OBJ_FLAG_HIDDEN);
        lv_label_set_text(app.hint_label, "Tap anywhere for the clock");
    } else {
        app.view = VIEW_CLOCK;
        lv_obj_clear_flag(app.clock_panel, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(app.weather_panel, LV_OBJ_FLAG_HIDDEN);
        lv_label_set_text(app.hint_label, "Tap anywhere for weather");
    }
}

static void make_ui(void)
{
    const lv_color_t background = lv_color_hex(0x07111f);
    const lv_color_t primary = lv_color_hex(0xf3f7ff);
    const lv_color_t secondary = lv_color_hex(0x9db4cf);
    const lv_color_t accent = lv_color_hex(0x58d7c4);
    app.screen = lv_scr_act();
    lv_obj_set_style_bg_color(app.screen, background, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(app.screen, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_add_flag(app.screen, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(app.screen, screen_event, LV_EVENT_CLICKED, NULL);

    app.location_label = make_label(app.screen, &lv_font_montserrat_16, secondary);
    lv_label_set_text(app.location_label, app.location_name);
    lv_obj_align(app.location_label, LV_ALIGN_TOP_MID, 0, 30);
    lv_obj_set_style_text_align(app.location_label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    app.connection_label = make_label(app.screen, &lv_font_montserrat_16, secondary);
    lv_label_set_text(app.connection_label, "Starting...");
    lv_obj_align(app.connection_label, LV_ALIGN_TOP_MID, 0, 57);
    lv_obj_set_style_text_align(app.connection_label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);

    app.clock_panel = lv_obj_create(app.screen);
    lv_obj_remove_style_all(app.clock_panel);
    lv_obj_set_size(app.clock_panel, DISPLAY_WIDTH - 40, 275);
    lv_obj_align(app.clock_panel, LV_ALIGN_CENTER, 0, -5);
    app.time_label = make_label(app.clock_panel, &lv_font_montserrat_48, primary);
    lv_obj_set_width(app.time_label, DISPLAY_WIDTH - 40);
    lv_obj_align(app.time_label, LV_ALIGN_CENTER, 0, -35);
    lv_obj_set_style_text_align(app.time_label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    app.date_label = make_label(app.clock_panel, &lv_font_montserrat_24, accent);
    lv_obj_set_width(app.date_label, DISPLAY_WIDTH - 40);
    lv_obj_align(app.date_label, LV_ALIGN_CENTER, 0, 40);
    lv_obj_set_style_text_align(app.date_label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);

    app.weather_panel = lv_obj_create(app.screen);
    lv_obj_remove_style_all(app.weather_panel);
    lv_obj_set_size(app.weather_panel, DISPLAY_WIDTH - 40, 285);
    lv_obj_align(app.weather_panel, LV_ALIGN_CENTER, 0, -2);
    lv_obj_add_flag(app.weather_panel, LV_OBJ_FLAG_HIDDEN);
    app.temperature_label = make_label(app.weather_panel, &lv_font_montserrat_48, primary);
    lv_obj_set_width(app.temperature_label, DISPLAY_WIDTH - 40);
    lv_obj_align(app.temperature_label, LV_ALIGN_TOP_MID, 0, 4);
    lv_obj_set_style_text_align(app.temperature_label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    app.condition_label = make_label(app.weather_panel, &lv_font_montserrat_24, accent);
    lv_obj_set_width(app.condition_label, DISPLAY_WIDTH - 40);
    lv_obj_align(app.condition_label, LV_ALIGN_TOP_MID, 0, 70);
    lv_obj_set_style_text_align(app.condition_label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    app.details_label = make_label(app.weather_panel, &lv_font_montserrat_16, secondary);
    lv_obj_set_width(app.details_label, DISPLAY_WIDTH - 40);
    lv_obj_align(app.details_label, LV_ALIGN_TOP_MID, 0, 111);
    lv_obj_set_style_text_align(app.details_label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    app.forecast_label = make_label(app.weather_panel, &lv_font_montserrat_16, primary);
    lv_obj_set_width(app.forecast_label, DISPLAY_WIDTH - 40);
    lv_obj_align(app.forecast_label, LV_ALIGN_TOP_MID, 0, 155);
    lv_obj_set_style_text_align(app.forecast_label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);

    app.wifi_button = make_button(app.screen, "Wi-Fi", 116);
    lv_obj_align(app.wifi_button, LV_ALIGN_BOTTOM_LEFT, 32, -48);
    lv_obj_add_event_cb(app.wifi_button, wifi_button_event, LV_EVENT_CLICKED, NULL);

    lv_obj_t *refresh_button = make_button(app.screen, "Refresh", 116);
    lv_obj_align(refresh_button, LV_ALIGN_BOTTOM_RIGHT, -32, -48);
    lv_obj_add_event_cb(refresh_button, refresh_button_event, LV_EVENT_CLICKED, NULL);

    app.hint_label = make_label(app.screen, &lv_font_montserrat_16, secondary);
    lv_label_set_text(app.hint_label, "Tap anywhere for weather");
    lv_obj_align(app.hint_label, LV_ALIGN_BOTTOM_MID, 0, -16);
    lv_obj_set_style_text_align(app.hint_label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
}

static void update_ui(lv_timer_t *timer)
{
    (void)timer;
    time_t now = time(NULL);
    struct tm local_time = {0};
    localtime_r(&now, &local_time);
    if (local_time.tm_year >= 124) {
        char time_text[16], date_text[32];
        strftime(time_text, sizeof(time_text), "%H:%M", &local_time);
        strftime(date_text, sizeof(date_text), "%A, %B %d", &local_time);
        lv_label_set_text(app.time_label, time_text);
        lv_label_set_text(app.date_label, date_text);
    } else {
        lv_label_set_text(app.time_label, "--:--");
        lv_label_set_text(app.date_label, "Waiting for network time");
    }

    if ((xEventGroupGetBits(app.events) & WIFI_CONNECTED_BIT) != 0) {
        lv_label_set_text(app.connection_label, "Wi-Fi connected");
    } else if (!app.wifi_configured) {
        lv_label_set_text(app.connection_label, app.wifi_scanning ? "Scanning for Wi-Fi..." : "Tap Wi-Fi to connect");
    } else {
        lv_label_set_text(app.connection_label, "Connecting to Wi-Fi...");
    }

    weather_snapshot_t snapshot = {0};
    if (xSemaphoreTake(app.weather_mutex, 0) == pdTRUE) {
        snapshot = app.weather;
        xSemaphoreGive(app.weather_mutex);
    }
    if (snapshot.valid) {
        lv_label_set_text_fmt(app.temperature_label, "%.1f°C", snapshot.temperature);
        lv_label_set_text(app.condition_label, weather_description(snapshot.weather_code));
        lv_label_set_text_fmt(app.details_label, "Humidity %d%%   Wind %.0f km/h", snapshot.humidity, snapshot.wind_speed);
        char forecast[256];
        snprintf(forecast, sizeof(forecast),
                 "Today  %.0f / %.0f°C  %s\nTomorrow  %.0f / %.0f°C  %s\nNext day  %.0f / %.0f°C  %s",
                 snapshot.days[0].minimum, snapshot.days[0].maximum, weather_short(snapshot.days[0].weather_code),
                 snapshot.days[1].minimum, snapshot.days[1].maximum, weather_short(snapshot.days[1].weather_code),
                 snapshot.days[2].minimum, snapshot.days[2].maximum, weather_short(snapshot.days[2].weather_code));
        lv_label_set_text(app.forecast_label, forecast);
        lv_label_set_text(app.connection_label, snapshot.status);
    } else {
        lv_label_set_text(app.temperature_label, "--°C");
        lv_label_set_text(app.condition_label, "No weather yet");
        lv_label_set_text(app.details_label, snapshot.status[0] ? snapshot.status : "Waiting for first update");
        lv_label_set_text(app.forecast_label, "Configure Wi-Fi to load a forecast");
    }
}

void app_main(void)
{
    memset(&app, 0, sizeof(app));
    app.events = xEventGroupCreate();
    app.weather_mutex = xSemaphoreCreateMutex();
    snprintf(app.weather.status, sizeof(app.weather.status), "Starting...");
    if (app.events == NULL || app.weather_mutex == NULL) {
        ESP_LOGE(TAG, "Failed to allocate application synchronization");
        return;
    }

    bsp_display_cfg_t display_config = {
        .lv_adapter_cfg = ESP_LV_ADAPTER_DEFAULT_CONFIG(),
        .rotation = ESP_LV_ADAPTER_ROTATE_0,
        .tear_avoid_mode = ESP_LV_ADAPTER_TEAR_AVOID_MODE_NONE,
        .touch_flags = {.swap_xy = 0, .mirror_x = 1, .mirror_y = 1},
    };
    if (bsp_display_start_with_config(&display_config) == NULL) {
        ESP_LOGE(TAG, "Could not start the Waveshare display");
        return;
    }
    bsp_display_brightness_set(CONFIG_IRIS_BRIGHTNESS);

    esp_err_t nvs_result = nvs_flash_init();
    if (nvs_result == ESP_ERR_NVS_NO_FREE_PAGES || nvs_result == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ESP_ERROR_CHECK(nvs_flash_init());
    } else {
        ESP_ERROR_CHECK(nvs_result);
    }
    load_runtime_config();

    ESP_ERROR_CHECK(bsp_display_lock(UINT32_MAX));
    make_ui();
    lv_timer_create(update_ui, 1000, NULL);
    bsp_display_unlock();

    start_wifi();
    start_time_sync();
    xTaskCreate(weather_task, "weather", 12288, NULL, 4, NULL);
    ESP_LOGI(TAG, "Iris ready");
}
