# Standalone RyzenAdj Library for Ryzen AI HX370

A lightweight, standalone library for power management of AMD Ryzen AI HX370 (Strix Point) processors. This library extracts the essential functionality from the full RyzenAdj project into a clean, importable API.

## Features

- **CPU Support**: AMD Ryzen AI HX370 (Strix Point) processors
- **Power Profiles**: Battery Saver, Balanced, Performance presets
- **Custom Profiles**: Create and apply custom power configurations
- **Real-time Monitoring**: Read power metrics, temperatures, and frequencies
- **Cross-platform**: Linux and Windows support
- **Clean API**: Simple C interface for integration

## Prerequisites

### Linux
- Root privileges (required for `/dev/mem` access)
- GCC or compatible C compiler
- AMD Ryzen AI HX370 processor

### Windows
- Administrator privileges
- MSVC or MinGW compiler
- Low-level memory access driver (WinRing0 or similar)

## Quick Start

### 1. Include the Library

```c
#include "ryzenadj.h"
```

### 2. Initialize

```c
ryzen_access ry = ryzenadj_init();
if (!ry) {
    printf("Failed to initialize RyzenAdj\n");
    return -1;
}
```

### 3. Apply Power Profile

```c
// Apply battery saver profile
int result = ryzenadj_apply_power_profile(ry, PROFILE_BATTERY_SAVER);
if (result != 0) {
    printf("Failed to apply profile\n");
}
```

### 4. Cleanup

```c
ryzenadj_cleanup(ry);
```

## API Reference

### Initialization and Cleanup

```c
ryzen_access ryzenadj_init(void);
void ryzenadj_cleanup(ryzen_access ry);
```

### Power Profiles

#### Predefined Profiles

```c
typedef enum {
    PROFILE_BATTERY_SAVER = 0,  // 15W sustained, 75°C, efficiency optimized
    PROFILE_BALANCED = 1,       // 28W sustained, 85°C, balanced performance
    PROFILE_PERFORMANCE = 2,    // 45W sustained, 95°C, maximum performance
    PROFILE_CUSTOM = 3          // User-defined configuration
} power_profile_t;

int ryzenadj_apply_power_profile(ryzen_access ry, power_profile_t profile);
```

#### Custom Profiles

```c
typedef struct {
    uint32_t stapm_limit;        /* Sustained Power Limit (mW) */
    uint32_t fast_limit;         /* PPT Fast Limit (mW) */
    uint32_t slow_limit;         /* PPT Slow Limit (mW) */
    uint32_t tctl_temp;          /* Temperature Limit (°C) */
    uint32_t vrm_current;        /* VRM Current Limit (mA) */
    uint32_t max_gfxclk;         /* Max Graphics Clock (MHz) */
    uint32_t min_gfxclk;         /* Min Graphics Clock (MHz) */
    int power_saving_mode;       /* Enable power saving optimizations */
    int max_performance_mode;    /* Enable performance optimizations */
    /* ... more parameters ... */
} power_profile_config_t;

int ryzenadj_apply_custom_profile(ryzen_access ry, const power_profile_config_t *config);
```

### Individual Settings

```c
int ryzenadj_set_stapm_limit(ryzen_access ry, uint32_t value);    // Sustained power (mW)
int ryzenadj_set_fast_limit(ryzen_access ry, uint32_t value);     // Peak power (mW)
int ryzenadj_set_tctl_temp(ryzen_access ry, uint32_t value);      // CPU temperature (°C)
int ryzenadj_set_max_gfxclk(ryzen_access ry, uint32_t value);     // Max GPU clock (MHz)
int ryzenadj_set_power_saving(ryzen_access ry);                  // Enable efficiency mode
int ryzenadj_set_max_performance(ryzen_access ry);               // Enable performance mode
```

### Monitoring

```c
typedef struct {
    float stapm_limit;      /* Current sustained power limit */
    float stapm_value;      /* Current sustained power usage */
    float fast_limit;       /* Current peak power limit */
    float fast_value;       /* Current peak power usage */
    float tctl_temp;        /* Temperature limit */
    float tctl_temp_value;  /* Current temperature */
    float socket_power;     /* Total socket power */
    /* ... more metrics ... */
} power_metrics_t;

int ryzenadj_get_metrics(ryzen_access ry, power_metrics_t *metrics);
float ryzenadj_get_socket_power(ryzen_access ry);
float ryzenadj_get_tctl_temp_value(ryzen_access ry);
```

## Example Usage

### Basic Profile Switching

