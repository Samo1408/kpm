# UI implementation status

The original standalone Android companion UI was an earlier experiment. It has been removed from the delivered project and is no longer built or published. The current UI is the in-module WebUI described below. Historical notes about the former APK are not a statement that it remains in this source tree.

## Direction update — in-module WebUI

The standalone `ui-android/` companion APK and its build target have been removed from the delivered project direction. The module now packages `webroot/index.html`, `webroot/app.js`, and `webroot/style.css`. The page uses the KernelSU-style `ksu.exec(command, callback)` bridge (or a compatible bridge) to enumerate package IDs and write per-app profile keys to `/data/adb/simspoof.prop`. This is configuration UI for the existing native engine, not proof that unsupported Java APIs or KPM hooks have been implemented.
