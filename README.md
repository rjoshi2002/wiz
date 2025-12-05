# Wiz Light Controller

A comprehensive Python-based controller for Wiz smart lights with both CLI and web interfaces.

## Features

- **CLI Interface** - Interactive command-line menu for local control
- **Web Interface** - Beautiful, responsive web UI accessible from any device on your network
- **Full Control** - Power, brightness, RGB colors, color temperature
- **Preset Options** - Quick access to common colors and temperatures
- **Multi-Light Support** - Control multiple lights simultaneously
- **Status Monitoring** - Check which lights are online and their current state

## Requirements

- Python 3.7+
- Wiz smart lights connected to WiFi
- IP addresses of your lights

## Installation

1. Clone this repository:
```bash
git clone <your-repo-url>
cd wiz
```

2. Create and activate a virtual environment:
```bash
python3 -m venv venv
source venv/bin/activate  # On Windows: venv\Scripts\activate
```

3. Install dependencies (for web interface):
```bash
pip install -r requirements.txt
```

## Configuration

Edit the `LIGHT_IPS` list in `wiz_control.py` with your light IP addresses:

```python
LIGHT_IPS = [
    "192.168.1.100",
    "192.168.1.101",
    # Add more IPs as needed
]
```

## Usage

### CLI Interface

Run the interactive command-line interface:

```bash
python wiz_control.py
```

**Menu Options:**
- 1 - Turn all lights ON
- 2 - Turn all lights OFF
- 3 - Set color (hex code)
- 4 - Set color (preset)
- 5 - Set color temperature (Kelvin)
- 6 - Set brightness
- 7 - Check status
- 0 - Exit

### Web Interface

Start the web server:

```bash
python web_server.py
```

Access from any device on your network:
- Local: `http://127.0.0.1:8080`
- Network: `http://<your-ip>:8080`

**Web Features:**
- Touch-friendly controls for mobile devices
- Real-time brightness slider
- Color picker with hex input
- 8 preset colors
- Color temperature control (2200K-6500K)
- Live status display
- Toast notifications

## Project Structure

```
wiz/
├── wiz_control.py          # CLI interface and core functions
├── web_server.py            # Flask web server
├── templates/
│   └── index.html          # Web UI
├── requirements.txt         # Python dependencies
├── .gitignore              # Git ignore rules
└── README.md               # This file
```

## API Endpoints

The web server exposes these REST API endpoints:

- `POST /api/lights/on` - Turn all lights on
- `POST /api/lights/off` - Turn all lights off
- `POST /api/lights/color` - Set RGB color and brightness
- `POST /api/lights/temperature` - Set color temperature
- `POST /api/lights/brightness` - Set brightness only
- `GET /api/lights/status` - Get status of all lights

## Technical Details

- **Protocol**: UDP port 38899
- **Format**: JSON-based Wiz protocol
- **Commands**: setPilot, getPilot methods
- **Web Framework**: Flask
- **No external dependencies** for CLI (uses Python standard library)

## Screenshots

### CLI Interface
Interactive menu with color presets and temperature control

### Web Interface
Responsive design works on phones, tablets, and desktop browsers

## Contributing

Feel free to open issues or submit pull requests!

## License

MIT License

## Acknowledgments

- Wiz smart lighting protocol
- Flask web framework
