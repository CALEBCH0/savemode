/* SPDX-License-Identifier: LGPL */
/* Standalone RyzenAdj API for Ryzen AI HX370 Power Management */

#ifndef RYZENADJ_STANDALONE_H
#define RYZENADJ_STANDALONE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>

#define RYZENADJ_VER_MAJOR 1
#define RYZENADJ_VER_MINOR 0
#define RYZENADJ_VER_PATCH 0

/* Error codes */
#define ADJ_ERR_FAM_UNSUPPORTED      -1
#define ADJ_ERR_SMU_TIMEOUT          -2
#define ADJ_ERR_SMU_UNSUPPORTED      -3
#define ADJ_ERR_SMU_REJECTED         -4
#define ADJ_ERR_MEMORY_ACCESS        -5
#define ADJ_ERR_INIT_FAILED          -6

/* Ryzen family enum */
typedef enum {
    FAM_UNKNOWN = -1,
    FAM_STRIXPOINT = 31,  /* Ryzen AI HX370 */
    FAM_STRIXHALO = 32,
} ryzen_family_t;

/* Opaque handle for RyzenAdj instance */
typedef struct ryzen_access *ryzen_access;

/* Power Profile Presets */
typedef enum {
    PROFILE_BATTERY_SAVER = 0,
    PROFILE_BALANCED = 1,
    PROFILE_PERFORMANCE = 2,
    PROFILE_CUSTOM = 3
} power_profile_t;

/* Power Profile Configuration */
typedef struct {
    uint32_t stapm_limit;        /* Sustained Power Limit (mW) */
    uint32_t fast_limit;         /* PPT Fast Limit (mW) */
    uint32_t slow_limit;         /* PPT Slow Limit (mW) */
    uint32_t slow_time;          /* Slow PPT Time (s) */
    uint32_t stapm_time;         /* STAPM Time (s) */
    uint32_t tctl_temp;          /* Tctl Temperature Limit (°C) */
    uint32_t vrm_current;        /* VRM Current Limit (mA) */
    uint32_t vrmsoc_current;     /* VRM SoC Current Limit (mA) */
    uint32_t vrmmax_current;     /* VRM Max Current Limit (mA) */
    uint32_t vrmsocmax_current;  /* VRM SoC Max Current Limit (mA) */
    uint32_t max_gfxclk;         /* Max Graphics Clock (MHz) */
    uint32_t min_gfxclk;         /* Min Graphics Clock (MHz) */
    uint32_t max_socclk;         /* Max SoC Clock (MHz) */
    uint32_t min_socclk;         /* Min SoC Clock (MHz) */
    uint32_t max_fclk;           /* Max FCLK (MHz) */
    uint32_t min_fclk;           /* Min FCLK (MHz) */
    uint32_t apu_skin_temp;      /* APU Skin Temperature Limit (°C) */
    uint32_t co_all;             /* Curve Optimizer All Cores */
    uint32_t co_per;             /* Curve Optimizer Per Core */
    int power_saving_mode;       /* Enable power saving mode */
    int max_performance_mode;    /* Enable max performance mode */
} power_profile_config_t;

/* Monitoring Values */
typedef struct {
    float stapm_limit;
    float stapm_value;
    float fast_limit;
    float fast_value;
    float slow_limit;
    float slow_value;
    float tctl_temp;
    float tctl_temp_value;
    float vrm_current;
    float vrm_current_value;
    float vrmsoc_current;
    float vrmsoc_current_value;
    float socket_power;
    float core_clk[16];  /* Per-core frequencies */
    float core_volt[16]; /* Per-core voltages */
    float core_power[16];/* Per-core power */
    float core_temp[16]; /* Per-core temperatures */
    float gfx_clk;
    float gfx_temp;
    float gfx_volt;
    float mem_clk;
    float fclk;
    float soc_power;
    float soc_volt;
} power_metrics_t;

