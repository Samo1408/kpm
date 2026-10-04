# Universal Spoof UI (companion APK)

This is an independent Android control UI for `Universal-Samsung-Spoof`. It does not use LSPosed scope to select Native targets. The app list comes from Android's installed packages; each saved record is written to `/data/adb/simspoof.prop` as `app.<package>.<key>=<value>`.

## Build

```bash
./gradlew assembleDebug
```

Output: `app/build/outputs/apk/debug/app-debug.apk`.

The UI uses `su` to read/write the root-owned shared configuration, so a root-capable manager must grant the app root access. If root access is denied, saving fails visibly rather than claiming success.

## Isolation rules

- Each profile is per-package; the file metadata is written as `scope=per_app`.
- Native mode requires `hook_mode=native`, `active=true`, and `allowed=true` for the package.
- LSPosed mode writes an explicit `hook_mode=lsposed` record, so the Zygisk module skips that app.
- The UI preserves records for other packages and preserves unknown keys / serialized preferences for the selected package.
- Saving does not activate unsupported hooks. Current Native implementation consumes the property mappings and selected `android.os.Build` static fields implemented in `zygisk/jni/zygisk_module.cpp`. Android ID, App Set ID, location, Wi-Fi, procfs, User-Agent, and all TelephonyManager methods are not implemented by this Native module yet.

After saving, force-stop and reopen the selected app. This is source-stage delivery; validate the APK and module on the target device before relying on runtime behavior.
