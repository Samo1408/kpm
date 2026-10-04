#pragma once
#ifdef __cplusplus
extern "C" {
#endif

#define SPOOF_MAX_VALUE 192

typedef struct {
    char model[SPOOF_MAX_VALUE];
    char brand[SPOOF_MAX_VALUE];
    char device[SPOOF_MAX_VALUE];
    char product[SPOOF_MAX_VALUE];
    char manufacturer[SPOOF_MAX_VALUE];
    char name[SPOOF_MAX_VALUE];
    char board[SPOOF_MAX_VALUE];
    char hardware[SPOOF_MAX_VALUE];
    char bootloader[SPOOF_MAX_VALUE];
    char locale[SPOOF_MAX_VALUE];
    char country[SPOOF_MAX_VALUE];
    char country_iso[SPOOF_MAX_VALUE];
    char country_iso_lower[SPOOF_MAX_VALUE];
    char serial[SPOOF_MAX_VALUE];
    char hook_mode[SPOOF_MAX_VALUE];
    char fingerprint[SPOOF_MAX_VALUE];
    char build_id[SPOOF_MAX_VALUE];
    char build_display[SPOOF_MAX_VALUE];
    char security_patch[SPOOF_MAX_VALUE];
    char baseband[SPOOF_MAX_VALUE];
    char soc_model[SPOOF_MAX_VALUE];
    char soc_manufacturer[SPOOF_MAX_VALUE];
    char board_platform[SPOOF_MAX_VALUE];
    char host[SPOOF_MAX_VALUE];
    char user[SPOOF_MAX_VALUE];
    char signature[SPOOF_MAX_VALUE];
    char network_operator[SPOOF_MAX_VALUE];
    char network_operator_name[SPOOF_MAX_VALUE];
    char sim_operator[SPOOF_MAX_VALUE];
    char sim_operator_name[SPOOF_MAX_VALUE];
    char timezone[SPOOF_MAX_VALUE];
} spoof_profile;

void spoof_profile_init(spoof_profile *p);
int spoof_profile_load(spoof_profile *p, const char *path);
const char *spoof_profile_get(const spoof_profile *p, const char *key);

#ifdef __cplusplus
}
#endif