/* Initialize and cleanup */
ryzen_access ryzenadj_init(void);
void ryzenadj_cleanup(ryzen_access ry);

/* Get CPU information */
ryzen_family_t ryzenadj_get_family(ryzen_access ry);
int ryzenadj_get_bios_if_ver(ryzen_access ry);
int ryzenadj_init_table(ryzen_access ry);

/* Apply power profiles */
int ryzenadj_apply_power_profile(ryzen_access ry, power_profile_t profile);
int ryzenadj_apply_custom_profile(ryzen_access ry, const power_profile_config_t *config);
int ryzenadj_get_current_profile(ryzen_access ry, power_profile_config_t *config);

/* Individual power settings */
int ryzenadj_set_stapm_limit(ryzen_access ry, uint32_t value);
int ryzenadj_set_fast_limit(ryzen_access ry, uint32_t value);
int ryzenadj_set_slow_limit(ryzen_access ry, uint32_t value);
int ryzenadj_set_slow_time(ryzen_access ry, uint32_t value);
int ryzenadj_set_stapm_time(ryzen_access ry, uint32_t value);
int ryzenadj_set_tctl_temp(ryzen_access ry, uint32_t value);
int ryzenadj_set_vrm_current(ryzen_access ry, uint32_t value);
int ryzenadj_set_vrmsoc_current(ryzen_access ry, uint32_t value);
int ryzenadj_set_vrmmax_current(ryzen_access ry, uint32_t value);
int ryzenadj_set_vrmsocmax_current(ryzen_access ry, uint32_t value);
int ryzenadj_set_max_gfxclk(ryzen_access ry, uint32_t value);
int ryzenadj_set_min_gfxclk(ryzen_access ry, uint32_t value);
int ryzenadj_set_max_socclk(ryzen_access ry, uint32_t value);
int ryzenadj_set_min_socclk(ryzen_access ry, uint32_t value);
int ryzenadj_set_max_fclk(ryzen_access ry, uint32_t value);
int ryzenadj_set_min_fclk(ryzen_access ry, uint32_t value);
int ryzenadj_set_apu_skin_temp(ryzen_access ry, uint32_t value);
int ryzenadj_set_co_all(ryzen_access ry, uint32_t value);
int ryzenadj_set_co_per(ryzen_access ry, uint32_t value);

/* Power modes */
int ryzenadj_set_power_saving(ryzen_access ry);
int ryzenadj_set_max_performance(ryzen_access ry);

/* Monitoring */
int ryzenadj_get_metrics(ryzen_access ry, power_metrics_t *metrics);
int ryzenadj_refresh_table(ryzen_access ry);

/* Get individual metrics */
float ryzenadj_get_stapm_limit(ryzen_access ry);
float ryzenadj_get_stapm_value(ryzen_access ry);
float ryzenadj_get_fast_limit(ryzen_access ry);
float ryzenadj_get_fast_value(ryzen_access ry);
float ryzenadj_get_slow_limit(ryzen_access ry);
float ryzenadj_get_slow_value(ryzen_access ry);
float ryzenadj_get_tctl_temp(ryzen_access ry);
float ryzenadj_get_tctl_temp_value(ryzen_access ry);
float ryzenadj_get_socket_power(ryzen_access ry);
float ryzenadj_get_core_clk(ryzen_access ry, uint32_t core);
float ryzenadj_get_core_volt(ryzen_access ry, uint32_t core);
float ryzenadj_get_core_power(ryzen_access ry, uint32_t core);
float ryzenadj_get_core_temp(ryzen_access ry, uint32_t core);
float ryzenadj_get_gfx_clk(ryzen_access ry);
float ryzenadj_get_gfx_temp(ryzen_access ry);
float ryzenadj_get_mem_clk(ryzen_access ry);
float ryzenadj_get_fclk(ryzen_access ry);
float ryzenadj_get_soc_power(ryzen_access ry);

#ifdef __cplusplus
}
#endif

#endif /* RYZENADJ_STANDALONE_H */