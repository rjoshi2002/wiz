/*
   Wiz Light UDP Driver

   This driver controls Wiz smart lights via UDP protocol
*/

#pragma once

#include <esp_err.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Wiz light IP addresses - configure these for your lights
#define WIZ_UDP_PORT 38899
#define WIZ_MAX_LIGHTS 10

// Wiz driver handle
typedef void* wiz_driver_handle_t;

// Configuration structure
typedef struct {
    const char** light_ips;  // Array of light IP addresses
    uint8_t num_lights;      // Number of lights in the array
    uint16_t udp_port;       // UDP port (default 38899)
} wiz_driver_config_t;

/**
 * @brief Initialize the Wiz UDP driver
 *
 * @param config Configuration structure with light IPs
 * @return wiz_driver_handle_t Handle on success, NULL on failure
 */
wiz_driver_handle_t wiz_driver_init(wiz_driver_config_t *config);

/**
 * @brief Set power state for all lights
 *
 * @param handle Wiz driver handle
 * @param power true for on, false for off
 * @return esp_err_t ESP_OK on success
 */
esp_err_t wiz_driver_set_power(wiz_driver_handle_t handle, bool power);

/**
 * @brief Set brightness for all lights
 *
 * @param handle Wiz driver handle
 * @param brightness Brightness value 0-100
 * @return esp_err_t ESP_OK on success
 */
esp_err_t wiz_driver_set_brightness(wiz_driver_handle_t handle, uint8_t brightness);

/**
 * @brief Set RGB color for all lights
 *
 * @param handle Wiz driver handle
 * @param r Red value 0-255
 * @param g Green value 0-255
 * @param b Blue value 0-255
 * @param brightness Brightness value 0-100
 * @return esp_err_t ESP_OK on success
 */
esp_err_t wiz_driver_set_rgb(wiz_driver_handle_t handle, uint8_t r, uint8_t g, uint8_t b, uint8_t brightness);

/**
 * @brief Set color temperature for all lights
 *
 * @param handle Wiz driver handle
 * @param kelvin Temperature in Kelvin (2200-6500)
 * @param brightness Brightness value 0-100
 * @return esp_err_t ESP_OK on success
 */
esp_err_t wiz_driver_set_temperature(wiz_driver_handle_t handle, uint16_t kelvin, uint8_t brightness);

/**
 * @brief Set HSV color for all lights
 *
 * @param handle Wiz driver handle
 * @param hue Hue value 0-360
 * @param saturation Saturation value 0-100
 * @param brightness Brightness value 0-100
 * @return esp_err_t ESP_OK on success
 */
esp_err_t wiz_driver_set_hsv(wiz_driver_handle_t handle, uint16_t hue, uint8_t saturation, uint8_t brightness);

/**
 * @brief Get the default configuration
 *
 * @return wiz_driver_config_t Default configuration
 */
wiz_driver_config_t wiz_driver_get_config(void);

#ifdef __cplusplus
}
#endif
