/* SPDX-License-Identifier: LGPL */
/* Standalone RyzenAdj Implementation for Ryzen AI HX370 */

#include "ryzenadj.h"
#include "ryzenadj_priv.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

#ifndef _WIN32
#include <unistd.h>
#define Sleep(x) usleep((x)*1000)
#else
#include <windows.h>
#endif

/* Predefined power profiles for HX370 */
static const power_profile_config_t BATTERY_SAVER_PROFILE = {
    .stapm_limit = 15000,        /* 15W */
    .fast_limit = 20000,         /* 20W */
    .slow_limit = 17000,         /* 17W */
    .slow_time = 10,
    .stapm_time = 300,
    .tctl_temp = 75,             /* 75°C */
    .vrm_current = 35000,        /* 35A */
    .vrmsoc_current = 15000,     /* 15A */
    .vrmmax_current = 45000,     /* 45A */
    .vrmsocmax_current = 20000,  /* 20A */
    .max_gfxclk = 1500,          /* 1.5GHz */
    .min_gfxclk = 400,           /* 400MHz */
    .max_socclk = 0,             /* Auto */
    .min_socclk = 0,             /* Auto */
    .max_fclk = 0,               /* Auto */
    .min_fclk = 0,               /* Auto */
    .apu_skin_temp = 40,         /* 40°C */
    .co_all = 0,                 /* No curve optimizer */
    .co_per = 0,
    .power_saving_mode = 1,
    .max_performance_mode = 0
};

static const power_profile_config_t BALANCED_PROFILE = {
    .stapm_limit = 28000,        /* 28W */
    .fast_limit = 35000,         /* 35W */
    .slow_limit = 30000,         /* 30W */
    .slow_time = 10,
    .stapm_time = 200,
    .tctl_temp = 85,             /* 85°C */
    .vrm_current = 50000,        /* 50A */
    .vrmsoc_current = 20000,     /* 20A */
    .vrmmax_current = 65000,     /* 65A */
    .vrmsocmax_current = 25000,  /* 25A */
    .max_gfxclk = 2200,          /* 2.2GHz */
    .min_gfxclk = 400,           /* 400MHz */
    .max_socclk = 0,             /* Auto */
    .min_socclk = 0,             /* Auto */
    .max_fclk = 0,               /* Auto */
    .min_fclk = 0,               /* Auto */
    .apu_skin_temp = 45,         /* 45°C */
    .co_all = 0,
    .co_per = 0,
    .power_saving_mode = 0,
    .max_performance_mode = 0
};

static const power_profile_config_t PERFORMANCE_PROFILE = {
    .stapm_limit = 45000,        /* 45W */
    .fast_limit = 54000,         /* 54W */
    .slow_limit = 50000,         /* 50W */
    .slow_time = 5,
    .stapm_time = 100,
    .tctl_temp = 95,             /* 95°C */
    .vrm_current = 70000,        /* 70A */
    .vrmsoc_current = 25000,     /* 25A */
    .vrmmax_current = 90000,     /* 90A */
    .vrmsocmax_current = 30000,  /* 30A */
    .max_gfxclk = 2900,          /* 2.9GHz max for HX370 */
    .min_gfxclk = 800,           /* 800MHz */
    .max_socclk = 0,             /* Auto */
    .min_socclk = 0,             /* Auto */
    .max_fclk = 0,               /* Auto */
    .min_fclk = 0,               /* Auto */
    .apu_skin_temp = 50,         /* 50°C */
    .co_all = 10,                /* Mild curve optimizer */
    .co_per = 0,
    .power_saving_mode = 0,
    .max_performance_mode = 1
};

