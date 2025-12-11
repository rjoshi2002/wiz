/*
   Wiz Light UDP Driver Implementation
*/

#include <string.h>
#include <esp_log.h>
#include <esp_wifi.h>
#include <lwip/sockets.h>
#include <lwip/netdb.h>
#include "wiz_udp_driver.h"

static const char *TAG = "wiz_udp_driver";

// Default light IP addresses - CONFIGURE THESE FOR YOUR LIGHTS
static const char* default_light_ips[] = {
    "192.168.0.155",
    "192.168.0.139",
    "192.168.0.196",
    "192.168.0.179",
    "192.168.0.116",
    "192.168.0.83"
};

typedef struct {
    const char** light_ips;
    uint8_t num_lights;
    uint16_t udp_port;
    int sock;
    bool sock_initialized;       // Track if socket has been created
    uint8_t current_brightness;  // Track current brightness for RGB commands
} wiz_driver_data_t;

// Helper function to convert HSV to RGB
static void hsv_to_rgb(uint16_t hue, uint8_t sat, uint8_t val, uint8_t *r, uint8_t *g, uint8_t *b)
{
    if (sat == 0) {
        *r = *g = *b = val;
        return;
    }

    uint16_t region = hue / 60;
    uint16_t remainder = (hue - (region * 60)) * 6;

    uint8_t p = (val * (100 - sat)) / 100;
    uint8_t q = (val * (100 - ((sat * remainder) / 600))) / 100;
    uint8_t t = (val * (100 - ((sat * (600 - remainder)) / 600))) / 100;

    val = (val * 255) / 100;  // Convert to 0-255
    p = (p * 255) / 100;
    q = (q * 255) / 100;
    t = (t * 255) / 100;

    switch (region) {
        case 0: *r = val; *g = t; *b = p; break;
        case 1: *r = q; *g = val; *b = p; break;
        case 2: *r = p; *g = val; *b = t; break;
        case 3: *r = p; *g = q; *b = val; break;
        case 4: *r = t; *g = p; *b = val; break;
        default: *r = val; *g = p; *b = q; break;
    }
}

