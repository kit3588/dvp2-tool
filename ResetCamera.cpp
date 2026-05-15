/**
 * ResetCamera — print camera settings and reset ROI to full frame.
 *
 * Pattern matches Demo.cpp: open the camera from a worker pthread, not main.
 * The GenICam node-map init in GigEGen.dscam.so segfaults if Open is called
 * from the main thread (observed on aarch64).
 */
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <pthread.h>
#include "DVPCamera.h"

static void print_settings(dvpHandle h, const char *label)
{
    dvpRegion region;
    dvpRegionDescr descr;
    double exp = 0;
    float gain = 0;
    bool roi_state = false;

    printf("\n=== %s ===\n", label);

    if (dvpGetRoiState(h, &roi_state) == DVP_STATUS_OK)
        printf("  ROI enabled : %s\n", roi_state ? "yes" : "no");

    if (dvpGetRoi(h, &region) == DVP_STATUS_OK)
        printf("  ROI         : x=%d y=%d w=%d h=%d\n",
               region.X, region.Y, region.W, region.H);

    if (dvpGetRoiDescr(h, &descr) == DVP_STATUS_OK)
        printf("  ROI max     : w=%d h=%d  step_w=%d step_h=%d\n",
               descr.iMaxW, descr.iMaxH, descr.iStepW, descr.iStepH);

    if (dvpGetExposure(h, &exp) == DVP_STATUS_OK)
        printf("  Exposure    : %.2f us\n", exp);

    if (dvpGetAnalogGain(h, &gain) == DVP_STATUS_OK)
        printf("  Analog gain : %.3f\n", gain);

    printf("\n");
}

static void *worker(void *p)
{
    const char *name = (const char *)p;
    dvpStatus s;
    dvpHandle h;

    printf("worker: opening %s...\n", name);
    s = dvpOpenByName(name, OPEN_NORMAL, &h);
    if (s != DVP_STATUS_OK) {
        fprintf(stderr, "worker: dvpOpenByName failed: %d\n", s);
        return nullptr;
    }
    printf("worker: open OK\n");

    print_settings(h, "Current settings");

    // Reset to factory defaults (clears persistent ROI and other params)
    printf("Calling dvpLoadDefault()...\n");
    s = dvpLoadDefault(h);
    printf("  dvpLoadDefault: %d\n", s);

    // Explicitly disable ROI and set it to full sensor area
    dvpRegionDescr descr;
    if (dvpGetRoiDescr(h, &descr) == DVP_STATUS_OK) {
        dvpRegion full = { 0, 0, descr.iMaxW, descr.iMaxH };
        s = dvpSetRoiState(h, false);
        printf("  dvpSetRoiState(false): %d\n", s);
        s = dvpSetRoi(h, full);
        printf("  dvpSetRoi(0,0,%d,%d): %d\n",
               full.W, full.H, s);
    }

    print_settings(h, "Settings after reset");

    // Persist: save current settings to UserSet1 and make it the default on next open.
    printf("Persisting (UserSet1)...\n");
    s = dvpSaveUserSet(h, USER_SET_1);
    printf("  dvpSaveUserSet(USER_SET_1): %d\n", s);
    s = dvpSetUserSet(h, USER_SET_1);
    printf("  dvpSetUserSet(USER_SET_1) [default-on-boot]: %d\n", s);

    dvpClose(h);
    printf("worker: closed.\n");
    return nullptr;
}

int main(void)
{
    dvpUint32 count = 0;

    printf("start...\n");
    dvpRefresh(&count);
    printf("dvpRefresh count = %u\n", count);
    if (count == 0) {
        fprintf(stderr, "No DVP camera found.\n");
        return 1;
    }

    dvpCameraInfo info[8];
    if (count > 8) count = 8;
    for (dvpUint32 i = 0; i < count; i++) {
        if (dvpEnum(i, &info[i]) == DVP_STATUS_OK) {
            printf("  [%u] %s  SN:%s\n", i,
                   info[i].FriendlyName, info[i].OriginalSerialNumber);
        }
    }

    pthread_t tid;
    pthread_create(&tid, NULL, worker, (void *)info[0].FriendlyName);
    pthread_join(tid, NULL);

    printf("Done.\n");
    return 0;
}