/* Initialize RyzenAdj */
ryzen_access ryzenadj_init(void) {
    ryzen_access ry;
    enum ryzen_family family = cpuid_get_family();
    
    /* Check if CPU is supported (HX370 is STRIXPOINT) */
    if (family != FAM_STRIXPOINT && family != FAM_STRIXHALO) {
        printf("Unsupported CPU family. This library supports Ryzen AI HX370 (Strix Point) only.\n");
        return NULL;
    }
    
    ry = (ryzen_access)calloc(1, sizeof(struct _ryzen_access));
    if (!ry) {
        printf("Memory allocation failed\n");
        return NULL;
    }
    
    ry->family = family;
    ry->bios_if_ver = 0;
    ry->table_values = NULL;
    
    /* Initialize OS access layer */
    ry->os_access = init_os_access_obj();
    if (!ry->os_access) {
        printf("Failed to initialize OS access layer. Run as administrator/root.\n");
        free(ry);
        return NULL;
    }
    
    /* Initialize SMU interfaces */
    ry->mp1_smu = get_smu(ry->os_access, TYPE_MP1);
    if (!ry->mp1_smu) {
        printf("Failed to initialize MP1 SMU interface\n");
        free_os_access_obj(ry->os_access);
        free(ry);
        return NULL;
    }
    
    ry->psmu = get_smu(ry->os_access, TYPE_PSMU);
    if (!ry->psmu) {
        printf("Failed to initialize PSMU interface\n");
        free(ry->mp1_smu);
        free_os_access_obj(ry->os_access);
        free(ry);
        return NULL;
    }
    
    return ry;
}

/* Cleanup */
void ryzenadj_cleanup(ryzen_access ry) {
    if (!ry) return;
    
    if (ry->table_values) free(ry->table_values);
    if (ry->mp1_smu) free(ry->mp1_smu);
    if (ry->psmu) free(ry->psmu);
    if (ry->os_access) free_os_access_obj(ry->os_access);
    free(ry);
}

/* Get CPU family */
ryzen_family_t ryzenadj_get_family(ryzen_access ry) {
    if (!ry) return FAM_UNKNOWN;
    return (ryzen_family_t)ry->family;
}

/* Get BIOS interface version */
int ryzenadj_get_bios_if_ver(ryzen_access ry) {
    if (!ry) return ADJ_ERR_INIT_FAILED;
    
    if (ry->bios_if_ver) return ry->bios_if_ver;
    
    smu_service_args_t args = {0, 0, 0, 0, 0, 0};
    smu_service_req(ry->mp1_smu, 0x3, &args);
    ry->bios_if_ver = args.arg0;
    return ry->bios_if_ver;
}

/* Initialize PM table */
int ryzenadj_init_table(ryzen_access ry) {
    if (!ry) return ADJ_ERR_INIT_FAILED;
    
    smu_service_args_t args = {0, 0, 0, 0, 0, 0};
    int resp;
    
    /* Get table version and size - STRIXPOINT uses message 0x6 */
    resp = smu_service_req(ry->psmu, 0x6, &args);
    if (resp != REP_MSG_OK) {
        return ADJ_ERR_SMU_REJECTED;
    }
    
    ry->table_ver = args.arg0;
    
    /* Set table size based on version */
    switch (ry->table_ver) {
        case 0x5D0008:
        case 0x5D0009: 
            ry->table_size = 0xD54; 
            break;
        case 0x64020c: 
            ry->table_size = 0xE50; 
            break;
        default: 
            ry->table_size = 0x1000; /* Default large size for unknown tables */
            break;
    }
    
    /* Allocate table memory */
    if (ry->table_values) {
        free(ry->table_values);
    }
    ry->table_values = calloc(ry->table_size, sizeof(float));
    if (!ry->table_values) {
        return ADJ_ERR_MEMORY_ACCESS;
    }
    
    /* Get table address - STRIXPOINT uses message 0x5 with arg0=3 */
    args.arg0 = 3;
    resp = smu_service_req(ry->psmu, 0x5, &args);
    if (resp != REP_MSG_OK) {
        return ADJ_ERR_SMU_REJECTED;
    }
    
    /* Map table memory */
    ry->table_addr = args.arg0;
    
    return 0;
}

/* Helper macro for SMU adjustments */
#define _do_adjust(MSG) \
    do { \
        smu_service_args_t args = {value, 0, 0, 0, 0, 0}; \
        int resp = smu_service_req(ry->mp1_smu, MSG, &args); \
        if (resp != REP_MSG_OK) return ADJ_ERR_SMU_REJECTED; \
    } while(0)

#define _do_adjust_psmu(MSG) \
    do { \
        smu_service_args_t args = {value, 0, 0, 0, 0, 0}; \
        int resp = smu_service_req(ry->psmu, MSG, &args); \
        if (resp != REP_MSG_OK) return ADJ_ERR_SMU_REJECTED; \
    } while(0)

/* Power limit functions - STRIXPOINT specific message IDs */
int ryzenadj_set_stapm_limit(ryzen_access ry, uint32_t value) {
    if (!ry) return ADJ_ERR_INIT_FAILED;
    _do_adjust(0x14);  /* STRIXPOINT uses 0x14 */
    return 0;
}

