#include "zygisk.hpp"
#include "spoof_profile.h"

#include <android/log.h>
#include <jni.h>
#include <cstdlib>
#include <cstring>
#include <string>

#define TAG "UniversalSamsungSpoof"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, TAG, __VA_ARGS__)

static zygisk::Api *g_api = nullptr;
static JNIEnv *g_env = nullptr;
static spoof_profile g_profile;
static bool g_profile_ok = false;

static bool nonempty(const char *s) { return s && *s; }

static std::string spoof_for_property(const char *key) {
    if (!key) return {};
    const char *v = nullptr;

    if (!strcmp(key, "ro.product.model")) v = g_profile.model;
    else if (!strcmp(key, "ro.product.brand")) v = g_profile.brand;
    else if (!strcmp(key, "ro.product.device")) v = g_profile.device;
    else if (!strcmp(key, "ro.product.name")) v = g_profile.name;
    else if (!strcmp(key, "ro.product.product")) v = g_profile.product;
    else if (!strcmp(key, "ro.product.manufacturer")) v = g_profile.manufacturer;
    else if (!strcmp(key, "ro.product.board")) v = g_profile.board;
    else if (!strcmp(key, "ro.hardware")) v = g_profile.hardware;
    else if (!strcmp(key, "ro.bootloader")) v = g_profile.bootloader;
    else if (!strcmp(key, "ro.product.locale")) v = g_profile.locale;
    else if (!strcmp(key, "gsm.operator.iso-country")) v = g_profile.country_iso;
    else if (!strcmp(key, "gsm.sim.operator.iso-country")) v = g_profile.country_iso;
    else if (!strcmp(key, "ro.serialno")) v = g_profile.serial;
    else if (!strcmp(key, "ril.serialnumber")) v = g_profile.serial;
    else if (!strcmp(key, "sys.serialnumber")) v = g_profile.serial;

    return nonempty(v) ? std::string(v) : std::string();
}

static jstring (*orig_get_1)(JNIEnv*, jclass, jstring) = nullptr;
static jstring (*orig_get_2)(JNIEnv*, jclass, jstring, jstring) = nullptr;
static jint (*orig_get_int)(JNIEnv*, jclass, jstring, jint) = nullptr;
static jlong (*orig_get_long)(JNIEnv*, jclass, jstring, jlong) = nullptr;
static jboolean (*orig_get_bool)(JNIEnv*, jclass, jstring, jboolean) = nullptr;

static std::string jstr(JNIEnv *env, jstring s) {
    if (!s) return {};
    const char *p = env->GetStringUTFChars(s, nullptr);
    std::string out = p ? p : "";
    if (p) env->ReleaseStringUTFChars(s, p);
    return out;
}

static jstring hooked_get_1(JNIEnv *env, jclass c, jstring key) {
    std::string k = jstr(env,key);
    std::string v = spoof_for_property(k.c_str());
    if (!v.empty()) {
        LOGI("SystemProperties.get spoof: %s=%s", k.c_str(), v.c_str());
        return env->NewStringUTF(v.c_str());
    }
    return orig_get_1 ? orig_get_1(env,c,key) : nullptr;
}

static jstring hooked_get_2(JNIEnv *env, jclass c, jstring key, jstring def) {
    std::string k = jstr(env,key);
    std::string v = spoof_for_property(k.c_str());
    if (!v.empty()) {
        LOGI("SystemProperties.get spoof: %s=%s", k.c_str(), v.c_str());
        return env->NewStringUTF(v.c_str());
    }
    return orig_get_2 ? orig_get_2(env,c,key,def) : def;
}

static jint hooked_get_int(JNIEnv *env, jclass c, jstring key, jint def) {
    return orig_get_int ? orig_get_int(env,c,key,def) : def;
}
static jlong hooked_get_long(JNIEnv *env, jclass c, jstring key, jlong def) {
    return orig_get_long ? orig_get_long(env,c,key,def) : def;
}
static jboolean hooked_get_bool(JNIEnv *env, jclass c, jstring key, jboolean def) {
    return orig_get_bool ? orig_get_bool(env,c,key,def) : def;
}

