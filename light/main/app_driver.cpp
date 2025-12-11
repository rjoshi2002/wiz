/*
   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include <esp_log.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <esp_matter.h>
#include <app_priv.h>
#include <common_macros.h>

#include <device.h>
#include <led_driver.h>
#include <button_gpio.h>
#include "wiz_udp_driver.h"

using namespace chip::app::Clusters;
using namespace esp_matter;

static const char *TAG = "app_driver";
extern uint16_t light_endpoint_id;

// Global variables to store current state
static uint16_t current_x = 0;
static uint16_t current_y = 0;
static uint8_t current_brightness = 100;
static uint8_t current_hue = 0;
static uint8_t current_saturation = 0;

/* Helper function to convert XY to RGB */
static void xy_to_rgb(uint16_t x, uint16_t y, uint8_t *r, uint8_t *g, uint8_t *b)
{
    // Convert from Matter's XY color space (0-65535) to normalized 0-1
    float fx = (float)x / 65535.0f;
    float fy = (float)y / 65535.0f;

    // Prevent division by zero
    if (fy < 0.001f) fy = 0.001f;

    // Calculate z
    float fz = 1.0f - fx - fy;

    // Convert XYZ to RGB
    float Y = 1.0f; // Assume full brightness for conversion
    float X = (Y / fy) * fx;
    float Z = (Y / fy) * fz;

    // XYZ to RGB conversion matrix (sRGB D65)
    float fr = X * 3.2406f - Y * 1.5372f - Z * 0.4986f;
    float fg = -X * 0.9689f + Y * 1.8758f + Z * 0.0415f;
    float fb = X * 0.0557f - Y * 0.2040f + Z * 1.0570f;

    // Apply gamma correction and clamp
    auto gamma_correct = [](float c) -> uint8_t {
        if (c <= 0.0031308f) {
            c = 12.92f * c;
        } else {
            c = 1.055f * powf(c, 1.0f/2.4f) - 0.055f;
        }
        if (c < 0.0f) c = 0.0f;
        if (c > 1.0f) c = 1.0f;
        return (uint8_t)(c * 255.0f);
    };

    *r = gamma_correct(fr);
    *g = gamma_correct(fg);
    *b = gamma_correct(fb);
}

/* Do any conversions/remapping for the actual value here */
static esp_err_t app_driver_light_set_power(wiz_driver_handle_t handle, esp_matter_attr_val_t *val)
{
    return wiz_driver_set_power(handle, val->val.b);
}

static esp_err_t app_driver_light_set_brightness(wiz_driver_handle_t handle, esp_matter_attr_val_t *val)
{
    current_brightness = REMAP_TO_RANGE(val->val.u8, MATTER_BRIGHTNESS, STANDARD_BRIGHTNESS);
    return wiz_driver_set_brightness(handle, current_brightness);
}

static esp_err_t app_driver_light_set_hue(wiz_driver_handle_t handle, esp_matter_attr_val_t *val)
{
    current_hue = REMAP_TO_RANGE(val->val.u8, MATTER_HUE, STANDARD_HUE);
    return wiz_driver_set_hsv(handle, current_hue, current_saturation, current_brightness);
}

static esp_err_t app_driver_light_set_saturation(wiz_driver_handle_t handle, esp_matter_attr_val_t *val)
{
    current_saturation = REMAP_TO_RANGE(val->val.u8, MATTER_SATURATION, STANDARD_SATURATION);
    return wiz_driver_set_hsv(handle, current_hue, current_saturation, current_brightness);
}

static esp_err_t app_driver_light_set_temperature(wiz_driver_handle_t handle, esp_matter_attr_val_t *val)
{
    // Convert from mireds to Kelvin
    // Kelvin = 1,000,000 / mireds
    uint32_t mireds = val->val.u16;
    uint16_t kelvin = (mireds > 0) ? (1000000 / mireds) : 4000;

    // Clamp to Wiz light range
    if (kelvin < 2200) kelvin = 2200;
    if (kelvin > 6500) kelvin = 6500;

    return wiz_driver_set_temperature(handle, kelvin, current_brightness);
}

static esp_err_t app_driver_light_set_xy(wiz_driver_handle_t handle, uint16_t x, uint16_t y)
{
    uint8_t r, g, b;
    xy_to_rgb(x, y, &r, &g, &b);
    return wiz_driver_set_rgb(handle, r, g, b, current_brightness);
}

static void app_driver_button_toggle_cb(void *arg, void *data)
{
    ESP_LOGI(TAG, "Toggle button pressed");
    uint16_t endpoint_id = light_endpoint_id;
    uint32_t cluster_id = OnOff::Id;
    uint32_t attribute_id = OnOff::Attributes::OnOff::Id;

    attribute_t *attribute = attribute::get(endpoint_id, cluster_id, attribute_id);

    esp_matter_attr_val_t val = esp_matter_invalid(NULL);
    attribute::get_val(attribute, &val);
    val.val.b = !val.val.b;
    attribute::update(endpoint_id, cluster_id, attribute_id, &val);
}

