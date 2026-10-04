#!/system/bin/sh
MODDIR=${0%/*}
mkdir -p /data/adb/universal-samsung-spoof/profiles
chmod 0755 /data/adb/universal-samsung-spoof/profiles

# Import the old reference once, but force per-app scope to avoid silently
# re-enabling spoofing for every app during migration.
if [ ! -f /data/adb/simspoof.prop ]; then
    TMP=/data/adb/simspoof.prop.tmp
    {
        echo 'format=simspoof-v1'
        echo 'managed=1'
        echo 'scope=per_app'
        if [ -f /system/spoof.prop ]; then
            grep -Ev '^(scope|managed|format|updated_at)=' /system/spoof.prop
        elif [ -f "$MODDIR/profile.prop" ]; then
            grep -Ev '^(scope|managed|format|updated_at)=' "$MODDIR/profile.prop"
        fi
    } > "$TMP"
    chmod 0644 "$TMP"
    mv "$TMP" /data/adb/simspoof.prop
fi
chmod 0644 /data/adb/simspoof.prop
