# Universal Samsung Spoof — Zygisk Entry Fix

This source keeps the working KernelPatch KPM and Phase 4 spoof engine, but replaces the handwritten Zygisk ABI header with the official public Zygisk API layout (API v5) and adds an ELF export check for `zygisk_module_entry`.

## Important

- No `framework.jar` / DEX patching.
- KPM code is unchanged from the working lifecycle-only foundation.
- Zygisk uses `REGISTER_ZYGISK_MODULE(UniversalSamsungSpoof)`.
- The build fails if `zygisk_module_entry` is not a global/default exported symbol.
- The current spoof hooks remain the same Phase 4 hooks; this change fixes loading first.

The official Zygisk documentation states that modules should inherit `zygisk::ModuleBase` and use `REGISTER_ZYGISK_MODULE`, and that the public API header is the canonical `zygisk.hpp`. citeturn0search1turn3view0


Zygisk entrypoint fix: zygisk_module_entry and zygisk_companion_entry are explicitly defined with C linkage and default visibility. CI verifies the dynamic symbol table before packaging.