esp_err_t app_driver_attribute_update(app_driver_handle_t driver_handle, uint16_t endpoint_id, uint32_t cluster_id,
                                      uint32_t attribute_id, esp_matter_attr_val_t *val)
{
    esp_err_t err = ESP_OK;
    if (endpoint_id == light_endpoint_id) {
        wiz_driver_handle_t handle = (wiz_driver_handle_t)driver_handle;
        if (cluster_id == OnOff::Id) {
            if (attribute_id == OnOff::Attributes::OnOff::Id) {
                err = app_driver_light_set_power(handle, val);
            }
        } else if (cluster_id == LevelControl::Id) {
            if (attribute_id == LevelControl::Attributes::CurrentLevel::Id) {
                err = app_driver_light_set_brightness(handle, val);
            }
        } else if (cluster_id == ColorControl::Id) {
            if (attribute_id == ColorControl::Attributes::CurrentHue::Id) {
                current_hue = REMAP_TO_RANGE(val->val.u8, MATTER_HUE, STANDARD_HUE);
                err = app_driver_light_set_hue(handle, val);
            } else if (attribute_id == ColorControl::Attributes::CurrentSaturation::Id) {
                current_saturation = REMAP_TO_RANGE(val->val.u8, MATTER_SATURATION, STANDARD_SATURATION);
                err = app_driver_light_set_saturation(handle, val);
            } else if (attribute_id == ColorControl::Attributes::ColorTemperatureMireds::Id) {
                err = app_driver_light_set_temperature(handle, val);
            } else if (attribute_id == ColorControl::Attributes::CurrentX::Id) {
                current_x = val->val.u16;
                err = app_driver_light_set_xy(handle, current_x, current_y);
            } else if (attribute_id == ColorControl::Attributes::CurrentY::Id) {
                current_y = val->val.u16;
                err = app_driver_light_set_xy(handle, current_x, current_y);
            }
        }
    }
    return err;
}

esp_err_t app_driver_light_set_defaults(uint16_t endpoint_id)
{
    esp_err_t err = ESP_OK;
    void *priv_data = endpoint::get_priv_data(endpoint_id);
    wiz_driver_handle_t handle = (wiz_driver_handle_t)priv_data;
    esp_matter_attr_val_t val = esp_matter_invalid(NULL);

    /* Setting brightness */
    attribute_t *attribute = attribute::get(endpoint_id, LevelControl::Id, LevelControl::Attributes::CurrentLevel::Id);
    attribute::get_val(attribute, &val);
    current_brightness = REMAP_TO_RANGE(val.val.u8, MATTER_BRIGHTNESS, STANDARD_BRIGHTNESS);
    err |= app_driver_light_set_brightness(handle, &val);

    /* Setting color */
    attribute = attribute::get(endpoint_id, ColorControl::Id, ColorControl::Attributes::ColorMode::Id);
    attribute::get_val(attribute, &val);
    if (val.val.u8 == (uint8_t)ColorControl::ColorMode::kCurrentHueAndCurrentSaturation) {
        /* Setting hue */
        attribute = attribute::get(endpoint_id, ColorControl::Id, ColorControl::Attributes::CurrentHue::Id);
        attribute::get_val(attribute, &val);
        current_hue = REMAP_TO_RANGE(val.val.u8, MATTER_HUE, STANDARD_HUE);
        err |= app_driver_light_set_hue(handle, &val);
        /* Setting saturation */
        attribute = attribute::get(endpoint_id, ColorControl::Id, ColorControl::Attributes::CurrentSaturation::Id);
        attribute::get_val(attribute, &val);
        current_saturation = REMAP_TO_RANGE(val.val.u8, MATTER_SATURATION, STANDARD_SATURATION);
        err |= app_driver_light_set_saturation(handle, &val);
    } else if (val.val.u8 == (uint8_t)ColorControl::ColorMode::kColorTemperature) {
        /* Setting temperature */
        attribute = attribute::get(endpoint_id, ColorControl::Id, ColorControl::Attributes::ColorTemperatureMireds::Id);
        attribute::get_val(attribute, &val);
        err |= app_driver_light_set_temperature(handle, &val);
    } else if (val.val.u8 == (uint8_t)ColorControl::ColorMode::kCurrentXAndCurrentY) {
        /* Setting XY coordinates */
        attribute = attribute::get(endpoint_id, ColorControl::Id, ColorControl::Attributes::CurrentX::Id);
        attribute::get_val(attribute, &val);
        current_x = val.val.u16;
        attribute = attribute::get(endpoint_id, ColorControl::Id, ColorControl::Attributes::CurrentY::Id);
        attribute::get_val(attribute, &val);
        current_y = val.val.u16;
        err |= app_driver_light_set_xy(handle, current_x, current_y);
    } else {
        ESP_LOGE(TAG, "Color mode not supported");
    }

    /* Setting power */
    attribute = attribute::get(endpoint_id, OnOff::Id, OnOff::Attributes::OnOff::Id);
    attribute::get_val(attribute, &val);
    err |= app_driver_light_set_power(handle, &val);

    return err;
}

app_driver_handle_t app_driver_light_init()
{
    /* Initialize Wiz UDP driver */
    wiz_driver_config_t config = wiz_driver_get_config();
    wiz_driver_handle_t handle = wiz_driver_init(&config);

    if (handle) {
        ESP_LOGI(TAG, "Wiz driver initialized successfully");
    } else {
        ESP_LOGE(TAG, "Failed to initialize Wiz driver");
    }

    return (app_driver_handle_t)handle;
}

app_driver_handle_t app_driver_button_init()
{
    /* Initialize button */
    button_handle_t handle = NULL;
    const button_config_t btn_cfg = {0};
    const button_gpio_config_t btn_gpio_cfg = button_driver_get_config();

    if (iot_button_new_gpio_device(&btn_cfg, &btn_gpio_cfg, &handle) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create button device");
        return NULL;
    }

    iot_button_register_cb(handle, BUTTON_PRESS_DOWN, NULL, app_driver_button_toggle_cb, NULL);
    return (app_driver_handle_t)handle;
}
