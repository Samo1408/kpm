#include <compiler.h>
#include <kpmodule.h>
#include <linux/printk.h>

KPM_NAME("universal-samsung-spoof");
KPM_VERSION("0.1.0");
KPM_LICENSE("GPL v2");
KPM_AUTHOR("Universal Samsung Spoof");
KPM_DESCRIPTION("Universal Samsung spoof KPM foundation");

static long uss_init(const char *args, const char *event, void *reserved)
{
    pr_info("universal-samsung-spoof: init event=%s args=%s\n",
            event ? event : "(null)",
            args ? args : "(null)");
    return 0;
}

static long uss_control0(const char *args, char *__user out_msg, int outlen)
{
    (void)args;
    (void)out_msg;
    (void)outlen;
    return 0;
}

static long uss_exit(void *reserved)
{
    pr_info("universal-samsung-spoof: exit\n");
    return 0;
}

KPM_INIT(uss_init);
KPM_CTL0(uss_control0);
KPM_EXIT(uss_exit);
