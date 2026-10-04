#include <compiler.h>
#include <kpmodule.h>

/* Shared userspace configuration ABI; no KPM spoof hooks are active yet. */
#define USS_SHARED_CONFIG_PATH "/data/adb/simspoof.prop"

KPM_NAME("universal-samsung-spoof");
KPM_VERSION("0.1.0");
KPM_LICENSE("GPL v2");
KPM_AUTHOR("Universal Samsung Spoof");
KPM_DESCRIPTION("Universal Samsung spoof KPM foundation");

static long uss_init(const char *args, const char *event, void *reserved)
{
    (void)args;
    (void)event;
    (void)reserved;
    (void)USS_SHARED_CONFIG_PATH;
    return 0;
}

static long uss_control0(const char *args, char *__user out_msg, int outlen)
{
    (void)args;
    (void)out_msg;
    (void)outlen;
    /* Reserved for a validated userspace-to-KPM profile relay. */
    return 0;
}

static long uss_exit(void *reserved)
{
    (void)reserved;
    return 0;
}

KPM_INIT(uss_init);
KPM_CTL0(uss_control0);
KPM_EXIT(uss_exit);
