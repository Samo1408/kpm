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