// Helper function to ensure socket is initialized (lazy initialization)
static esp_err_t ensure_socket_initialized(wiz_driver_data_t *driver)
{
    if (driver->sock_initialized) {
        return ESP_OK;  // Already initialized
    }

    // Create UDP socket
    driver->sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (driver->sock < 0) {
        ESP_LOGE(TAG, "Failed to create socket: errno %d", errno);
        return ESP_FAIL;
    }

    // Set socket timeout
    struct timeval timeout;
    timeout.tv_sec = 2;
    timeout.tv_usec = 0;
    setsockopt(driver->sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

    driver->sock_initialized = true;
    ESP_LOGI(TAG, "UDP socket initialized successfully");

    return ESP_OK;
}

// Helper function to send UDP command to all lights
static esp_err_t send_to_all_lights(wiz_driver_data_t *driver, const char *json_command)
{
    if (!driver || !json_command) {
        return ESP_ERR_INVALID_ARG;
    }

    // Lazy initialize socket on first use
    if (ensure_socket_initialized(driver) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize socket");
        return ESP_FAIL;
    }

    struct sockaddr_in dest_addr;
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(driver->udp_port);

    int success_count = 0;

    for (int i = 0; i < driver->num_lights; i++) {
        dest_addr.sin_addr.s_addr = inet_addr(driver->light_ips[i]);

        int err = sendto(driver->sock, json_command, strlen(json_command), 0,
                        (struct sockaddr *)&dest_addr, sizeof(dest_addr));

        if (err < 0) {
            ESP_LOGW(TAG, "Failed to send to light %s: errno %d", driver->light_ips[i], errno);
        } else {
            success_count++;
            ESP_LOGD(TAG, "Sent to light %s: %s", driver->light_ips[i], json_command);
        }

        vTaskDelay(pdMS_TO_TICKS(50));  // Small delay between lights
    }

    return (success_count > 0) ? ESP_OK : ESP_FAIL;
}

wiz_driver_config_t wiz_driver_get_config(void)
{
    wiz_driver_config_t config = {
        .light_ips = default_light_ips,
        .num_lights = sizeof(default_light_ips) / sizeof(default_light_ips[0]),
        .udp_port = WIZ_UDP_PORT
    };
    return config;
}

wiz_driver_handle_t wiz_driver_init(wiz_driver_config_t *config)
{
    if (!config || !config->light_ips || config->num_lights == 0) {
        ESP_LOGE(TAG, "Invalid configuration");
        return NULL;
    }

    wiz_driver_data_t *driver = (wiz_driver_data_t*)malloc(sizeof(wiz_driver_data_t));
    if (!driver) {
        ESP_LOGE(TAG, "Failed to allocate driver data");
        return NULL;
    }

    driver->light_ips = config->light_ips;
    driver->num_lights = config->num_lights;
    driver->udp_port = config->udp_port;
    driver->current_brightness = 100;
    driver->sock = -1;
    driver->sock_initialized = false;  // Socket will be created lazily on first use

    ESP_LOGI(TAG, "Wiz driver initialized with %d lights on port %d (socket will be created when needed)",
             driver->num_lights, driver->udp_port);

    return (wiz_driver_handle_t)driver;
}

esp_err_t wiz_driver_set_power(wiz_driver_handle_t handle, bool power)
{
    wiz_driver_data_t *driver = (wiz_driver_data_t*)handle;
    if (!driver) {
        return ESP_ERR_INVALID_ARG;
    }

    char json_cmd[128];
    snprintf(json_cmd, sizeof(json_cmd),
             "{\"method\":\"setPilot\",\"params\":{\"state\":%s}}",
             power ? "true" : "false");

    ESP_LOGI(TAG, "Setting power: %s", power ? "ON" : "OFF");
    return send_to_all_lights(driver, json_cmd);
}

esp_err_t wiz_driver_set_brightness(wiz_driver_handle_t handle, uint8_t brightness)
{
    wiz_driver_data_t *driver = (wiz_driver_data_t*)handle;
    if (!driver) {
        return ESP_ERR_INVALID_ARG;
    }

    // Clamp brightness to 0-100
    if (brightness > 100) brightness = 100;

    driver->current_brightness = brightness;

    char json_cmd[128];
    snprintf(json_cmd, sizeof(json_cmd),
             "{\"method\":\"setPilot\",\"params\":{\"dimming\":%d}}",
             brightness);

    ESP_LOGI(TAG, "Setting brightness: %d%%", brightness);
    return send_to_all_lights(driver, json_cmd);
}

esp_err_t wiz_driver_set_rgb(wiz_driver_handle_t handle, uint8_t r, uint8_t g, uint8_t b, uint8_t brightness)
{
    wiz_driver_data_t *driver = (wiz_driver_data_t*)handle;
    if (!driver) {
        return ESP_ERR_INVALID_ARG;
    }

    // Clamp brightness to 0-100
    if (brightness > 100) brightness = 100;

    driver->current_brightness = brightness;

    char json_cmd[256];
    snprintf(json_cmd, sizeof(json_cmd),
             "{\"method\":\"setPilot\",\"params\":{\"state\":true,\"r\":%d,\"g\":%d,\"b\":%d,\"dimming\":%d}}",
             r, g, b, brightness);

    ESP_LOGI(TAG, "Setting RGB: (%d, %d, %d) at %d%%", r, g, b, brightness);
    return send_to_all_lights(driver, json_cmd);
}

esp_err_t wiz_driver_set_temperature(wiz_driver_handle_t handle, uint16_t kelvin, uint8_t brightness)
{
    wiz_driver_data_t *driver = (wiz_driver_data_t*)handle;
    if (!driver) {
        return ESP_ERR_INVALID_ARG;
    }

    // Clamp values
    if (kelvin < 2200) kelvin = 2200;
    if (kelvin > 6500) kelvin = 6500;
    if (brightness > 100) brightness = 100;

    driver->current_brightness = brightness;

    char json_cmd[256];
    snprintf(json_cmd, sizeof(json_cmd),
             "{\"method\":\"setPilot\",\"params\":{\"state\":true,\"temp\":%d,\"dimming\":%d}}",
             kelvin, brightness);

    ESP_LOGI(TAG, "Setting temperature: %dK at %d%%", kelvin, brightness);
    return send_to_all_lights(driver, json_cmd);
}

esp_err_t wiz_driver_set_hsv(wiz_driver_handle_t handle, uint16_t hue, uint8_t saturation, uint8_t brightness)
{
    wiz_driver_data_t *driver = (wiz_driver_data_t*)handle;
    if (!driver) {
        return ESP_ERR_INVALID_ARG;
    }

    // Convert HSV to RGB
    uint8_t r, g, b;
    hsv_to_rgb(hue, saturation, brightness, &r, &g, &b);

    ESP_LOGI(TAG, "Setting HSV: H=%d S=%d V=%d -> RGB(%d,%d,%d)",
             hue, saturation, brightness, r, g, b);

    // Use the RGB function to send the command
    return wiz_driver_set_rgb(handle, r, g, b, brightness);
}
