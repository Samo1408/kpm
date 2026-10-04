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
