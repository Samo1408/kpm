# Universal Samsung Spoof — Alpha 2

Architecture:
- Zygisk native module for `system_server`
- Safe KPM skeleton (no kernel hook enabled by default)
- Shared `profile.prop`
- No `framework.jar` / DEX modification

Alpha 2 implements:
- Native `android.os.SystemProperties` string/int/long/boolean interception in system_server.
- Runtime spoofing for `ro.product.*`, locale/country, serial and selected Samsung serial properties.
- Runtime replacement of selected `android.os.Build` static string fields in system_server.
- Independent hook status logging.
- Conservative fallback to the original value for unsupported/missing profile keys.

The KPM is intentionally lifecycle-only in this release. Kernel hooks are kernel-dependent and must not be enabled blindly.

Profile location:
`/data/adb/universal-samsung-spoof/profile.prop`

Example:
```properties
model=SM-S928B
brand=samsung
device=e3q
product=e3qxxx
manufacturer=samsung
name=Galaxy S25 Ultra
board=s5e9945
hardware=qcom
bootloader=S928BXXU
locale=en-US
country=United States
country_iso=US
serial=UNIVERSAL-SPOOF
```


## GitHub Actions toolchain fix

The workflow does not use Ubuntu packages for `aarch64-none-elf`.
It downloads Arm GNU Toolchain 14.3.rel1 for the GitHub runner and places
`aarch64-none-elf-*` on PATH before building the KPM.


### GitHub Actions SDK fix

The workflow now uses the Android SDK already provisioned on GitHub-hosted Ubuntu runners.
It calls `sdkmanager` directly and installs only NDK 27.2.12479018; it no longer uses
`android-actions/setup-android`, which was attempting to install the obsolete `tools` package.

## Phase 4

Phase 4 adds the Samsung/framework-facing layer without modifying `framework.jar`:

- `android/os/SemSystemProperties`: probes native `get`, `getInt`, `getLong`, `getBoolean`, and `native_get` variants.
- Serial coverage: `ro.serialno`, `ril.serialnumber`, and `sys.serialnumber` are spoofed through the SystemProperties/SemSystemProperties layers; `Build.getSerial()` is detected at runtime but is not patched through unstable ART `ArtMethod` entry-point rewriting.
- Country coverage: `gsm.operator.iso-country`, `gsm.sim.operator.iso-country`, `persist.sys.country`, and `ro.product.locale.region` are covered by the property layer.
- `TelephonyManager.getNetworkCountryIso()`, `getNetworkCountryIso(int)`, and `getSimCountryIso()` are probed and logged when present.

The module remains boot-safe: an unavailable native method is skipped and the original implementation/value is preserved.
