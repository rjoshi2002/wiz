# Wiz Light Integration for ESP Matter

This ESP Matter light example has been modified to control Wiz smart lights via UDP instead of controlling a physical LED.

## What Was Modified

### New Files Created:
1. **wiz_udp_driver.h** - Header file defining the Wiz UDP driver interface
2. **wiz_udp_driver.cpp** - Implementation of UDP communication with Wiz lights

### Modified Files:
1. **app_driver.cpp** - Replaced LED driver calls with Wiz UDP driver calls
   - Power control → Wiz on/off commands
   - Brightness → Wiz dimming (0-100%)
   - Hue/Saturation → Converted to RGB and sent to Wiz lights
   - Color Temperature → Converted from mireds to Kelvin (2200-6500K)
   - XY Color → Converted to RGB for Wiz lights

## How It Works

When you control this Matter device (e.g., from Apple Home, Google Home, or any Matter controller):
- The Matter light appears as **one single light** to the controller
- All commands are sent via UDP to **all configured Wiz lights simultaneously**
- All your Wiz lights act in unison as one grouped light

## Configuration

### Setting Your Wiz Light IP Addresses

Edit [wiz_udp_driver.cpp](wiz_udp_driver.cpp) line 12-19 to configure your Wiz light IP addresses:

```cpp
static const char* default_light_ips[] = {
    "192.168.0.155",
    "192.168.0.139",
    "192.168.0.196",
    "192.168.0.179",
    "192.168.0.116",
    "192.168.0.83"
};
```

**Important:**
- Replace these IPs with your actual Wiz light IP addresses
- You can add or remove lights from the array
- Make sure your lights have static IP addresses or DHCP reservations

## Building and Flashing

```bash
# Navigate to the light directory
cd light

# Set the target (e.g., ESP32-C3, ESP32-S3, etc.)
idf.py set-target esp32c3

# Configure (optional - for WiFi credentials)
idf.py menuconfig

# Build
idf.py build

# Flash and monitor
idf.py flash monitor
```

## Commissioning

1. Build and flash the firmware to your ESP device
2. The device will appear as a Matter-compatible light
3. Commission it using your preferred Matter controller:
   - **Apple Home:** Use "Add Accessory" and scan the QR code
   - **Google Home:** Use "Add Device" → "Matter-enabled device"
   - **Amazon Alexa:** Use "Add Device" → "Matter"

4. Once commissioned, controlling the Matter device will control all your Wiz lights together

## Features Supported

- ✅ On/Off control
- ✅ Brightness (0-100%)
- ✅ RGB color control
- ✅ Hue/Saturation control
- ✅ Color temperature (2200K-6500K)
- ✅ XY color space
- ✅ Multiple lights controlled as one

## Technical Details

### UDP Communication
- **Protocol:** Wiz UDP protocol (JSON-based)
- **Port:** 38899
- **Timeout:** 2 seconds per light
- **Delay:** 50ms between commands to multiple lights
- **Socket Initialization:** Lazy (created on first command to avoid crash during early boot)

### Color Conversions
- **HSV → RGB:** Converts Matter's hue/saturation to RGB for Wiz
- **XY → RGB:** Converts CIE 1931 XY color space to RGB
- **Mireds → Kelvin:** Temperature = 1,000,000 / mireds

### Example Commands Sent to Wiz Lights

**Power On:**
```json
{"method":"setPilot","params":{"state":true}}
```

**Set RGB Color:**
```json
{"method":"setPilot","params":{"state":true,"r":255,"g":0,"b":0,"dimming":80}}
```

**Set Temperature:**
```json
{"method":"setPilot","params":{"state":true,"temp":2700,"dimming":100}}
```

## Troubleshooting

### Lights not responding
1. Check that light IPs are correct in `wiz_udp_driver.cpp`
2. Ensure ESP device and lights are on the same network
3. Check the serial monitor for UDP errors: `idf.py monitor`
4. Verify lights are accessible: `ping <light-ip>`

### Only some lights respond
- Check network connectivity for each light
- Ensure all lights support the Wiz UDP protocol (port 38899)
- Check the logs for specific light failures

### Color/brightness issues
- Some Wiz models may have different capabilities
- Try using color temperature mode if RGB doesn't work well
- Check that brightness values are properly scaled (0-100%)

## Next Steps

- Add individual light control (create separate Matter endpoints)
- Add scenes/effects support
- Implement light grouping configuration via Matter
- Add status feedback from lights
