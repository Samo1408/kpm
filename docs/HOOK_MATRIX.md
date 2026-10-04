# Hook Matrix — Phase 4

| Area | API / property | Mechanism | Status | Fallback |
|---|---|---|---|---|
| Build | `android.os.Build` static fields | JNI field update | active | original field |
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
