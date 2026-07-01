#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <thread>
#include <chrono>
#include "DVPCamera.h"

#define ENUM_DEV_MAX 32

typedef struct DsGlanIpConfigText_s {
    char sn[64];
    char DeviceMac[18];
    char DeviceIP[16];
    char DeviceNetMask[16];
    char DeviceGateway[16];
    char Mode[16];
    char EthIP[16];
    char EthMac[18];
} DsGlanIpConfigText_t;

static void print_usage(const char *prog) {
    fprintf(stderr, "Usage:\n");
    fprintf(stderr, "  %s list\n", prog);
    fprintf(stderr, "  %s set <SN> --ip <IP> [--mask <MASK>] [--gw <GW>] [--mode PERSISTENT|DHCP]\n", prog);
}

static int cmd_list() {
    printf("Waiting for device enumeration...\n");
    std::this_thread::sleep_for(std::chrono::seconds(3));

    dvpUint32 count = 0;
    dvpCameraInfo info[ENUM_DEV_MAX];

    dvpRefresh(&count);
    if (count > ENUM_DEV_MAX) count = ENUM_DEV_MAX;
    printf("Found %u device(s)\n\n", count);

    for (uint32_t i = 0; i < count; i++) {
        if (DVP_STATUS_OK != dvpEnum(i, &info[i])) continue;

        DsGlanIpConfigText_t ipInfo;
        memset(&ipInfo, 0, sizeof(ipInfo));
        snprintf(ipInfo.sn, sizeof(ipInfo.sn), "%s", info[i].OriginalSerialNumber);

        printf("[%u] %s  SN: %s\n", i, info[i].FriendlyName, info[i].OriginalSerialNumber);

        dvpStatus ret = dvpConfigEx(i, 0x1100, 0, &ipInfo);
        if (ret == DVP_STATUS_OK) {
            printf("    IP:   %s\n", ipInfo.DeviceIP);
            printf("    Mask: %s\n", ipInfo.DeviceNetMask);
            printf("    GW:   %s\n", ipInfo.DeviceGateway);
            printf("    MAC:  %s\n", ipInfo.DeviceMac);
            printf("    Mode: %s\n", ipInfo.Mode);
            printf("    EthIP:%s\n", ipInfo.EthIP);
        } else {
            printf("    dvpConfigEx(GET) failed: %d\n", ret);
        }
        printf("\n");
    }
    return 0;
}

static int cmd_set(int argc, char *argv[]) {
    // argv[0] = "set", argv[1] = SN, then --ip, --mask, --gw, --mode pairs
    if (argc < 2) {
        fprintf(stderr, "Error: 'set' requires <SN> and --ip <IP>\n");
        return 1;
    }

    const char *target_sn = argv[1];
    const char *new_ip   = NULL;
    const char *new_mask = NULL;
    const char *new_gw   = NULL;
    const char *new_mode = NULL;

    for (int i = 2; i < argc - 1; i++) {
        if (strcmp(argv[i], "--ip")   == 0) new_ip   = argv[++i];
        else if (strcmp(argv[i], "--mask") == 0) new_mask = argv[++i];
        else if (strcmp(argv[i], "--gw")   == 0) new_gw   = argv[++i];
        else if (strcmp(argv[i], "--mode") == 0) new_mode = argv[++i];
    }

    if (!new_ip) {
        fprintf(stderr, "Error: --ip is required\n");
        return 1;
    }

    printf("Waiting for device enumeration...\n");
    std::this_thread::sleep_for(std::chrono::seconds(3));

    dvpUint32 count = 0;
    dvpCameraInfo info[ENUM_DEV_MAX];
    dvpRefresh(&count);
    if (count > ENUM_DEV_MAX) count = ENUM_DEV_MAX;

    for (uint32_t i = 0; i < count; i++) {
        if (DVP_STATUS_OK != dvpEnum(i, &info[i])) continue;
        if (strcmp(info[i].OriginalSerialNumber, target_sn) != 0) continue;

        DsGlanIpConfigText_t cfg;
        memset(&cfg, 0, sizeof(cfg));
        snprintf(cfg.sn, sizeof(cfg.sn), "%s", target_sn);

        dvpStatus ret = dvpConfigEx(i, 0x1100, 0, &cfg);
        if (ret != DVP_STATUS_OK) {
            fprintf(stderr, "Error: GET config failed for SN=%s (status=%d)\n", target_sn, ret);
            return 1;
        }

        // Overlay only the specified fields
        snprintf(cfg.DeviceIP, sizeof(cfg.DeviceIP), "%s", new_ip);
        snprintf(cfg.DeviceNetMask, sizeof(cfg.DeviceNetMask), "%s", new_mask ? new_mask : "255.255.255.0");
        if (new_gw)   snprintf(cfg.DeviceGateway, sizeof(cfg.DeviceGateway), "%s", new_gw);
        snprintf(cfg.Mode, sizeof(cfg.Mode), "%s", new_mode ? new_mode : "PERSISTENT");

        ret = dvpConfigEx(i, 0x1000, 0, &cfg);
        if (ret != DVP_STATUS_OK) {
            fprintf(stderr, "Error: SET config failed for SN=%s (status=%d)\n", target_sn, ret);
            return 1;
        }

        printf("Success: IP config updated for SN=%s\n", target_sn);
        printf("  IP:   %s\n", cfg.DeviceIP);
        printf("  Mask: %s\n", cfg.DeviceNetMask);
        printf("  GW:   %s\n", cfg.DeviceGateway);
        printf("  Mode: %s\n", cfg.Mode);
        return 0;
    }

    fprintf(stderr, "Error: device with SN=%s not found\n", target_sn);
    return 1;
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        print_usage(argv[0]);
        return 1;
    }

    if (strcmp(argv[1], "list") == 0) {
        return cmd_list();
    } else if (strcmp(argv[1], "set") == 0) {
        return cmd_set(argc - 1, argv + 1);
    } else {
        fprintf(stderr, "Unknown command: %s\n", argv[1]);
        print_usage(argv[0]);
        return 1;
    }
}
