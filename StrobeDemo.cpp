/**
 * StrobeDemo — software-trigger 10 frames with strobe output enabled.
 *
 * The camera drives LINE_2 high for the duration of each sensor exposure
 * (EXPOSURE_DURATION mode).  Wire LINE_2 to an external strobe light or
 * oscilloscope to observe the signal.
 *
 * Build: already covered by the Makefile target below.
 * Run  : ./StrobeDemo
 */
#include <stdio.h>
#include <stdint.h>
#include <pthread.h>
#include <unistd.h>
#include "DVPCamera.h"

#define GRAB_COUNT 10

static void* run(void *p)
{
    dvpStatus status;
    dvpHandle h;
    const char *name = (const char *)p;

    printf("[StrobeDemo] opening camera: %s\n", name);

    do {
        status = dvpOpenByName(name, OPEN_NORMAL, &h);
        if (status != DVP_STATUS_OK) {
            fprintf(stderr, "[StrobeDemo] dvpOpenByName failed: %d\n", status);
            break;
        }

        // ── Strobe output on LINE_2 ──────────────────────────────────────────
        // Configure LINE_2 as output, sourced from the strobe signal.
        status = dvpSetLineMode(h, LINE_2, LINE_MODE_OUTPUT);
        if (status != DVP_STATUS_OK)
            fprintf(stderr, "[StrobeDemo] WARN: dvpSetLineMode(LINE_2, OUTPUT) failed: %d\n", status);

        status = dvpSetLineSource(h, LINE_2, OUTPUT_SOURCE_STROBE);
        if (status != DVP_STATUS_OK)
            fprintf(stderr, "[StrobeDemo] WARN: dvpSetLineSource(LINE_2, STROBE) failed: %d\n", status);

        // Active-high pulse, driven for the full sensor exposure window.
        status = dvpSetStrobeOutputType(h, STROBE_OUT_HIGH);
        if (status != DVP_STATUS_OK)
            fprintf(stderr, "[StrobeDemo] WARN: dvpSetStrobeOutputType(HIGH) failed: %d\n", status);

        status = dvpSetStrobeDriver(h, EXPOSURE_DURATION);
        if (status != DVP_STATUS_OK)
            fprintf(stderr, "[StrobeDemo] WARN: dvpSetStrobeDriver(EXPOSURE_DURATION) failed: %d\n", status);

        // Optional: zero pre-strobe delay (default is usually 0 already).
        dvpSetStrobeDelay(h, 0.0);

        // Print effective exposure so we know how wide the strobe pulse will be.
        double exp_us = 0;
        dvpGetExposure(h, &exp_us);
        printf("[StrobeDemo] exposure: %.1f us  → strobe pulse width ≈ %.1f us\n", exp_us, exp_us);

        // ── Software trigger mode ────────────────────────────────────────────
        // We fire each frame manually so we can count exactly 10.
        status = dvpSetTriggerState(h, true);
        if (status != DVP_STATUS_OK) {
            fprintf(stderr, "[StrobeDemo] dvpSetTriggerState(true) failed: %d\n", status);
            break;
        }
        dvpSetTriggerSource(h, TRIGGER_SOURCE_SOFTWARE);

        // ── Start streaming ──────────────────────────────────────────────────
        status = dvpStart(h);
        if (status != DVP_STATUS_OK) {
            fprintf(stderr, "[StrobeDemo] dvpStart failed: %d\n", status);
            break;
        }

        printf("[StrobeDemo] firing %d triggers...\n", GRAB_COUNT);

        for (int i = 0; i < GRAB_COUNT; i++) {
            // Fire one trigger — camera exposes, LINE_2 goes high during exposure.
            status = dvpTriggerFire(h);
            if (status != DVP_STATUS_OK) {
                fprintf(stderr, "[StrobeDemo] dvpTriggerFire failed on shot %d: %d\n", i+1, status);
                continue;
            }

            dvpFrame frame;
            void *pbuf = NULL;
            status = dvpGetFrame(h, &frame, &pbuf, 3000);
            if (status != DVP_STATUS_OK) {
                fprintf(stderr, "[StrobeDemo] dvpGetFrame timeout/error on shot %d: %d\n", i+1, status);
                continue;
            }

            printf("[StrobeDemo] shot %2d/%d  frame_id=%lu  %dx%d  exp=%.1f us\n",
                   i+1, GRAB_COUNT,
                   (unsigned long)frame.uFrameID,
                   frame.iWidth, frame.iHeight,
                   frame.fExposure);

            if (i == GRAB_COUNT - 1) {
                const char *path = "strobe_last.jpg";
                dvpStatus sv = dvpSavePicture(&frame, pbuf, path, 90);
                if (sv == DVP_STATUS_OK)
                    printf("[StrobeDemo] saved last frame to %s\n", path);
                else
                    fprintf(stderr, "[StrobeDemo] dvpSavePicture failed: %d\n", sv);
            }

            // Small gap between triggers so the strobe pulses are clearly
            // separated on an oscilloscope (100 ms).
            usleep(100000);
        }

        dvpStop(h);

    } while (0);

    // Disable strobe output before closing.
    dvpSetStrobeOutputType(h, STROBE_OUT_OFF);
    dvpClose(h);

    printf("[StrobeDemo] done, status=%d\n", status);
    return NULL;
}

int main(void)
{
    dvpUint32 count = 0;
    dvpRefresh(&count);
    if (count == 0) { fprintf(stderr, "No DVP camera found.\n"); return 1; }

    dvpCameraInfo info[8];
    if (count > 8) count = 8;
    for (dvpUint32 i = 0; i < count; i++)
        if (dvpEnum(i, &info[i]) == DVP_STATUS_OK)
            printf("[%u] %s\n", i, info[i].FriendlyName);

    // dvpOpenByName must be called from a pthread (not main thread).
    pthread_t tid;
    pthread_create(&tid, NULL, run, (void *)info[0].FriendlyName);
    pthread_join(tid, NULL);

    return 0;
}
