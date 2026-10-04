#include <compiler.h>
#include <kpmodule.h>
#include <linux/printk.h>
#include <linux/string.h>

KPM_NAME("universal-samsung-spoof");
KPM_VERSION("0.2.0-alpha2");
KPM_LICENSE("GPL v2");
KPM_AUTHOR("Universal-Samsung-Spoof");
KPM_DESCRIPTION("Safe kernel-side companion; no kernel hooks enabled by default.");

static long uss_init(const char *args, const char *event, void *reserved) {
    pr_info("universal-samsung-spoof: init event=%s args=%s\n",
            event ? event : "", args ? args : "");
    return 0;
}

static long uss_ctl0(const char *args, char *__user out_msg, int outlen) {
    const char *msg = "universal-samsung-spoof: KPM online; no kernel hook enabled";
    if (out_msg && outlen > 0)
        compat_copy_to_user(out_msg, msg, strlen(msg) + 1);
    pr_info("universal-samsung-spoof: ctl0 args=%s\n", args ? args : "");
    return 0;
}

static long uss_exit(void *reserved) {
    pr_info("universal-samsung-spoof: exit\n");
    return 0;
}

KPM_INIT(uss_init);
KPM_CTL0(uss_ctl0);
KPM_EXIT(uss_exit);
