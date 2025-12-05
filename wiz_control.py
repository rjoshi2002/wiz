#!/usr/bin/env python3
"""
Simple Wiz Smart Light Controller
Controls Wiz lights over UDP on port 38899
"""

import socket
import json
import time

# Configuration - Add your light IP addresses here
LIGHT_IPS = [
    "172.18.53.231",  # Replace with your actual light IPs
    "172.18.53.230",
    "172.18.53.192",
    "172.18.53.178",
    "172.18.53.177",
    "172.18.53.179",
    # Add more IPs as needed
]

UDP_PORT = 38899
TIMEOUT = 2  # seconds


def send_command(ip, command):
    """Send a command to a Wiz light via UDP"""
    try:
        # Create UDP socket
        sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        sock.settimeout(TIMEOUT)

        # Send command
        message = json.dumps(command).encode('utf-8')
        sock.sendto(message, (ip, UDP_PORT))

        # Wait for response
        response, _ = sock.recvfrom(1024)
        result = json.loads(response.decode('utf-8'))

        sock.close()
        return result
    except socket.timeout:
        print(f"  ⚠️  Timeout waiting for response from {ip}")
        return None
    except Exception as e:
        print(f"  ❌ Error communicating with {ip}: {e}")
        return None


def turn_on(ip):
    """Turn on a Wiz light"""
    command = {"method": "setPilot", "params": {"state": True}}
    return send_command(ip, command)


def turn_off(ip):
    """Turn off a Wiz light"""
    command = {"method": "setPilot", "params": {"state": False}}
    return send_command(ip, command)


def set_color(ip, r, g, b, brightness=100):
    """Set color of a Wiz light using RGB values"""
    command = {
        "method": "setPilot",
        "params": {
            "state": True,
            "r": r,
            "g": g,
            "b": b,
            "dimming": brightness
        }
    }
    return send_command(ip, command)


def hex_to_rgb(hex_color):
    """Convert hex color code to RGB tuple"""
    # Remove '#' if present
    hex_color = hex_color.lstrip('#')

    # Convert to RGB
    if len(hex_color) == 6:
        return tuple(int(hex_color[i:i+2], 16) for i in (0, 2, 4))
    else:
        raise ValueError("Invalid hex color format. Use format: #RRGGBB or RRGGBB")


def get_status(ip):
    """Get the current status of a Wiz light"""
    command = {"method": "getPilot", "params": {}}
    return send_command(ip, command)


def set_temperature(ip, kelvin, brightness=100):
    """Set color temperature of a Wiz light in Kelvin"""
    command = {
        "method": "setPilot",
        "params": {
            "state": True,
            "temp": kelvin,
            "dimming": brightness
        }
    }
    return send_command(ip, command)


def set_all_color(r, g, b, brightness=100):
    """Set color for all configured lights"""
    print(f"Setting color to RGB({r}, {g}, {b}) at {brightness}% brightness...")
    for ip in LIGHT_IPS:
        print(f"  {ip}...", end=" ")
        result = set_color(ip, r, g, b, brightness)
        if result and result.get("result", {}).get("success"):
            print("✓ Done")
        else:
            print("✗ Failed")
        time.sleep(0.1)


def set_all_temperature(kelvin, brightness=100):
    """Set color temperature for all configured lights"""
    print(f"Setting temperature to {kelvin}K at {brightness}% brightness...")
    for ip in LIGHT_IPS:
        print(f"  {ip}...", end=" ")
        result = set_temperature(ip, kelvin, brightness)
        if result and result.get("result", {}).get("success"):
            print("✓ Done")
        else:
            print("✗ Failed")
        time.sleep(0.1)


def turn_all_on():
    """Turn on all configured lights"""
    print("Turning ON all lights...")
    for ip in LIGHT_IPS:
        print(f"  {ip}...", end=" ")
        result = turn_on(ip)
        if result and result.get("result", {}).get("success"):
            print("✓ ON")
        else:
            print("✗ Failed")
        time.sleep(0.1)  # Small delay between commands


def turn_all_off():
    """Turn off all configured lights"""
    print("Turning OFF all lights...")
    for ip in LIGHT_IPS:
        print(f"  {ip}...", end=" ")
        result = turn_off(ip)
        if result and result.get("result", {}).get("success"):
            print("✓ OFF")
        else:
            print("✗ Failed")
        time.sleep(0.1)


def check_all_status():
    """Check status of all configured lights"""
    print("Checking status of all lights...")
    for ip in LIGHT_IPS:
        print(f"  {ip}...", end=" ")
        result = get_status(ip)
        if result and "result" in result:
            state = result["result"].get("state")
            if state is not None:
                status = "ON" if state else "OFF"
                print(f"✓ {status}")
            else:
                print("✗ Unknown state")
        else:
            print("✗ No response")


