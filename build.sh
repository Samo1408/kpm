\
#!/usr/bin/env bash
set -euo pipefail

: "${ANDROID_NDK:?Set ANDROID_NDK to an Android NDK installation}"
: "${KP_DIR:=./KernelPatch}"
: "${TARGET_COMPILE:=aarch64-none-elf-}"

"$ANDROID_NDK/ndk-build" -C zygisk/jni \
  NDK_PROJECT_PATH="$PWD/zygisk/jni" \
  APP_BUILD_SCRIPT="$PWD/zygisk/jni/Android.mk" \
  NDK_APPLICATION_MK="$PWD/zygisk/jni/Application.mk"

make -C kpm KP_DIR="$KP_DIR" TARGET_COMPILE="$TARGET_COMPILE"

rm -rf dist universal-samsung-spoof
mkdir -p dist/universal-samsung-spoof/zygisk
cp packaging/module.prop dist/universal-samsung-spoof/module.prop
cp packaging/profile.prop dist/universal-samsung-spoof/profile.prop
cp packaging/post-fs-data.sh dist/universal-samsung-spoof/post-fs-data.sh
cp packaging/service.sh dist/universal-samsung-spoof/service.sh
cp zygisk/jni/libs/arm64-v8a/libuniversal_spoof.so dist/universal-samsung-spoof/zygisk/arm64-v8a.so
chmod 0755 dist/universal-samsung-spoof/*.sh

cd dist
zip -r ../universal-samsung-spoof-zygisk-alpha2.zip universal-samsung-spoof >/dev/null
cp ../kpm/universal-samsung-spoof.kpm .
cp ../docs/HOOK_MATRIX.md .
zip -r ../universal-samsung-spoof-complete-alpha2.zip . >/dev/null

echo "Build complete."
