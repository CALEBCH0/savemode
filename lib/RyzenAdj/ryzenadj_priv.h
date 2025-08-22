/* SPDX-License-Identifier: LGPL */
/* Private structures and definitions for standalone RyzenAdj */

#ifndef RYZENADJ_PRIV_H
#define RYZENADJ_PRIV_H

#include <stdint.h>
#include <stddef.h>

/* Platform-specific includes */
#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#include <sys/mman.h>
#include <fcntl.h>
#endif

/* Ryzen family enumeration (internal) */
enum ryzen_family {
    FAM_UNKNOWN = -1,
    FAM_STRIXPOINT = 31,
    FAM_STRIXHALO = 32,
};

/* SMU response codes */
#define REP_MSG_OK                    0x1
#define REP_MSG_Failed                0xFF
#define REP_MSG_UnknownCmd            0xFE
#define REP_MSG_CmdRejectedPrereq     0xFD
#define REP_MSG_CmdRejectedBusy       0xFC

/* SMU types */
typedef enum {
    TYPE_MP1,
    TYPE_PSMU
} smu_type_t;

/* SMU service arguments structure */
typedef struct {
    uint32_t arg0;
    uint32_t arg1;
    uint32_t arg2;
    uint32_t arg3;
    uint32_t arg4;
    uint32_t arg5;
} smu_service_args_t;

/* Forward declarations */
typedef struct _os_access *os_access_t;
typedef struct _smu *smu_t;

/* SMU access structure */
struct _smu {
    uint32_t req;        /* Request register */
    uint32_t rsp;        /* Response register */
    uint32_t arg_base;   /* Arguments base address */
    os_access_t os;      /* OS access object */
};

/* OS access structure */
struct _os_access {
#ifdef _WIN32
    HANDLE driver_handle;
#else
    int mem_fd;
    void *mapped_mem;
    size_t mapped_size;
#endif
    uint32_t nb_base_addr;
};

/* Main RyzenAdj access structure */
struct _ryzen_access {
    enum ryzen_family family;
    int bios_if_ver;
    uint32_t table_ver;
    size_t table_size;
    uint32_t table_addr;
    float *table_values;
    os_access_t os_access;
    smu_t mp1_smu;
    smu_t psmu;
};

/* CPU identification */
enum ryzen_family cpuid_get_family(void);

/* OS access functions */
os_access_t init_os_access_obj(void);
void free_os_access_obj(os_access_t os);
uint32_t os_read_memory(os_access_t os, uint32_t addr);
void os_write_memory(os_access_t os, uint32_t addr, uint32_t value);

/* SMU functions */
smu_t get_smu(os_access_t os, smu_type_t type);
int smu_service_req(smu_t smu, uint32_t id, smu_service_args_t *args);

/* SMU register addresses for Strix Point */
#define STRIXPOINT_MP1_C2PMSG_MESSAGE     0x3B10530
#define STRIXPOINT_MP1_C2PMSG_RESPONSE    0x3B10570
#define STRIXPOINT_MP1_C2PMSG_ARGUMENT_0  0x3B10A40

#define STRIXPOINT_PSMU_C2PMSG_MESSAGE    0x3B10A20
#define STRIXPOINT_PSMU_C2PMSG_RESPONSE   0x3B10A80
#define STRIXPOINT_PSMU_C2PMSG_ARGUMENT_0 0x3B10A88

/* NB SMU base address */
#define NB_SMU_BASE_ADDR 0x00000000

#endif /* RYZENADJ_PRIV_H */