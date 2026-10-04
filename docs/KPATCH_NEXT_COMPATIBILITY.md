# KPatch Next compatibility and build status

## Target supplied for this project

- Architecture: AArch64 only
- Android: 14 / SDK 34
- Linux kernel: 4.14.357
- KPM manager: KPatch Next

## Source API inspected

The bundled `third_party/KPatch-Next-EXP` source reports version `0.13.14`. Its KPM headers provide `KPM_NAME`, `KPM_VERSION`, `KPM_INIT`, `KPM_CTL0`, and `KPM_EXIT`, and its demo builds relocatable `.kpm` output with an AArch64 bare-metal toolchain. The separate `KernelPatch-main.zip` source reports `0.13.9`; the KPM lifecycle declarations are substantially the same, but the Linux header directory layout differs. The project now builds against the KPatch Next tree supplied by the user instead of cloning the other source.

KPatch Next's README lists AArch64 and Linux 3.18–6.12 as theoretical support and requires `CONFIG_KALLSYMS=y`. That is not a guarantee that this specific device's kernel image or KPM manager accepts a module built from these sources. Runtime compatibility must still be verified on the target device.

## Verification boundary

The KPM C source was compiled locally as an AArch64 relocatable object using Clang and the bundled KPatch Next headers. The environment did not include `aarch64-none-elf-gcc`, so the exact GitHub Actions toolchain build and device-side load were not executed in this environment.

The current `kpm/main.c` is a lifecycle/control stub. It does not install identity hooks, read `/data/adb/simspoof.prop` from kernel space, or implement per-app kernel scoping. The shared config file is a userspace file; a validated control relay and a safe, explicitly scoped design would be required before claiming those capabilities.