int ryzenadj_set_fast_limit(ryzen_access ry, uint32_t value) {
    if (!ry) return ADJ_ERR_INIT_FAILED;
    _do_adjust(0x15);  /* STRIXPOINT uses 0x15 */
    return 0;
}

int ryzenadj_set_slow_limit(ryzen_access ry, uint32_t value) {
    if (!ry) return ADJ_ERR_INIT_FAILED;
    _do_adjust(0x16);  /* STRIXPOINT uses 0x16 */
    return 0;
}

int ryzenadj_set_slow_time(ryzen_access ry, uint32_t value) {
    if (!ry) return ADJ_ERR_INIT_FAILED;
    _do_adjust(0x17);  /* STRIXPOINT uses 0x17 */
    return 0;
}

int ryzenadj_set_stapm_time(ryzen_access ry, uint32_t value) {
    if (!ry) return ADJ_ERR_INIT_FAILED;
    _do_adjust(0x18);  /* STRIXPOINT uses 0x18 */
    return 0;
}

int ryzenadj_set_tctl_temp(ryzen_access ry, uint32_t value) {
    if (!ry) return ADJ_ERR_INIT_FAILED;
    _do_adjust(0x19);  /* STRIXPOINT uses 0x19 */
    return 0;
}

int ryzenadj_set_vrm_current(ryzen_access ry, uint32_t value) {
    if (!ry) return ADJ_ERR_INIT_FAILED;
    _do_adjust(0x1a);  /* STRIXPOINT uses 0x1a */
    return 0;
}

int ryzenadj_set_vrmsoc_current(ryzen_access ry, uint32_t value) {
    if (!ry) return ADJ_ERR_INIT_FAILED;
    _do_adjust(0x1b);  /* STRIXPOINT uses 0x1b */
    return 0;
}

int ryzenadj_set_vrmmax_current(ryzen_access ry, uint32_t value) {
    if (!ry) return ADJ_ERR_INIT_FAILED;
    _do_adjust(0x1c);  /* STRIXPOINT uses 0x1c */
    return 0;
}

int ryzenadj_set_vrmsocmax_current(ryzen_access ry, uint32_t value) {
    if (!ry) return ADJ_ERR_INIT_FAILED;
    _do_adjust(0x1d);  /* STRIXPOINT uses 0x1d */
    return 0;
}

int ryzenadj_set_max_gfxclk(ryzen_access ry, uint32_t value) {
    if (!ry) return ADJ_ERR_INIT_FAILED;
    _do_adjust_psmu(0xB1);  /* STRIXPOINT uses PSMU 0xB1 */
    return 0;
}

int ryzenadj_set_min_gfxclk(ryzen_access ry, uint32_t value) {
    if (!ry) return ADJ_ERR_INIT_FAILED;
    _do_adjust_psmu(0xB2);  /* STRIXPOINT uses PSMU 0xB2 */
    return 0;
}

int ryzenadj_set_max_socclk(ryzen_access ry, uint32_t value) {
    if (!ry) return ADJ_ERR_INIT_FAILED;
    _do_adjust_psmu(0xB3);  /* STRIXPOINT uses PSMU 0xB3 */
    return 0;
}

int ryzenadj_set_min_socclk(ryzen_access ry, uint32_t value) {
    if (!ry) return ADJ_ERR_INIT_FAILED;
    _do_adjust_psmu(0xB4);  /* STRIXPOINT uses PSMU 0xB4 */
    return 0;
}

int ryzenadj_set_max_fclk(ryzen_access ry, uint32_t value) {
    if (!ry) return ADJ_ERR_INIT_FAILED;
    _do_adjust_psmu(0xB5);  /* STRIXPOINT uses PSMU 0xB5 */
    return 0;
}

int ryzenadj_set_min_fclk(ryzen_access ry, uint32_t value) {
    if (!ry) return ADJ_ERR_INIT_FAILED;
    _do_adjust_psmu(0xB6);  /* STRIXPOINT uses PSMU 0xB6 */
    return 0;
}

int ryzenadj_set_apu_skin_temp(ryzen_access ry, uint32_t value) {
    if (!ry) return ADJ_ERR_INIT_FAILED;
    _do_adjust(0x38);  /* STRIXPOINT uses 0x38 */
    return 0;
}

