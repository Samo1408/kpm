# Universal Samsung Spoof — shared SimSpoofer configuration

This source contains the Zygisk property spoofing engine and a KernelPatch KPM lifecycle/control foundation. It does not modify `framework.jar` or DEX files.

## Canonical configuration

The single shared settings file is `/data/adb/simspoof.prop`. It is the authoritative file for the SimSpoofer-to-Zygisk profile bridge. The format is documented in [`docs/SHARED_CONFIG_PATH.md`](docs/SHARED_CONFIG_PATH.md).

- SimSpoofer imports the selected package's `app.<package>.*` block when opening the editor and writes the profile preferences back when saved.
- `active=true` is the editor's Active switch; `allowed=true` means the app is selected and enabled in SimSpoofer's profile list; `hook_mode=native` selects Zygisk for that app. Zygisk requires all three.
- `hook_mode=lsposed`, inactive profiles, disabled profile-list entries, missing records, and malformed records do not fall back to global spoofing.
- Zygisk reads the shared file directly. The older `/data/adb/universal-samsung-spoof/profiles/<package>.prop` files are compatibility caches, not the authoritative source when the shared file exists.
- The `post-fs-data.sh` migration imports `/system/spoof.prop` once if the new file does not exist. It filters legacy scope metadata and sets `scope=per_app`, preventing an old global profile from silently applying to all apps. The original old file is not deleted.
- Root-level legacy values remain available for SimSpoofer import. Explicit `scope=global` is the only way to request a global Zygisk profile; SimSpoofer-managed mode writes `scope=per_app`.

## Native hook coverage

The current Native bridge maps supported Build/property fields, hardware serial, country ISO, operator values, and timezone where the corresponding fields are present. It does not yet port every Java-level SIM, location, DRM, Android ID, App Set ID, or other SimSpoofer hook to Native. `SemSystemProperties` availability varies by Samsung/Android version and is logged.

## KPM limitation

`kpm/main.c` remains a lifecycle/control stub and has no active kernel spoof hooks or kernel-side reader for `/data/adb/simspoof.prop`. The shared path is declared as the agreed configuration ABI, but true KPM consumption requires a supported userspace-to-KPM profile relay and actual KPM hook implementation. Do not interpret the shared-file bridge or Native selector as proof that KPM spoof hooks are active.


## Integration audit status (2026-10-04)

- The SimSpoofer `hookMode` is now checked by the LSPosed/libxposed entry point. A profile set to `native` returns before any LSPosed hook installer runs; the native path remains responsible for that profile.
- Zygisk `SystemProperties` and Samsung `SemSystemProperties` integer, long, and boolean getters now parse mapped profile values when valid and fall back to the original implementation for missing or invalid values.
- KPM remains **not an active identity-spoofing engine** in this source snapshot. Its current `main.c` contains lifecycle callbacks only. The shared file `/data/adb/simspoof.prop` is a userspace file and cannot be read directly by KPM as a normal file. The next kernel implementation needs a real userspace-to-KPM control/config relay and must be built against the KernelPatch headers/API version used by the target device. No global kernel hook is enabled as a substitute for per-app targeting.
- Native coverage is still narrower than LSPosed: Android ID, GSF ID, App Set ID, SIM serial APIs, DRM/Widevine, location, mock-location indicators, procfs, and User-Agent do not become native merely because their preferences are serialized. See `docs/HOOK_MATRIX.md`.

## KPatch Next build target

The KPM build is wired to the bundled `third_party/KPatch-Next-EXP` source tree (version recorded in its `version` file), targeting AArch64. The workflow no longer clones upstream KernelPatch or assumes its older `kernel/arch/arm64` header layout. To override the bundled source for a local build, set `KP_DIR` to another compatible KPatch Next source root.

The current KPM implementation is intentionally a lifecycle/control foundation only. A successful `.kpm` compilation validates the KPM packaging/build interface; it does **not** mean kernel identity hooks are active or that per-app scoping is implemented in kernel space.


## Independent per-app UI

A separate Android companion UI now lives in [`ui-android/`](ui-android/). It lists installed apps, edits per-app Native/LSPosed mode and the fields consumed by the current native property/Build layer, and writes the canonical `/data/adb/simspoof.prop` file through `su`. It does not require an app to be added to LSPosed scope for Native mode. Grant root access to the companion app, save a profile, then force-stop and reopen the target app.

Build the UI separately with `cd ui-android && ./gradlew assembleDebug`. The CI workflow also builds and publishes `UniversalSpoofUI-debug.apk` with the source/module artifacts. This UI is a companion APK, not a manager-specific in-app WebUI; it avoids relying on undocumented KPatch Next WebUI APIs.

The UI deliberately does not claim support for hooks that are not implemented in the current Zygisk runtime. Android ID, App Set ID, location, Wi-Fi, procfs, User-Agent and all TelephonyManager Java APIs remain outside the Native hook implementation; see `docs/HOOK_MATRIX.md`.
