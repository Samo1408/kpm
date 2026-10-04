#include "spoof_profile.h"
#include <stdio.h>
#include <string.h>
#include <ctype.h>

static void trim(char *s) {
    char *a = s;
    while (*a && isspace((unsigned char)*a)) a++;
    if (a != s) memmove(s, a, strlen(a) + 1);
    size_t n = strlen(s);
    while (n && isspace((unsigned char)s[n - 1])) s[--n] = 0;
}

void spoof_profile_init(spoof_profile *p) {
    if (p) memset(p, 0, sizeof(*p));
}

static char *slot(spoof_profile *p, const char *k) {
    if (!strcmp(k,"model")) return p->model;
    if (!strcmp(k,"brand")) return p->brand;
    if (!strcmp(k,"device")) return p->device;
    if (!strcmp(k,"product")) return p->product;
    if (!strcmp(k,"manufacturer")) return p->manufacturer;
    if (!strcmp(k,"name")) return p->name;
    if (!strcmp(k,"board")) return p->board;
    if (!strcmp(k,"hardware")) return p->hardware;
    if (!strcmp(k,"bootloader")) return p->bootloader;
    if (!strcmp(k,"locale")) return p->locale;
    if (!strcmp(k,"country")) return p->country;
    if (!strcmp(k,"country_iso")) return p->country_iso;
    if (!strcmp(k,"serial")) return p->serial;
    return NULL;
}

int spoof_profile_load(spoof_profile *p, const char *path) {
    if (!p || !path) return -1;
    FILE *f = fopen(path, "re");
    if (!f) return -2;

    char line[512];
    while (fgets(line, sizeof(line), f)) {
        trim(line);
        if (!line[0] || line[0] == '#') continue;

        char *eq = strchr(line, '=');
        if (!eq) continue;
        *eq++ = 0;
        trim(line);
        trim(eq);

        char *dst = slot(p, line);
        if (!dst) continue;
        snprintf(dst, SPOOF_MAX_VALUE, "%s", eq);
    }
    fclose(f);
    return 0;
}

const char *spoof_profile_get(const spoof_profile *p, const char *key) {
    if (!p || !key) return NULL;
    if (!strcmp(key,"model")) return p->model;
    if (!strcmp(key,"brand")) return p->brand;
    if (!strcmp(key,"device")) return p->device;
    if (!strcmp(key,"product")) return p->product;
    if (!strcmp(key,"manufacturer")) return p->manufacturer;
    if (!strcmp(key,"name")) return p->name;
    if (!strcmp(key,"board")) return p->board;
    if (!strcmp(key,"hardware")) return p->hardware;
    if (!strcmp(key,"bootloader")) return p->bootloader;
    if (!strcmp(key,"locale")) return p->locale;
    if (!strcmp(key,"country")) return p->country;
    if (!strcmp(key,"country_iso")) return p->country_iso;
    if (!strcmp(key,"serial")) return p->serial;
    return NULL;
}