static void set_build_field(const char *name, const char *value) {
    if (!value || !*value || !g_env) return;
    jclass cls = g_env->FindClass("android/os/Build");
    if (!cls) { g_env->ExceptionClear(); return; }
    jfieldID f = g_env->GetStaticFieldID(cls, name, "Ljava/lang/String;");
    if (!f) { g_env->ExceptionClear(); return; }
    g_env->SetStaticObjectField(cls, f, g_env->NewStringUTF(value));
    if (g_env->ExceptionCheck()) g_env->ExceptionClear();
}

static void apply_build_fields() {
    set_build_field("MODEL", g_profile.model);
    set_build_field("BRAND", g_profile.brand);
    set_build_field("DEVICE", g_profile.device);
    set_build_field("PRODUCT", g_profile.product);
    set_build_field("MANUFACTURER", g_profile.manufacturer);
    set_build_field("BOARD", g_profile.board);
    set_build_field("HARDWARE", g_profile.hardware);
    set_build_field("BOOTLOADER", g_profile.bootloader);
    LOGI("Build static fields updated from profile");
}

static void install_system_property_hooks() {
    JNINativeMethod methods[] = {
        {"native_get", "(Ljava/lang/String;)Ljava/lang/String;", (void*)hooked_get_1},
        {"native_get", "(Ljava/lang/String;Ljava/lang/String;)Ljava/lang/String;", (void*)hooked_get_2},
        {"native_get_int", "(Ljava/lang/String;I)I", (void*)hooked_get_int},
        {"native_get_long", "(Ljava/lang/String;J)J", (void*)hooked_get_long},
        {"native_get_boolean", "(Ljava/lang/String;Z)Z", (void*)hooked_get_bool},
    };

    g_api->hookJniNativeMethods(g_env, "android/os/SystemProperties", methods,
                                sizeof(methods)/sizeof(methods[0]));

    orig_get_1 = (decltype(orig_get_1))methods[0].fnPtr;
    orig_get_2 = (decltype(orig_get_2))methods[1].fnPtr;
    orig_get_int = (decltype(orig_get_int))methods[2].fnPtr;
    orig_get_long = (decltype(orig_get_long))methods[3].fnPtr;
    orig_get_bool = (decltype(orig_get_bool))methods[4].fnPtr;

    LOGI("SystemProperties hooks: get=%p get2=%p int=%p long=%p bool=%p",
         (void*)orig_get_1, (void*)orig_get_2, (void*)orig_get_int,
         (void*)orig_get_long, (void*)orig_get_bool);
}

class UniversalSamsungSpoof : public zygisk::ModuleBase {
public:
    void onLoad(zygisk::Api *api, JNIEnv *env) override {
        g_api = api;
        g_env = env;
    }

    void preServerSpecialize(zygisk::ServerSpecializeArgs *) override {
        spoof_profile_init(&g_profile);

        const char *paths[] = {
            "/data/adb/universal-samsung-spoof/profile.prop",
            "/data/adb/modules/universal-samsung-spoof/profile.prop",
            "/data/local/tmp/universal-samsung-spoof/profile.prop"
        };

        for (const char *p : paths) {
            if (spoof_profile_load(&g_profile, p) == 0) {
                g_profile_ok = true;
                LOGI("Loaded profile: %s", p);
                break;
            }
        }

        if (!g_profile_ok)
            LOGW("No profile loaded; all hooks will fall back to original values");
    }

    void postServerSpecialize(const zygisk::ServerSpecializeArgs *) override {
        if (!g_profile_ok || !g_api || !g_env) return;

        install_system_property_hooks();
        apply_build_fields();

        LOGI("Alpha2 system_server spoof engine initialized");
    }
};

REGISTER_ZYGISK_MODULE(UniversalSamsungSpoof)
