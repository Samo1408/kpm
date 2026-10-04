# Shared SimSpoofer configuration contract

Canonical path: `/data/adb/simspoof.prop`

## Format

The file is a UTF-8 line-oriented `key=value` file. Root-level metadata:

- `format=simspoof-v1`
- `managed=1`
- `scope=per_app` — disables global Zygisk fallback
- `updated_at=<epoch milliseconds>`

Each selected app has its own namespaced block, for example:

```properties
app.com.example.target.active=true
app.com.example.target.allowed=true
app.com.example.target.hook_mode=native
app.com.example.target.model=ExampleModel
app.com.example.target._pref_spoofingEnabled=bool:true
app.com.example.target._pref_DEVICE_MODEL_ENABLED=bool:true
```

`active` is the editor's Active switch; `allowed` is controlled by the SimSpoofer profile list; `hook_mode` is `native` or `lsposed`. Zygisk requires all three to permit execution: active true, allowed true, and native mode. Missing/invalid records fail closed. `_pref_` entries persist all typed SharedPreferences values, including settings not yet implemented in the Native hook engine, so SimSpoofer can restore them when reopening the profile. The flat per-package files under `/data/adb/universal-samsung-spoof/profiles/` are compatibility caches only.

## Migration

On first boot with this module version, `post-fs-data.sh` imports `/system/spoof.prop` (or the bundled module profile if the former is absent) into `/data/adb/simspoof.prop`. It filters old scope metadata and forces `scope=per_app`, so a legacy global profile is not silently applied to all apps. Original source files are not deleted. Legacy flat values remain available for import into a SimSpoofer profile.

## Component responsibilities and limitation

- SimSpoofer reads an app's namespaced block on editor open and writes the block on Save/Active; profile-list changes export configured profiles and remove blocks for deleted apps.
- Zygisk reads `/data/adb/simspoof.prop` directly and uses only the matching app block. If the shared file exists, it never falls back to stale per-app caches or a global legacy profile.
- KPM source declares this same path as the agreed configuration ABI, but its current implementation is only a lifecycle/control stub and has no active kernel spoof hooks or file reader. A true KPM consumer still requires a supported userspace-to-KPM profile relay and actual KPM hook implementation; this source does not claim those kernel hooks are active.


## Independent Universal Spoof UI

`ui-android/` is a standalone companion APK that can manage the shared per-app config without using SimSpoofer's UI. It writes `app.<package>.*` records and maintains `scope=per_app`. A Native record is applied only when `hook_mode=native`, `active=true`, and `allowed=true`; an LSPosed record is an explicit skip/tombstone for Zygisk. The UI preserves other packages and unknown keys. It does not make unsupported hook families functional.
