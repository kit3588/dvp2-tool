/**
 * CameraInfo — dump all resolution modes, QuickROI presets, ROI descriptor,
 *              source formats, and current settings for the first DVP camera.
 *
 * Requires -Wl,--no-as-needed -lstdc++ (see Makefile).
 */
#include <stdio.h>
#include <stdint.h>
#include "DVPCamera.h"

int main(void)
{
    dvpUint32 count = 0;
    dvpRefresh(&count);
    if (count == 0) { fprintf(stderr, "No DVP camera found.\n"); return 1; }

    dvpCameraInfo info[8];
    if (count > 8) count = 8;
    for (dvpUint32 i = 0; i < count; i++)
        if (dvpEnum(i, &info[i]) == DVP_STATUS_OK)
            printf("[%u] %s  SN:%s\n", i, info[i].FriendlyName, info[i].OriginalSerialNumber);

    dvpHandle h;
    if (dvpOpenByName(info[0].FriendlyName, OPEN_NORMAL, &h) != DVP_STATUS_OK) {
        fprintf(stderr, "dvpOpenByName failed\n"); return 1;
    }

    // ── Resolution modes ─────────────────────────────────────────────────────
    dvpSelectionDescr resDescr;
    if (dvpGetResolutionModeSelDescr(h, &resDescr) == DVP_STATUS_OK) {
        printf("\nResolution modes (%u):\n", resDescr.uCount);
        for (dvpUint32 i = 0; i < resDescr.uCount; i++) {
            dvpResolutionMode rm;
            if (dvpGetResolutionModeSelDetail(h, i, &rm) == DVP_STATUS_OK)
                printf("  [%u] \"%s\"  max=%dx%d\n",
                       i, rm.selection.string, rm.region.iMaxW, rm.region.iMaxH);
        }
    }

    // ── Source formats ────────────────────────────────────────────────────────
    dvpSelectionDescr fmtDescr;
    if (dvpGetSourceFormatSelDescr(h, &fmtDescr) == DVP_STATUS_OK) {
        printf("\nSource formats (%u):\n", fmtDescr.uCount);
        for (dvpUint32 i = 0; i < fmtDescr.uCount; i++) {
            dvpFormatSelection fs;
            if (dvpGetSourceFormatSelDetail(h, i, &fs) == DVP_STATUS_OK)
                printf("  [%u] \"%s\"  format=%d\n", i, fs.selection.string, (int)fs.format);
        }
    }

    // ── ROI descriptor (step = alignment requirement) ─────────────────────────
    dvpRegionDescr roiDescr;
    if (dvpGetRoiDescr(h, &roiDescr) == DVP_STATUS_OK) {
        printf("\nROI descriptor:\n");
        printf("  max       : %d x %d\n", roiDescr.iMaxW, roiDescr.iMaxH);
        printf("  min       : %d x %d\n", roiDescr.iMinW, roiDescr.iMinH);
        printf("  step      : w=%d  h=%d  (alignment requirement)\n",
               roiDescr.iStepW, roiDescr.iStepH);
        printf("  16-aligned: w_step%%16=%d  h_step%%16=%d\n",
               roiDescr.iStepW % 16, roiDescr.iStepH % 16);
    }

    // ── QuickROI presets ─────────────────────────────────────────────────────
    dvpSelectionDescr qrDescr;
    if (dvpGetQuickRoiSelDescr(h, &qrDescr) == DVP_STATUS_OK) {
        printf("\nQuickROI presets (%u):\n", qrDescr.uCount);
        for (dvpUint32 i = 0; i < qrDescr.uCount; i++) {
            dvpQuickRoi qr;
            if (dvpGetQuickRoiSelDetail(h, i, &qr) == DVP_STATUS_OK)
                printf("  [%u] \"%s\"  x=%d y=%d w=%d h=%d  w%%16=%d\n",
                       i, qr.selection.string,
                       qr.roi.X, qr.roi.Y, qr.roi.W, qr.roi.H,
                       qr.roi.W % 16);
        }
    }

    // ── Current ROI ──────────────────────────────────────────────────────────
    dvpRegion region;
    bool roi_state = false;
    dvpGetRoiState(h, &roi_state);
    if (dvpGetRoi(h, &region) == DVP_STATUS_OK) {
        printf("\nCurrent ROI (enabled=%s):\n", roi_state ? "yes" : "no");
        printf("  x=%d y=%d w=%d h=%d  w%%16=%d\n",
               region.X, region.Y, region.W, region.H, region.W % 16);
    }

    // ── Exposure / gain ──────────────────────────────────────────────────────
    double exp = 0; float gain = 0;
    dvpGetExposure(h, &exp);
    dvpGetAnalogGain(h, &gain);
    printf("\nExposure: %.2f us  Gain: %.3f\n", exp, gain);

    dvpClose(h);
    return 0;
}
