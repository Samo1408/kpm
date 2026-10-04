#!/system/bin/sh
MODDIR=${0%/*}
mkdir -p /data/adb/universal-samsung-spoof
[ -f /data/adb/universal-samsung-spoof/profile.prop ] || cp "$MODDIR/profile.prop" /data/adb/universal-samsung-spoof/profile.prop
chmod 0644 /data/adb/universal-samsung-spoof/profile.prop
