# kernel modules

## dsfilter.ko

DVP2 GigE camera network filter driver by do3think.

| Field       | Value                          |
|-------------|--------------------------------|
| Author      | do3think                       |
| License     | GPL                            |
| Arch        | aarch64                        |
| vermagic    | 6.1.99 SMP mod_unload aarch64  |

Must be loaded before `dvpRefresh()` or GigE cameras will not be discovered.

### Load

```bash
insmod kernel/dsfilter.ko
```

### Verify

```bash
lsmod | grep dsfilter
```

### Unload

```bash
rmmod dsfilter
```

> **Note**: This `.ko` is tied to kernel `6.1.99`. It will refuse to load on a
> different kernel version. Recompile from source against the target kernel headers
> if you upgrade the kernel.
