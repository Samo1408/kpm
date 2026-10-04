#!/system/bin/sh
MODDIR=${0%/*}
mkdir -p /data/adb/universal-samsung-spoof
mkdir -p /data/adb/universal-samsung-spoof/profiles
chmod 0755 /data/adb/universal-samsung-spoof/profiles
[ -f /data/adb/universal-samsung-spoof/profile.prop ] || cp "$MODDIR/profile.prop" /data/adb/universal-samsung-spoof/profile.prop
chmod 0644 /data/adb/universal-samsung-spoof/profile.prop
