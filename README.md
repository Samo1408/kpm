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
