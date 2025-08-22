/* SPDX-License-Identifier: LGPL */
/* SMU operations for standalone RyzenAdj */

#include "ryzenadj_priv.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#ifdef _WIN32
#include <windows.h>
#include <setupapi.h>
#include <devguid.h>
#include <regstr.h>
#else
#include <unistd.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <errno.h>
#endif

/* CPU ID detection */
#if defined(__GNUC__)
#include <cpuid.h>
#elif defined (_WIN32)
#include <intrin.h>
#endif

#define CPUID_VENDOR_AMD "AuthenticAMD"

static void getcpuid(unsigned int CPUInfo[4], unsigned int InfoType) {
#if defined(__GNUC__)
    __cpuid(InfoType, CPUInfo[0], CPUInfo[1], CPUInfo[2], CPUInfo[3]);
#elif defined(_WIN32)
    __cpuid((int*)(void*)CPUInfo, (int)InfoType);
#endif
}

enum ryzen_family cpuid_get_family(void) {
    uint32_t regs[4];
    int family, model;
    char vendor[16] = {0};
    
    getcpuid(regs, 0);
    
    /* Extract vendor string */
    *(uint32_t*)&vendor[0] = regs[1];  /* EBX */
    *(uint32_t*)&vendor[4] = regs[3];  /* EDX */
    *(uint32_t*)&vendor[8] = regs[2];  /* ECX */
    
    if (strncmp(vendor, CPUID_VENDOR_AMD, strlen(CPUID_VENDOR_AMD))) {
        printf("Not an AMD processor\n");
        return FAM_UNKNOWN;
    }
    
    getcpuid(regs, 1);
    
    family = ((regs[0] >> 8) & 0xf) + ((regs[0] >> 20) & 0xff);
    model = ((regs[0] >> 4) & 0xf) | ((regs[0] >> 12) & 0xf0);
    
    if (family == 0x1A) {  /* Zen5 family */
        switch (model) {
            case 32:  /* Strix Point APU */
            case 36:  /* Strix Point APU variant */
                return FAM_STRIXPOINT;
            case 112: /* Strix Halo APU */
                return FAM_STRIXHALO;
            default:
                printf("Unsupported Zen5 model: %d\n", model);
                break;
        }
    } else {
        printf("Unsupported CPU family: 0x%x\n", family);
    }
    
    return FAM_UNKNOWN;
}

#ifdef _WIN32
/* Windows implementation using WinRing0 or direct access */
os_access_t init_os_access_obj(void) {
    os_access_t os = calloc(1, sizeof(struct _os_access));
    if (!os) return NULL;
    
    /* Try to open a device handle for memory access */
    /* This would require WinRing0 driver or similar low-level access */
    os->driver_handle = INVALID_HANDLE_VALUE;
    
    printf("Windows memory access not fully implemented in standalone version\n");
    printf("Consider using the full RyzenAdj with WinRing0 driver\n");
    
    /* For now, return a dummy handle to allow compilation */
    return os;
}

void free_os_access_obj(os_access_t os) {
    if (!os) return;
    
    if (os->driver_handle != INVALID_HANDLE_VALUE) {
        CloseHandle(os->driver_handle);
    }
    
    free(os);
}

uint32_t os_read_memory(os_access_t os, uint32_t addr) {
    if (!os || os->driver_handle == INVALID_HANDLE_VALUE) {
        return 0;
    }
    
    /* Windows memory access would require WinRing0 or similar driver */
    printf("Memory read not implemented: 0x%08x\n", addr);
    return 0;
}

void os_write_memory(os_access_t os, uint32_t addr, uint32_t value) {
    if (!os || os->driver_handle == INVALID_HANDLE_VALUE) {
        return;
    }
    
    /* Windows memory access would require WinRing0 or similar driver */
    printf("Memory write not implemented: 0x%08x = 0x%08x\n", addr, value);
}

#else
/* Linux implementation using /dev/mem */
os_access_t init_os_access_obj(void) {
    os_access_t os = calloc(1, sizeof(struct _os_access));
    if (!os) return NULL;
    
    os->mem_fd = open("/dev/mem", O_RDWR | O_SYNC);
    if (os->mem_fd < 0) {
        printf("Failed to open /dev/mem: %s\n", strerror(errno));
        printf("Run as root or ensure proper permissions\n");
        free(os);
        return NULL;
    }
    
    /* Map NB SMU region */
    os->nb_base_addr = 0x00000000;  /* NB SMU base */
    os->mapped_size = 0x1000000;    /* 16MB region */
    
    os->mapped_mem = mmap(NULL, os->mapped_size, PROT_READ | PROT_WRITE, 
                         MAP_SHARED, os->mem_fd, os->nb_base_addr);
    
    if (os->mapped_mem == MAP_FAILED) {
        printf("Failed to mmap NB SMU region: %s\n", strerror(errno));
        close(os->mem_fd);
        free(os);
        return NULL;
    }
    
    return os;
}

