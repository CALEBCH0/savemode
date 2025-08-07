# 💻 Battery Saver App for Windows (Zephyrus G16)

A lightweight one-click battery optimization tool designed specifically for ASUS ROG Zephyrus G16 laptops. Switch between power-saving and performance modes instantly.

## 🎯 Overview

This app provides instant battery optimization by controlling Windows power settings, GPU performance, display brightness, and background processes - all with a single click.

### System Requirements

**Target Hardware:**
- **CPU:** AMD Ryzen AI 9 HX 370 
- **GPU:** NVIDIA RTX 4070 (105W - 85W + 20W Dynamic Boost)
- **RAM:** 32GB LPDDR5X (16GB × 2)
- **Storage:** 1TB + 512GB Samsung PM9A1
- **Display:** 16" 2.5K OLED (2560×1600) ROG Nebula Display

**Software:**
- Windows 10/11
- Python 3.8+
- Administrator privileges

## 🚀 Quick Start

1. **Clone the repository:**
   ```bash
   git clone https://github.com/yourusername/battery-saver-g16.git
   cd battery-saver-g16
   ```

2. **Install dependencies:**
   ```bash
   pip install -r requirements.txt
   ```

3. **Run the application:**
   ```bash
   # Option 1: Use the batch file (auto-requests admin)
   run_as_admin.bat
   
   # Option 2: Run directly with Python (requires admin)
   python battery_saver.py
   ```

## ✅ Implemented Features

| Feature | Description | Status |
|---------|-------------|--------|
| **Power Plan Switching** | Automatically switches between Windows Power Saver and High Performance plans | ✅ |
| **Brightness Control** | Adjusts screen brightness (40% battery / 80% performance) | ✅ |
| **Process Management** | Kills resource-heavy background apps (Armoury Crate, Discord, Steam, etc.) | ✅ |
| **GPU Power Control** | Sets NVIDIA power limits (60W battery / 105W performance) | ✅ |
| **Profile System** | Save and load custom profile configurations | ✅ |
| **GUI Interface** | Clean Tkinter UI with one-click profile switching | ✅ |
| **Admin Detection** | Warns if not running with required permissions | ✅ |
| **Persistent Settings** | Saves preferences to `~/.battery_saver_config.json` | ✅ |

## 📝 TODO Features

| Feature | Description | Priority |
|---------|-------------|----------|
| **CPU Throttling** | Integrate RyzenAdj for limiting CPU boost/TDP | High |
| **Refresh Rate Control** | Switch display between 60Hz and 240Hz | High |
| **MUX Switch** | G-Helper integration for iGPU/dGPU switching | High |
| **RGB Control** | Turn off keyboard backlighting | Medium |
| **System Tray** | Minimize to tray with quick access | Medium |
| **Custom Profiles** | User-editable power profiles | Medium |
| **Battery Status** | Show current battery % and time remaining | Low |
| **Startup Management** | Control startup programs per profile | Low |
| **Windows Update** | Pause updates in battery mode | Low |
| **FPS Limiting** | Global FPS cap via RTSS | Low |
| **SSD Power States** | Optimize storage power consumption | Low |
| **Network Control** | Disable WiFi/Bluetooth when not needed | Low |

## 🔧 Configuration

### Default Profile Settings

**Battery Saver Mode:**
- Brightness: 40%
- Refresh Rate: 60Hz (planned)
- GPU Power Limit: 60W
- Kills background bloatware
- Windows Power Saver plan

**Performance Mode:**
- Brightness: 80%
- Refresh Rate: 240Hz (planned)
- GPU Power Limit: 105W
- Keeps all processes running
- Windows High Performance plan

### Customizing Profiles

Edit `~/.battery_saver_config.json`:
```json
{
  "battery_saver": {
    "brightness": 40,
    "refresh_rate": 60,
    "kill_bloatware": true,
    "gpu_power_limit": 60
  },
  "performance": {
    "brightness": 80,
    "refresh_rate": 240,
    "kill_bloatware": false,
    "gpu_power_limit": 105
  }
}
```

## 🛠️ External Tools

The following tools enhance functionality when available:

| Tool | Purpose | Required |
|------|---------|----------|
| **nvidia-smi** | GPU power management | ✅ Yes |
| **WMI** | Display brightness control | ✅ Yes |
| **RyzenAdj** | CPU power management | ❌ Optional |
| **G-Helper** | MUX switch & RGB control | ❌ Optional |
| **RTSS** | Frame rate limiting | ❌ Optional |

## 📋 Managed Background Processes

The app can automatically terminate these resource-heavy processes in Battery Saver mode:

- Armoury Crate Services
- Game launchers (Steam, Epic, Origin)
- Communication apps (Discord, Teams, Slack)
- Media apps (Spotify)
- Xbox Game Bar components

## 🧪 Testing

Run tests to verify functionality:
```bash
# Test power plan switching
python -m pytest tests/test_power_manager.py

# Test with admin privileges
run_as_admin.bat --test
```

## 🤝 Contributing

Contributions are welcome! Please feel free to submit pull requests or open issues for bugs and feature requests.

## 📄 License

This project is open source and available under the [MIT License](LICENSE).

## ⚠️ Disclaimer

This tool modifies system settings and requires administrator privileges. While designed for the Zephyrus G16, use at your own risk. Always ensure your system is properly cooled when switching to performance mode.