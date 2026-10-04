# Alpha 2 Hook Matrix

| Target | Mechanism | Status |
|---|---|---|
| `android.os.SystemProperties.native_get` | Zygisk JNI native hook | Implemented |
| `native_get(String,String)` | Zygisk JNI native hook | Implemented |
| `native_get_int` | Hook/fallback wrapper | Implemented |
| `native_get_long` | Hook/fallback wrapper | Implemented |
| `native_get_boolean` | Hook/fallback wrapper | Implemented |
| `android.os.Build.MODEL` | Static field replacement | Implemented |
| `Build.BRAND/DEVICE/PRODUCT` | Static field replacement | Implemented |
| `Build.MANUFACTURER/BOARD/HARDWARE` | Static field replacement | Implemented |
| `Build.BOOTLOADER` | Static field replacement | Implemented |
| `ro.product.locale` | SystemProperties | Implemented |
| `ro.serialno` | SystemProperties | Implemented |
| `ril.serialnumber` | SystemProperties | Implemented |
| `gsm.*.iso-country` | SystemProperties | Implemented |
| Telephony Java methods | Zygisk | Next stage |
| Samsung `SemSystemProperties` Java API | Zygisk | Next stage |
| Kernel hardware identifiers | KPM | Intentionally disabled pending target symbol |
