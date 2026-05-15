# dvp2-tool

DVP2 GigE camera demo tools for RK3588 (aarch64). Assumes the DVP2 vendor
libraries are installed system-wide under `/usr/lib`.

## Prerequisites

- `libdvp.so`, `libhzd.so`, `libvfvlog.so`, `GigE.dscam.so`, `GigEGen.dscam.so`
  all present in `/usr/lib` (installed by the vendor `install.sh`)
- `dsfilter.ko` kernel module loaded (`insmod dsfilter.ko`) before running any tool
- GigE camera network interface configured, e.g.:
  ```
  ip addr add 192.168.1.100/24 dev end0
  ```

## Build

```bash
make
```

## Tools

### `Demo`
Basic grab demo. Enumerates cameras, lets you pick one by index, opens it in
trigger mode, grabs 20 frames and prints frame info.

### `IPConfigDemo`
Shows camera network config (IP, MAC, gateway). Edit `DEV_SN1`/`DEV_SN2` at
the top of `IPConfigDemo.cpp` to match your camera serial numbers.

### `Dvp2StreamCallback`
Callback-based streaming demo.

### `ResetCamera`
Prints current camera settings (ROI, exposure, gain, sensor max resolution)
and resets the ROI to full sensor size. Saves persistently to `USER_SET_1`
so the setting survives power-cycles.

Useful when the camera has a stale ROI from a previous session that causes
RGA hardware scaler errors (`width stride not 16-aligned`).

```
./ResetCamera
```

## Known Issues

### `dvpOpenByName` segfault on GigE cameras

**Symptom**: crash inside `D3tNodeMapAdapter::GetNode` in `GigEGen.dscam.so`.

**Root cause**: `libdvp.so` lazily `dlopen`s `GigEGen.dscam.so` at open time.
That library uses GenICam C++ types (`gcstring`) that require `libstdc++.so.6`
to be loaded *before* the shared lib runs. Binaries that use only C-style stdio
don't pull `libstdc++` at link time — so when `dlopen` fires, the C++ runtime
is uninitialized → SIGSEGV.

**Fix**: Link any DVP2 binary with:
```
-Wl,--no-as-needed -lstdc++ -Wl,--as-needed
```
This is already applied to `ResetCamera` in the Makefile. `Demo`, `IPConfigDemo`,
and `Dvp2StreamCallback` use `<iostream>`/`<string>` so they pull `libstdc++`
naturally.

**Note**: Camera open (`dvpOpenByName`) must also be called from a `pthread`
worker, not from the `main` thread — required by `GigEGen.dscam.so`.
