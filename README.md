# Universal Samsung Spoof — Zygisk Entry Fix

This source keeps the KernelPatch KPM foundation and Phase 4 spoof engine, uses the public Zygisk API layout (API v5), and checks the ELF export for `zygisk_module_entry`.

## Important

- No `framework.jar` / DEX patching.
- KPM code is unchanged from the working lifecycle-only foundation.
- Zygisk uses `REGISTER_ZYGISK_MODULE(UniversalSamsungSpoof)`.
- The build fails if `zygisk_module_entry` is not a global/default exported symbol.
- The same Phase 4 hooks are initialized in both `system_server` and specialized app processes.
- `SemSystemProperties` remains signature/version dependent; unavailable variants are logged and are not counted as active.

The official Zygisk documentation states that modules should inherit `zygisk::ModuleBase` and use `REGISTER_ZYGISK_MODULE`, and that the public API header is the canonical `zygisk.hpp`. citeturn0search1turn3view0


Zygisk entrypoint fix: zygisk_module_entry and zygisk_companion_entry are explicitly defined with C linkage and default visibility. CI verifies the dynamic symbol table before packaging.


## SimSpoofer hook-mode bridge (initial integration)

SimSpoofer can export a per-package mode record to:
`/data/adb/universal-samsung-spoof/profiles/<package>.prop`.

- `hook_mode=native`: Zygisk loads the per-package profile and applies the properties currently supported by `spoof_profile`.
- `hook_mode=lsposed`: Zygisk skips its app-process runtime for that explicitly configured package so the LSPosed route can be used instead.
- Apps without a per-package mode file retain the legacy profile behavior for compatibility.
- The bridge currently maps device Build/property fields, hardware serial, and country ISO where corresponding SimSpoofer fields are enabled/saved. It does not yet port all Java-level SIM, location, DRM, Android ID, App Set ID, or other SimSpoofer hooks to Native.
- `kpm/main.c` remains a lifecycle/control foundation; it does not yet implement KPM hooks. Do not interpret the Native selector as proof that KPM spoof hooks are active.

Native mode export requires root access from the SimSpoofer UI and an installed module at `/data/adb/modules/universal-samsung-spoof`. If export fails, the app displays a warning and the saved LSPosed preferences remain intact.