```c
#include "ryzenadj.h"
#include <stdio.h>

int main() {
    ryzen_access ry = ryzenadj_init();
    if (!ry) {
        printf("Failed to initialize RyzenAdj\n");
        return -1;
    }
    
    // Check CPU family
    ryzen_family_t family = ryzenadj_get_family(ry);
    if (family != FAM_STRIXPOINT) {
        printf("Unsupported CPU\n");
        ryzenadj_cleanup(ry);
        return -1;
    }
    
    // Apply performance profile
    int result = ryzenadj_apply_power_profile(ry, PROFILE_PERFORMANCE);
    if (result == 0) {
        printf("Performance profile applied successfully\n");
    } else {
        printf("Failed to apply profile: %d\n", result);
    }
    
    ryzenadj_cleanup(ry);
    return 0;
}
```

### Custom Power Configuration

```c
#include "ryzenadj.h"

int apply_custom_gaming_profile(ryzen_access ry) {
    power_profile_config_t gaming_profile = {
        .stapm_limit = 35000,        // 35W sustained
        .fast_limit = 45000,         // 45W peak
        .slow_limit = 40000,         // 40W average
        .tctl_temp = 90,             // 90°C limit
        .vrm_current = 60000,        // 60A VRM
        .max_gfxclk = 2700,          // 2.7GHz GPU
        .min_gfxclk = 800,           // 800MHz GPU minimum
        .power_saving_mode = 0,
        .max_performance_mode = 1
    };
    
    return ryzenadj_apply_custom_profile(ry, &gaming_profile);
}
```

### Power Monitoring

```c
#include "ryzenadj.h"
#include <stdio.h>

void monitor_power(ryzen_access ry) {
    power_metrics_t metrics;
    
    if (ryzenadj_get_metrics(ry, &metrics) == 0) {
        printf("Power Usage: %.2fW / %.2fW\n", 
               metrics.stapm_value, metrics.stapm_limit);
        printf("Temperature: %.1f°C / %.1f°C\n",
               metrics.tctl_temp_value, metrics.tctl_temp);
        printf("Socket Power: %.2fW\n", metrics.socket_power);
    }
}
```

## Error Codes

```c
#define ADJ_ERR_FAM_UNSUPPORTED      -1  // Unsupported CPU family
#define ADJ_ERR_SMU_TIMEOUT          -2  // SMU communication timeout
#define ADJ_ERR_SMU_UNSUPPORTED      -3  // Unsupported SMU operation
#define ADJ_ERR_SMU_REJECTED         -4  // SMU rejected the command
#define ADJ_ERR_MEMORY_ACCESS        -5  // Memory access error
#define ADJ_ERR_INIT_FAILED          -6  // Initialization failed
```

## Compilation

### Linux

```bash
gcc -o test_app test_app.c ryzenadj.c smu_ops.c -lm
sudo ./test_app
```

### Windows (with MinGW)

```bash
gcc -o test_app.exe test_app.c ryzenadj.c smu_ops.c
# Run as administrator
test_app.exe
```

## Power Profile Details

### Battery Saver Profile
- **Power**: 15W sustained, 20W peak
- **Temperature**: 75°C limit
- **GPU**: 1.5GHz max, efficiency focused
- **Features**: Power saving mode enabled

### Balanced Profile  
- **Power**: 28W sustained, 35W peak
- **Temperature**: 85°C limit
- **GPU**: 2.2GHz max, balanced clocks
- **Features**: Default AMD behavior

### Performance Profile
- **Power**: 45W sustained, 54W peak
- **Temperature**: 95°C limit  
- **GPU**: 2.9GHz max, maximum clocks
- **Features**: Performance mode enabled, curve optimizer

## Important Notes

- **Root/Administrator privileges required** for hardware access
- **CPU support**: Only Ryzen AI HX370 (Strix Point) processors
- **Thermal management**: Monitor temperatures to prevent overheating
- **Power delivery**: Ensure adequate cooling and power supply
- **BIOS settings**: Some settings may be overridden by BIOS/firmware

## Integration Tips

1. **Error Handling**: Always check return codes
2. **Initialization**: Call `ryzenadj_init()` once at startup
3. **Cleanup**: Always call `ryzenadj_cleanup()` before exit
4. **Monitoring**: Refresh metrics before reading values
5. **Threading**: Library is not thread-safe, use external synchronization

## Troubleshooting

### Linux
- **Permission denied**: Run as root or set proper udev rules
- **Segmentation fault**: Check CPU support and initialization
- **Values not applying**: Verify BIOS settings and thermal limits

### Windows  
- **Access denied**: Run as Administrator
- **Driver not found**: Install WinRing0 or similar low-level access driver
- **SMU timeout**: Check for conflicting software (MSI Afterburner, etc.)

## License

LGPL - Same as the original RyzenAdj project

## Contributing

This is a minimal extraction from RyzenAdj. For full features and updates, see the main RyzenAdj repository at https://github.com/FlyGoat/RyzenAdj