int ryzenadj_set_co_all(ryzen_access ry, uint32_t value) {
    if (!ry) return ADJ_ERR_INIT_FAILED;
    _do_adjust(0x4C);  /* STRIXPOINT uses 0x4C */
    return 0;
}

int ryzenadj_set_co_per(ryzen_access ry, uint32_t value) {
    if (!ry) return ADJ_ERR_INIT_FAILED;
    _do_adjust(0x4b);  /* STRIXPOINT uses 0x4b */
    return 0;
}

/* Power mode functions */
int ryzenadj_set_power_saving(ryzen_access ry) {
    if (!ry) return ADJ_ERR_INIT_FAILED;
    uint32_t value = 0;
    _do_adjust(0x12);  /* STRIXPOINT uses 0x12 for power saving */
    return 0;
}

int ryzenadj_set_max_performance(ryzen_access ry) {
    if (!ry) return ADJ_ERR_INIT_FAILED;
    uint32_t value = 0;
    _do_adjust(0x11);  /* STRIXPOINT uses 0x11 for max performance */
    return 0;
}

/* Apply predefined power profile */
int ryzenadj_apply_power_profile(ryzen_access ry, power_profile_t profile) {
    if (!ry) return ADJ_ERR_INIT_FAILED;
    
    const power_profile_config_t *config;
    
    switch (profile) {
        case PROFILE_BATTERY_SAVER:
            config = &BATTERY_SAVER_PROFILE;
            break;
        case PROFILE_BALANCED:
            config = &BALANCED_PROFILE;
            break;
        case PROFILE_PERFORMANCE:
            config = &PERFORMANCE_PROFILE;
            break;
        default:
            return ADJ_ERR_SMU_UNSUPPORTED;
    }
    
    return ryzenadj_apply_custom_profile(ry, config);
}

/* Apply custom power profile */
int ryzenadj_apply_custom_profile(ryzen_access ry, const power_profile_config_t *config) {
    if (!ry || !config) return ADJ_ERR_INIT_FAILED;
    
    int err = 0;
    
    /* Apply power limits */
    if (config->stapm_limit > 0) {
        err = ryzenadj_set_stapm_limit(ry, config->stapm_limit);
        if (err) return err;
    }
    
    if (config->fast_limit > 0) {
        err = ryzenadj_set_fast_limit(ry, config->fast_limit);
        if (err) return err;
    }
    
    if (config->slow_limit > 0) {
        err = ryzenadj_set_slow_limit(ry, config->slow_limit);
        if (err) return err;
    }
    
    /* Apply time constants */
    if (config->slow_time > 0) {
        err = ryzenadj_set_slow_time(ry, config->slow_time);
        if (err) return err;
    }
    
    if (config->stapm_time > 0) {
        err = ryzenadj_set_stapm_time(ry, config->stapm_time);
        if (err) return err;
    }
    
    /* Apply temperature limits */
    if (config->tctl_temp > 0) {
        err = ryzenadj_set_tctl_temp(ry, config->tctl_temp);
        if (err) return err;
    }
    
    if (config->apu_skin_temp > 0) {
        err = ryzenadj_set_apu_skin_temp(ry, config->apu_skin_temp);
        if (err) return err;
    }
    
    /* Apply current limits */
    if (config->vrm_current > 0) {
        err = ryzenadj_set_vrm_current(ry, config->vrm_current);
        if (err) return err;
    }
    
    if (config->vrmsoc_current > 0) {
        err = ryzenadj_set_vrmsoc_current(ry, config->vrmsoc_current);
        if (err) return err;
    }
    
    if (config->vrmmax_current > 0) {
        err = ryzenadj_set_vrmmax_current(ry, config->vrmmax_current);
        if (err) return err;
    }
    
    if (config->vrmsocmax_current > 0) {
        err = ryzenadj_set_vrmsocmax_current(ry, config->vrmsocmax_current);
        if (err) return err;
    }
    
    /* Apply frequency limits */
    if (config->max_gfxclk > 0) {
        err = ryzenadj_set_max_gfxclk(ry, config->max_gfxclk);
        if (err) return err;
    }
    
    if (config->min_gfxclk > 0) {
        err = ryzenadj_set_min_gfxclk(ry, config->min_gfxclk);
        if (err) return err;
    }
    
    if (config->max_socclk > 0) {
        err = ryzenadj_set_max_socclk(ry, config->max_socclk);
        if (err) return err;
    }
    
    if (config->min_socclk > 0) {
        err = ryzenadj_set_min_socclk(ry, config->min_socclk);
        if (err) return err;
    }
    
    if (config->max_fclk > 0) {
        err = ryzenadj_set_max_fclk(ry, config->max_fclk);
        if (err) return err;
    }
    
    if (config->min_fclk > 0) {
        err = ryzenadj_set_min_fclk(ry, config->min_fclk);
        if (err) return err;
    }
    
    /* Apply curve optimizer */
    if (config->co_all != 0) {
        err = ryzenadj_set_co_all(ry, config->co_all);
        if (err) return err;
    }
    
    if (config->co_per != 0) {
        err = ryzenadj_set_co_per(ry, config->co_per);
        if (err) return err;
    }
    
    /* Apply power modes */
    if (config->power_saving_mode) {
        err = ryzenadj_set_power_saving(ry);
        if (err) return err;
    } else if (config->max_performance_mode) {
        err = ryzenadj_set_max_performance(ry);
        if (err) return err;
    }
    
    return 0;
}