def show_menu():
    """Display the main menu"""
    print("\n" + "=" * 50)
    print("Wiz Light Controller - Interactive Mode")
    print("=" * 50)
    print(f"Controlling {len(LIGHT_IPS)} light(s)")
    print("\nOptions:")
    print("  1 - Turn all lights ON")
    print("  2 - Turn all lights OFF")
    print("  3 - Set color (hex code)")
    print("  4 - Set color (preset)")
    print("  5 - Set color temperature (Kelvin)")
    print("  6 - Set brightness")
    print("  7 - Check status")
    print("  0 - Exit")
    print("-" * 50)


def get_brightness_input():
    """Get brightness level from user"""
    while True:
        try:
            brightness = input("Enter brightness (1-100): ").strip()
            brightness = int(brightness)
            if 1 <= brightness <= 100:
                return brightness
            else:
                print("Please enter a value between 1 and 100")
        except ValueError:
            print("Please enter a valid number")


def handle_hex_color():
    """Handle hex color input"""
    hex_input = input("Enter hex color code (e.g., #FF5733 or FF5733): ").strip()
    try:
        r, g, b = hex_to_rgb(hex_input)
        brightness = get_brightness_input()
        set_all_color(r, g, b, brightness)
    except ValueError as e:
        print(f"Error: {e}")


def handle_preset_color():
    """Handle preset color selection"""
    presets = {
        "1": ("Red", 255, 0, 0),
        "2": ("Green", 0, 255, 0),
        "3": ("Blue", 0, 0, 255),
        "4": ("Yellow", 255, 255, 0),
        "5": ("Purple", 128, 0, 128),
        "6": ("Cyan", 0, 255, 255),
        "7": ("Orange", 255, 165, 0),
        "8": ("Pink", 255, 192, 203),
        "9": ("Warm White", 255, 230, 200),
    }

    print("\nPreset Colors:")
    for key, (name, _, _, _) in presets.items():
        print(f"  {key} - {name}")

    choice = input("Select preset (1-9): ").strip()
    if choice in presets:
        name, r, g, b = presets[choice]
        brightness = get_brightness_input()
        print(f"Setting to {name}...")
        set_all_color(r, g, b, brightness)
    else:
        print("Invalid preset selection")


def handle_brightness():
    """Handle brightness adjustment (keeps current color)"""
    brightness = get_brightness_input()
    print(f"Setting brightness to {brightness}%...")
    # For brightness only, we use a simple dimming command
    for ip in LIGHT_IPS:
        print(f"  {ip}...", end=" ")
        command = {"method": "setPilot", "params": {"dimming": brightness}}
        result = send_command(ip, command)
        if result and result.get("result", {}).get("success"):
            print("✓ Done")
        else:
            print("✗ Failed")
        time.sleep(0.1)


def handle_temperature():
    """Handle color temperature selection"""
    presets = {
        "1": ("Candle Light", 2200),
        "2": ("Warm White", 2700),
        "3": ("Soft White", 3000),
        "4": ("Neutral White", 4000),
        "5": ("Cool White", 5000),
        "6": ("Daylight", 6500),
        "c": ("Custom", None),
    }

    print("\nColor Temperature Presets:")
    for key, (name, kelvin) in presets.items():
        if kelvin:
            print(f"  {key} - {name} ({kelvin}K)")
        else:
            print(f"  {key} - {name}")

    choice = input("Select preset (1-6, c for custom): ").strip().lower()

    if choice in presets:
        name, kelvin = presets[choice]

        if kelvin is None:
            # Custom temperature
            while True:
                try:
                    kelvin_input = input("Enter temperature in Kelvin (2200-6500): ").strip()
                    kelvin = int(kelvin_input)
                    if 2200 <= kelvin <= 6500:
                        break
                    else:
                        print("Please enter a value between 2200 and 6500")
                except ValueError:
                    print("Please enter a valid number")

        brightness = get_brightness_input()
        if name != "Custom":
            print(f"Setting to {name} ({kelvin}K)...")
        else:
            print(f"Setting to {kelvin}K...")
        set_all_temperature(kelvin, brightness)
    else:
        print("Invalid selection")


def main_loop():
    """Main interactive loop"""
    print("\nWelcome to Wiz Light Controller!")
    print(f"Loaded {len(LIGHT_IPS)} light IP(s)")

    while True:
        show_menu()
        choice = input("\nEnter your choice: ").strip()

        if choice == "1":
            turn_all_on()
        elif choice == "2":
            turn_all_off()
        elif choice == "3":
            handle_hex_color()
        elif choice == "4":
            handle_preset_color()
        elif choice == "5":
            handle_temperature()
        elif choice == "6":
            handle_brightness()
        elif choice == "7":
            check_all_status()
        elif choice == "0":
            print("\nExiting... Goodbye!")
            break
        else:
            print("Invalid choice. Please try again.")


if __name__ == "__main__":
    try:
        main_loop()
    except KeyboardInterrupt:
        print("\n\nInterrupted. Exiting... Goodbye!")
    except Exception as e:
        print(f"\nUnexpected error: {e}")
        import traceback
        traceback.print_exc()