void free_os_access_obj(os_access_t os) {
    if (!os) return;
    
    if (os->mapped_mem != MAP_FAILED) {
        munmap(os->mapped_mem, os->mapped_size);
    }
    
    if (os->mem_fd >= 0) {
        close(os->mem_fd);
    }
    
    free(os);
}

uint32_t os_read_memory(os_access_t os, uint32_t addr) {
    if (!os || os->mapped_mem == MAP_FAILED) {
        return 0;
    }
    
    if (addr >= os->mapped_size) {
        printf("Address out of range: 0x%08x\n", addr);
        return 0;
    }
    
    return *(volatile uint32_t*)((char*)os->mapped_mem + addr);
}

void os_write_memory(os_access_t os, uint32_t addr, uint32_t value) {
    if (!os || os->mapped_mem == MAP_FAILED) {
        return;
    }
    
    if (addr >= os->mapped_size) {
        printf("Address out of range: 0x%08x\n", addr);
        return;
    }
    
    *(volatile uint32_t*)((char*)os->mapped_mem + addr) = value;
}
#endif

/* SMU operations */
smu_t get_smu(os_access_t os, smu_type_t type) {
    if (!os) return NULL;
    
    smu_t smu = calloc(1, sizeof(struct _smu));
    if (!smu) return NULL;
    
    smu->os = os;
    
    /* Set register addresses based on SMU type and CPU family */
    /* These addresses are specific to Strix Point */
    switch (type) {
        case TYPE_MP1:
            smu->req = STRIXPOINT_MP1_C2PMSG_MESSAGE;
            smu->rsp = STRIXPOINT_MP1_C2PMSG_RESPONSE;
            smu->arg_base = STRIXPOINT_MP1_C2PMSG_ARGUMENT_0;
            break;
        case TYPE_PSMU:
            smu->req = STRIXPOINT_PSMU_C2PMSG_MESSAGE;
            smu->rsp = STRIXPOINT_PSMU_C2PMSG_RESPONSE;
            smu->arg_base = STRIXPOINT_PSMU_C2PMSG_ARGUMENT_0;
            break;
        default:
            free(smu);
            return NULL;
    }
    
    return smu;
}

int smu_service_req(smu_t smu, uint32_t id, smu_service_args_t *args) {
    if (!smu || !args) return REP_MSG_Failed;
    
    int timeout = 1000;  /* 1 second timeout */
    uint32_t response;
    
    /* Clear response register */
    os_write_memory(smu->os, smu->rsp, 0);
    
    /* Write arguments */
    os_write_memory(smu->os, smu->arg_base + 0, args->arg0);
    os_write_memory(smu->os, smu->arg_base + 4, args->arg1);
    os_write_memory(smu->os, smu->arg_base + 8, args->arg2);
    os_write_memory(smu->os, smu->arg_base + 12, args->arg3);
    os_write_memory(smu->os, smu->arg_base + 16, args->arg4);
    os_write_memory(smu->os, smu->arg_base + 20, args->arg5);
    
    /* Send command */
    os_write_memory(smu->os, smu->req, id);
    
    /* Wait for response */
    do {
        usleep(1000);  /* 1ms delay */
        response = os_read_memory(smu->os, smu->rsp);
        timeout--;
    } while (response == 0 && timeout > 0);
    
    if (timeout <= 0) {
        printf("SMU timeout on command 0x%x\n", id);
        return REP_MSG_Failed;
    }
    
    /* Read response arguments */
    args->arg0 = os_read_memory(smu->os, smu->arg_base + 0);
    args->arg1 = os_read_memory(smu->os, smu->arg_base + 4);
    args->arg2 = os_read_memory(smu->os, smu->arg_base + 8);
    args->arg3 = os_read_memory(smu->os, smu->arg_base + 12);
    args->arg4 = os_read_memory(smu->os, smu->arg_base + 16);
    args->arg5 = os_read_memory(smu->os, smu->arg_base + 20);
    
    return response;
}