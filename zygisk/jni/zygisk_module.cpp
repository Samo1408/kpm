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
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, TAG, __VA_ARGS__)

static zygisk::Api *g_api = nullptr;
static JNIEnv *g_env = nullptr;
static spoof_profile g_profile;
static bool g_profile_ok = false;

static bool nonempty(const char *s) { return s && *s; }

static std::string spoof_for_property(const char *key) {
    if (!key) return {};
    const char *v = nullptr;

    // Device identity
    if (!strcmp(key, "ro.product.model")) v = g_profile.model;
    else if (!strcmp(key, "ro.product.brand")) v = g_profile.brand;
    else if (!strcmp(key, "ro.product.device")) v = g_profile.device;
    else if (!strcmp(key, "ro.product.name")) v = g_profile.name;
    else if (!strcmp(key, "ro.product.product")) v = g_profile.product;
    else if (!strcmp(key, "ro.product.manufacturer")) v = g_profile.manufacturer;
    else if (!strcmp(key, "ro.product.board")) v = g_profile.board;
    else if (!strcmp(key, "ro.hardware")) v = g_profile.hardware;
    else if (!strcmp(key, "ro.bootloader")) v = g_profile.bootloader;

    // Locale / country
    else if (!strcmp(key, "ro.product.locale")) v = g_profile.locale;
    else if (!strcmp(key, "ro.product.locale.region")) v = g_profile.country_iso;
    else if (!strcmp(key, "persist.sys.country")) v = g_profile.country_iso;

    // Telephony country / operator properties. These are the lowest-risk
    // framework-facing layer and are used by several Android/Samsung paths.
    else if (!strcmp(key, "gsm.operator.iso-country")) v = g_profile.country_iso;
    else if (!strcmp(key, "gsm.sim.operator.iso-country")) v = g_profile.country_iso;

    // Serial identity
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

static bool install_one(const char *label, JNINativeMethod *m) {
    if (!m->fnPtr) {
        LOGW("Hook unavailable: %s", label);
        return false;
    }
    LOGI("Hook installed: %s", label);
    return true;
}

static void install_system_property_hooks() {
    JNINativeMethod methods[] = {
        {"native_get", "(Ljava/lang/String;)Ljava/lang/String;", (void*)hooked_get_1},
        {"native_get", "(Ljava/lang/String;Ljava/lang/String;)Ljava/lang/String;", (void*)hooked_get_2},
        {"native_get_int", "(Ljava/lang/String;I)I", (void*)hooked_get_int},
        {"native_get_long", "(Ljava/lang/String;J)J", (void*)hooked_get_long},
        {"native_get_boolean", "(Ljava/lang/String;Z)Z", (void*)hooked_get_bool},
    };
    g_api->hookJniNativeMethods(g_env, "android/os/SystemProperties", methods, 5);
    orig_get_1 = (decltype(orig_get_1))methods[0].fnPtr;
    orig_get_2 = (decltype(orig_get_2))methods[1].fnPtr;
    orig_get_int = (decltype(orig_get_int))methods[2].fnPtr;
    orig_get_long = (decltype(orig_get_long))methods[3].fnPtr;
    orig_get_bool = (decltype(orig_get_bool))methods[4].fnPtr;
    int ok = 0;
    ok += install_one("SystemProperties.native_get/1", &methods[0]);
    ok += install_one("SystemProperties.native_get/2", &methods[1]);
    ok += install_one("SystemProperties.native_get_int", &methods[2]);
    ok += install_one("SystemProperties.native_get_long", &methods[3]);
    ok += install_one("SystemProperties.native_get_boolean", &methods[4]);
    LOGI("SystemProperties hook summary: %d/5 active", ok);
}


// Samsung's SemSystemProperties has changed implementation details between
// One UI / Android releases. We therefore probe several native signatures.
// hookJniNativeMethods() leaves fnPtr == nullptr when a method is not native
// or does not exist, so unsupported variants are harmless.
static jstring (*sem_orig_get_1)(JNIEnv*, jclass, jstring) = nullptr;
static jstring (*sem_orig_get_2)(JNIEnv*, jclass, jstring, jstring) = nullptr;
static jint (*sem_orig_get_int)(JNIEnv*, jclass, jstring, jint) = nullptr;
static jlong (*sem_orig_get_long)(JNIEnv*, jclass, jstring, jlong) = nullptr;
static jboolean (*sem_orig_get_bool)(JNIEnv*, jclass, jstring, jboolean) = nullptr;

static jstring sem_hook_get_1(JNIEnv *env, jclass c, jstring key) {
    std::string k = jstr(env, key);
    std::string v = spoof_for_property(k.c_str());
    if (!v.empty()) {
        LOGI("SemSystemProperties.get spoof: %s=%s", k.c_str(), v.c_str());
        return env->NewStringUTF(v.c_str());
    }
    return sem_orig_get_1 ? sem_orig_get_1(env, c, key) : nullptr;
}

static jstring sem_hook_get_2(JNIEnv *env, jclass c, jstring key, jstring def) {
    std::string k = jstr(env, key);
    std::string v = spoof_for_property(k.c_str());
    if (!v.empty()) {
        LOGI("SemSystemProperties.get(def) spoof: %s=%s", k.c_str(), v.c_str());
        return env->NewStringUTF(v.c_str());
    }
    return sem_orig_get_2 ? sem_orig_get_2(env, c, key, def) : def;
}

static jint sem_hook_get_int(JNIEnv *env, jclass c, jstring key, jint def) {
    return sem_orig_get_int ? sem_orig_get_int(env, c, key, def) : def;
}
static jlong sem_hook_get_long(JNIEnv *env, jclass c, jstring key, jlong def) {
    return sem_orig_get_long ? sem_orig_get_long(env, c, key, def) : def;
}
static jboolean sem_hook_get_bool(JNIEnv *env, jclass c, jstring key, jboolean def) {
    return sem_orig_get_bool ? sem_orig_get_bool(env, c, key, def) : def;
}

static void install_sem_system_properties_hooks() {
    JNINativeMethod methods[] = {
        {"get", "(Ljava/lang/String;)Ljava/lang/String;", (void*)sem_hook_get_1},
        {"get", "(Ljava/lang/String;Ljava/lang/String;)Ljava/lang/String;", (void*)sem_hook_get_2},
        {"getInt", "(Ljava/lang/String;I)I", (void*)sem_hook_get_int},
        {"getLong", "(Ljava/lang/String;J)J", (void*)sem_hook_get_long},
        {"getBoolean", "(Ljava/lang/String;Z)Z", (void*)sem_hook_get_bool},
        {"native_get", "(Ljava/lang/String;)Ljava/lang/String;", (void*)sem_hook_get_1},
        {"native_get", "(Ljava/lang/String;Ljava/lang/String;)Ljava/lang/String;", (void*)sem_hook_get_2},
    };

    g_api->hookJniNativeMethods(g_env, "android/os/SemSystemProperties", methods, 7);
    sem_orig_get_1 = (decltype(sem_orig_get_1))methods[0].fnPtr;
    sem_orig_get_2 = (decltype(sem_orig_get_2))methods[1].fnPtr;
    sem_orig_get_int = (decltype(sem_orig_get_int))methods[2].fnPtr;
    sem_orig_get_long = (decltype(sem_orig_get_long))methods[3].fnPtr;
    sem_orig_get_bool = (decltype(sem_orig_get_bool))methods[4].fnPtr;

    int ok = 0;
    for (int i = 0; i < 7; ++i) {
        if (methods[i].fnPtr) ++ok;
        else LOGW("SemSystemProperties variant unavailable: %s %s", methods[i].name, methods[i].signature);
    }
    LOGI("SemSystemProperties hook summary: %d/7 native variants active", ok);
}

static void verify_serial_and_country_apis() {
    // Build.getSerial() and TelephonyManager country APIs are Java methods on
    // modern Android and are therefore not JNI-native methods that can safely
    // be replaced through hookJniNativeMethods(). We deliberately do not patch
    // ART ArtMethod entry points here because that is Android/ART-version
    // specific and would defeat the boot-safe design of this module.
    jclass build = g_env->FindClass("android/os/Build");
    if (build) {
        jmethodID serial = g_env->GetStaticMethodID(build, "getSerial", "()Ljava/lang/String;");
        if (serial) LOGI("Build.getSerial() present; serial spoof is supplied through property layer");
        else g_env->ExceptionClear();
    } else g_env->ExceptionClear();

    jclass telephony = g_env->FindClass("android/telephony/TelephonyManager");
    if (telephony) {
        const char *sigs[] = {
            "()Ljava/lang/String;",
            "(I)Ljava/lang/String;"
        };
        jmethodID a = g_env->GetMethodID(telephony, "getNetworkCountryIso", sigs[0]);
        if (a) LOGI("TelephonyManager.getNetworkCountryIso() present"); else g_env->ExceptionClear();
        jmethodID b = g_env->GetMethodID(telephony, "getNetworkCountryIso", sigs[1]);
        if (b) LOGI("TelephonyManager.getNetworkCountryIso(int) present"); else g_env->ExceptionClear();
        jmethodID c = g_env->GetMethodID(telephony, "getSimCountryIso", "()Ljava/lang/String;");
        if (c) LOGI("TelephonyManager.getSimCountryIso() present"); else g_env->ExceptionClear();
    } else g_env->ExceptionClear();
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
            LOGW("No profile loaded; original values will be preserved");
        else
            LOGI("Profile ready: model=%s country=%s locale=%s", g_profile.model, g_profile.country_iso, g_profile.locale);
    }

    void postServerSpecialize(const zygisk::ServerSpecializeArgs *) override {
        if (!g_profile_ok || !g_api || !g_env) return;

        install_system_property_hooks();
        install_sem_system_properties_hooks();
        apply_build_fields();
        verify_serial_and_country_apis();
        LOGI("Phase 1-3: system_server Zygisk engine initialized safely");
    }
};

REGISTER_ZYGISK_MODULE(UniversalSamsungSpoof)
