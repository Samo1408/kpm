# Zygisk entry-point fix

The previous build used `-fvisibility=hidden` while the hand-written Zygisk
registration macro did not explicitly export `zygisk_module_entry`.

This build marks `zygisk_module_entry` and `zygisk_companion_entry` with
`visibility("default")` and the CI workflow verifies the exported symbol with
`readelf` before packaging.

The KPM source is unchanged.
