# Hook Matrix — Phase 4

Per-app runtime initialization runs only in matching app processes. A global profile applies to apps and `system_server` when `scope` is absent (legacy compatibility) or `scope=global`; `scope=per_app` disables global application. This does not mean every API is spoofed: entries marked as presence checks or probes are not direct hooks.

| Area | API / property | Mechanism | Status | Fallback |
|---|---|---|---|---|
| Build | `android.os.Build` static fields | JNI field update per specialized process | attempted in selected app process; system_server only in explicit global scope | original field if field update fails |
| SystemProperties | `native_get(String)` | Zygisk JNI native hook | active/probed | original |
| SystemProperties | `native_get(String,String)` | Zygisk JNI native hook | active/probed | original |
| SystemProperties | `native_get_int/long/boolean` | Zygisk JNI native hook | active/probed | original |
| Samsung | `android.os.SemSystemProperties` native variants | Zygisk JNI native hook | probed | original |
| Serial | `ro.serialno` | property layer | active | original |
| Serial | `ril.serialnumber` | property layer | active | original |
| Serial | `sys.serialnumber` | property layer | active | original |
| Build serial | `Build.getSerial()` | runtime presence check | detected, not ART-patched | property layer |
| Country | `gsm.operator.iso-country` | property layer | active | original |
| Country | `gsm.sim.operator.iso-country` | property layer | active | original |
| Country | `persist.sys.country` | property layer | active | original |
| Country | `ro.product.locale.region` | property layer | active | original |
| Telephony | `getNetworkCountryIso()` | runtime presence check | detected | property layer |
| Telephony | `getNetworkCountryIso(int)` | runtime presence check | detected | property layer |
| Telephony | `getSimCountryIso()` | runtime presence check | detected | property layer |


## SimSpoofer integration status

| SimSpoofer field group | Native bridge status | Notes |
|---|---|---|
| Device model / brand / product / board / hardware / bootloader | Profile export supported | Applied only when the device-model feature is enabled and values are present |
| Hardware serial | Profile export supported | Property-layer mapping only; Java `Build.getSerial()` is not ART-patched |
| Country ISO | Profile export supported | Property-layer mapping; Samsung CSC/Wi-Fi and Telephony Java APIs can still report a different country |
| SIM/operator Java APIs | Not ported to Native | Existing LSPosed hooks remain available |
| Android ID, GSF ID, App Set ID, Widevine, location, User-Agent, procfs | Not ported to Native | Requires dedicated per-API native/runtime implementations; not implied by the mode switch |
| KPM | Lifecycle foundation only | No kernel hooks are implemented in `kpm/main.c` |
| SIM/network operator numeric + name | Property-layer export supported | Maps `gsm.operator.*` and `gsm.sim.operator.*` properties; does not replace every `TelephonyManager` API |
| Time zone | Property-layer export supported | Maps `persist.sys.timezone`; Java/system time-zone services may cache their own value |
