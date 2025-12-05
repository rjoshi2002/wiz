#!/usr/bin/env python3
"""
Wiz Light Web Controller
Web interface for controlling Wiz smart lights
"""

from flask import Flask, render_template, jsonify, request
import socket
import json
import time

# Import configuration from wiz_control
from wiz_control import (
    LIGHT_IPS, UDP_PORT, TIMEOUT,
    send_command, turn_on, turn_off, set_color,
    set_temperature, get_status
)

app = Flask(__name__)


@app.route('/')
def index():
    """Serve the main control page"""
    return render_template('index.html', num_lights=len(LIGHT_IPS))


@app.route('/api/lights/on', methods=['POST'])
def lights_on():
    """Turn all lights on"""
    results = []
    for ip in LIGHT_IPS:
        result = turn_on(ip)
        success = result and result.get("result", {}).get("success", False)
        results.append({"ip": ip, "success": success})

    return jsonify({
        "status": "success" if all(r["success"] for r in results) else "partial",
        "results": results
    })


@app.route('/api/lights/off', methods=['POST'])
def lights_off():
    """Turn all lights off"""
    results = []
    for ip in LIGHT_IPS:
        result = turn_off(ip)
        success = result and result.get("result", {}).get("success", False)
        results.append({"ip": ip, "success": success})

    return jsonify({
        "status": "success" if all(r["success"] for r in results) else "partial",
        "results": results
    })


@app.route('/api/lights/color', methods=['POST'])
def set_lights_color():
    """Set color for all lights"""
    data = request.json
    r = data.get('r', 255)
    g = data.get('g', 255)
    b = data.get('b', 255)
    brightness = data.get('brightness', 100)

    results = []
    for ip in LIGHT_IPS:
        result = set_color(ip, r, g, b, brightness)
        success = result and result.get("result", {}).get("success", False)
        results.append({"ip": ip, "success": success})
        time.sleep(0.05)

    return jsonify({
        "status": "success" if all(r["success"] for r in results) else "partial",
        "results": results
    })


@app.route('/api/lights/temperature', methods=['POST'])
def set_lights_temperature():
    """Set color temperature for all lights"""
    data = request.json
    kelvin = data.get('kelvin', 4000)
    brightness = data.get('brightness', 100)

    results = []
    for ip in LIGHT_IPS:
        result = set_temperature(ip, kelvin, brightness)
        success = result and result.get("result", {}).get("success", False)
        results.append({"ip": ip, "success": success})
        time.sleep(0.05)

    return jsonify({
        "status": "success" if all(r["success"] for r in results) else "partial",
        "results": results
    })


@app.route('/api/lights/brightness', methods=['POST'])
def set_lights_brightness():
    """Set brightness for all lights"""
    data = request.json
    brightness = data.get('brightness', 100)

    results = []
    for ip in LIGHT_IPS:
        command = {"method": "setPilot", "params": {"dimming": brightness}}
        result = send_command(ip, command)
        success = result and result.get("result", {}).get("success", False)
        results.append({"ip": ip, "success": success})
        time.sleep(0.05)

    return jsonify({
        "status": "success" if all(r["success"] for r in results) else "partial",
        "results": results
    })


@app.route('/api/lights/status', methods=['GET'])
def get_lights_status():
    """Get status of all lights"""
    statuses = []
    for ip in LIGHT_IPS:
        result = get_status(ip)
        if result and "result" in result:
            statuses.append({
                "ip": ip,
                "online": True,
                "state": result["result"].get("state", False),
                "brightness": result["result"].get("dimming", 100)
            })
        else:
            statuses.append({
                "ip": ip,
                "online": False,
                "state": False,
                "brightness": 0
            })

    return jsonify({"lights": statuses})


if __name__ == '__main__':
    import socket

    # Get local IP address
    s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    try:
        s.connect(('8.8.8.8', 80))
        local_ip = s.getsockname()[0]
    except Exception:
        local_ip = '127.0.0.1'
    finally:
        s.close()

    print("\n" + "=" * 60)
    print("Wiz Light Web Controller")
    print("=" * 60)
    print(f"Controlling {len(LIGHT_IPS)} light(s)")
    print(f"\nAccess the controller at:")
    print(f"  Local:   http://127.0.0.1:8080")
    print(f"  Network: http://{local_ip}:8080")
    print("\nPress Ctrl+C to stop the server")
    print("=" * 60 + "\n")

    app.run(host='0.0.0.0', port=8080, debug=True)