/* Refresh PM table */
int ryzenadj_refresh_table(ryzen_access ry) {
    if (!ry) return ADJ_ERR_INIT_FAILED;
    if (!ry->table_values) return ADJ_ERR_MEMORY_ACCESS;
    
    /* Read PM table from SMU */
    smu_service_args_t args = {0, 0, 0, 0, 0, 0};
    int resp = smu_service_req(ry->psmu, 0x3, &args);
    
    if (resp != REP_MSG_OK) {
        return ADJ_ERR_SMU_REJECTED;
    }
    
    return 0;
}

/* Helper to read float value from PM table */
#define _read_float_value(OFFSET) \
    do { \
        if (!ry || !ry->table_values) return NAN; \
        if (OFFSET >= ry->table_size) return NAN; \
        return ry->table_values[OFFSET / sizeof(float)]; \
    } while(0)

/* Monitoring functions */
float ryzenadj_get_stapm_limit(ryzen_access ry) {
    _read_float_value(0x0);
}

float ryzenadj_get_stapm_value(ryzen_access ry) {
    _read_float_value(0x4);
}

float ryzenadj_get_fast_limit(ryzen_access ry) {
    _read_float_value(0x8);
}

float ryzenadj_get_fast_value(ryzen_access ry) {
    _read_float_value(0xC);
}

float ryzenadj_get_slow_limit(ryzen_access ry) {
    _read_float_value(0x10);
}

float ryzenadj_get_slow_value(ryzen_access ry) {
    _read_float_value(0x14);
}

float ryzenadj_get_tctl_temp(ryzen_access ry) {
    /* STRIXPOINT specific offset */
    _read_float_value(0xE0);
}

float ryzenadj_get_tctl_temp_value(ryzen_access ry) {
    /* STRIXPOINT specific offset */
    _read_float_value(0xE4);
}

float ryzenadj_get_socket_power(ryzen_access ry) {
    /* STRIXPOINT specific offset */
    _read_float_value(0xC0);
}

/* Get all metrics at once */
int ryzenadj_get_metrics(ryzen_access ry, power_metrics_t *metrics) {
    if (!ry || !metrics) return ADJ_ERR_INIT_FAILED;
    
    /* Refresh table first */
    int err = ryzenadj_refresh_table(ry);
    if (err) return err;
    
    /* Read basic power metrics */
    metrics->stapm_limit = ryzenadj_get_stapm_limit(ry);
    metrics->stapm_value = ryzenadj_get_stapm_value(ry);
    metrics->fast_limit = ryzenadj_get_fast_limit(ry);
    metrics->fast_value = ryzenadj_get_fast_value(ry);
    metrics->slow_limit = ryzenadj_get_slow_limit(ry);
    metrics->slow_value = ryzenadj_get_slow_value(ry);
    metrics->tctl_temp = ryzenadj_get_tctl_temp(ry);
    metrics->tctl_temp_value = ryzenadj_get_tctl_temp_value(ry);
    metrics->socket_power = ryzenadj_get_socket_power(ry);
    
    /* Note: Per-core metrics would require additional PM table mapping */
    /* which is complex and varies by table version */
    
    return 0;
}