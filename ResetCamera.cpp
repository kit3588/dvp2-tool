/**
 * ResetCamera — print camera settings and reset ROI to full frame.
 *
 * Requires linking with -Wl,--no-as-needed -lstdc++ so that libstdc++.so.6
 * is loaded before libdvp.so dlopen's the camera driver plugin (.dscam.so).
 * Without it, dvpOpenByName segfaults on both GigE and USB cameras.
 * See Makefile ResetCamera target.
 */
#include <stdio.h>
#include <stdint.h>
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

int main(void)
{
    dvpUint32 count = 0;
    dvpStatus s;
    dvpHandle h;

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
        if (dvpEnum(i, &info[i]) == DVP_STATUS_OK)
            printf("  [%u] %s  SN:%s\n", i,
                   info[i].FriendlyName, info[i].OriginalSerialNumber);
    }

    printf("opening %s...\n", info[0].FriendlyName);
    s = dvpOpenByName(info[0].FriendlyName, OPEN_NORMAL, &h);
    if (s != DVP_STATUS_OK) {
        fprintf(stderr, "dvpOpenByName failed: %d\n", s);
        return 1;
    }
    printf("open OK\n");

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
        printf("  dvpSetRoi(0,0,%d,%d): %d\n", full.W, full.H, s);
    }

    print_settings(h, "Settings after reset");

    // Persist: save to UserSet1 and make it the default on next open
    printf("Persisting (UserSet1)...\n");
    s = dvpSaveUserSet(h, USER_SET_1);
    printf("  dvpSaveUserSet(USER_SET_1): %d\n", s);
    s = dvpSetUserSet(h, USER_SET_1);
    printf("  dvpSetUserSet(USER_SET_1) [default-on-boot]: %d\n", s);

    dvpClose(h);
    printf("Done.\n");
    return 0;
}
