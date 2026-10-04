# Independent UI implementation status

Implemented in this source phase:

- Standalone Android companion UI project in `ui-android/` with a dark card-based interface.
- Installed-app list/search using Android PackageManager.
- Per-app `hook_mode` selection (`native` / `lsposed`), active switch, allow-list switch, and editor for fields currently represented by the Native profile parser.
- Root-only read/write of `/data/adb/simspoof.prop` using a base64 payload and atomic temp-file rename; package name validation and line-break sanitization are applied before writing.
- Per-app config update preserves other package records and unknown keys, including serialized SimSpoofer preference records.
- UI does not depend on LSPosed scope for Native mode; LSPosed selection leaves a clear skip record for Zygisk.
- CI workflow updated to build the APK and include it in the complete artifact.

Validation performed locally:

- Shared profile C parser compiled with Clang warnings-as-errors and passed a per-package isolation test.
- Web UI JavaScript syntax checked with Node.
- Android manifest XML parsed successfully.
- Shell scripts passed `bash -n`; workflow YAML parsed successfully.
- ZIP source integrity is checked after packaging.

Not verified locally: APK compilation or device runtime. The Gradle wrapper could not download Gradle because this environment has no DNS/network access to `services.gradle.org`. The CI workflow builds the APK in a network-enabled runner. The Native engine still only applies the hook families documented in `HOOK_MATRIX.md`; this UI does not implement missing hooks by itself.
