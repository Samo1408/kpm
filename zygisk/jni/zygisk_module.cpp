#include "zygisk.hpp"
#include "spoof_profile.h"

#include <android/log.h>
#include <jni.h>
#include <cstdlib>
#include <cstring>
#include <cctype>
#include <string>

#define TAG "UniversalSamsungSpoof"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, TAG, __VA_ARGS__)

static zygisk::Api *g_api = nullptr;
static JNIEnv *g_env = nullptr;
static spoof_profile g_profile;
static bool g_profile_ok = false;
static bool g_runtime_initialized = false;
static bool g_skip_app_runtime = false;

static bool nonempty(const char *s) { return s && *s; }

// Android Telephony country ISO properties are conventionally lower-case,
// while ro.product.locale.region is an upper-case region subtag. Keep these
// representations separate so one does not leak into the other.
static std::string telephony_country_iso() {
    const char *src = nonempty(g_profile.country_iso_lower)
        ? g_profile.country_iso_lower : g_profile.country_iso;
    std::string out = src ? src : "";
    for (char &ch : out) ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
    return out;
}

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
    else if (!strcmp(key, "ro.build.fingerprint")) v = g_profile.fingerprint;
    else if (!strcmp(key, "ro.build.id")) v = g_profile.build_id;
    else if (!strcmp(key, "ro.build.display.id")) v = g_profile.build_display;
    else if (!strcmp(key, "ro.build.version.security_patch")) v = g_profile.security_patch;
    else if (!strcmp(key, "gsm.version.baseband")) v = g_profile.baseband;
    else if (!strcmp(key, "ro.soc.model")) v = g_profile.soc_model;
    else if (!strcmp(key, "ro.soc.manufacturer")) v = g_profile.soc_manufacturer;
    else if (!strcmp(key, "ro.board.platform")) v = g_profile.board_platform;
    else if (!strcmp(key, "ro.build.host")) v = g_profile.host;
    else if (!strcmp(key, "ro.build.user")) v = g_profile.user;
    else if (!strcmp(key, "ro.build.signature")) v = g_profile.signature;

    // Locale / country
    else if (!strcmp(key, "ro.product.locale")) v = g_profile.locale;
    else if (!strcmp(key, "ro.product.locale.region")) v = g_profile.country_iso;
    else if (!strcmp(key, "persist.sys.country")) v = g_profile.country_iso;

    // Telephony country / operator properties. These are the lowest-risk
    // framework-facing layer and are used by several Android/Samsung paths.
    else if (!strcmp(key, "gsm.operator.iso-country")) {
        std::string iso = telephony_country_iso();
        return iso;
    }
    else if (!strcmp(key, "gsm.sim.operator.iso-country")) {
        std::string iso = telephony_country_iso();
        return iso;
    }
    else if (!strcmp(key, "gsm.operator.numeric")) v = g_profile.network_operator;
    else if (!strcmp(key, "gsm.operator.alpha")) v = g_profile.network_operator_name;
    else if (!strcmp(key, "gsm.sim.operator.numeric")) v = g_profile.sim_operator;
    else if (!strcmp(key, "gsm.sim.operator.alpha")) v = g_profile.sim_operator_name;
    else if (!strcmp(key, "persist.sys.timezone")) v = g_profile.timezone;

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
    set_build_field("FINGERPRINT", g_profile.fingerprint);
    set_build_field("ID", g_profile.build_id);
    set_build_field("DISPLAY", g_profile.build_display);
    jclass version = g_env->FindClass("android/os/Build$VERSION");
    if (version && nonempty(g_profile.security_patch)) {
        jfieldID patch = g_env->GetStaticFieldID(version, "SECURITY_PATCH", "Ljava/lang/String;");
        if (patch) g_env->SetStaticObjectField(version, patch, g_env->NewStringUTF(g_profile.security_patch));
        if (g_env->ExceptionCheck()) g_env->ExceptionClear();
    } else if (!version) g_env->ExceptionClear();
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


// SemSystemProperties.get* methods are Java wrappers on many Samsung builds,
// not JNI-native methods. Zygisk's hookJniNativeMethods API cannot replace
// ordinary ART Java methods, so probing them as native variants always yields
// 0/N on those builds and gives a misleading impression of missing native hooks.
// These wrappers commonly delegate to android.os.SystemProperties, whose JNI
// methods are hooked above. We log this limitation explicitly rather than
// attempting unsafe ART entry-point patching that varies by Android release.
static void install_sem_system_properties_hooks() {
    LOGI("SemSystemProperties: direct Java wrappers are not hookable via JNI-native API; relying on SystemProperties JNI hooks where delegated");
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

static bool load_profile_if_needed() {
    if (g_profile_ok) return true;

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

    if (!g_profile_ok) {
        LOGW("No profile loaded; original values will be preserved");
        return false;
    }

    LOGI("Profile ready: model=%s country=%s locale=%s",
         g_profile.model, g_profile.country_iso, g_profile.locale);
    return true;
}

static bool load_app_profile_for_package(const std::string &package_name) {
    if (package_name.empty()) return false;
    const std::string path = "/data/adb/universal-samsung-spoof/profiles/" + package_name + ".prop";
    spoof_profile candidate;
    spoof_profile_init(&candidate);
    if (spoof_profile_load(&candidate, path.c_str()) != 0) return false;

    if (!strcmp(candidate.hook_mode, "lsposed")) {
        g_skip_app_runtime = true;
        g_profile_ok = false;
        LOGI("Per-app mode is LSPosed; skipping Zygisk app hooks for %s", package_name.c_str());
        return true;
    }
    if (strcmp(candidate.hook_mode, "native") != 0) {
        LOGW("Unknown hook_mode in %s; retaining legacy profile behavior", path.c_str());
        return false;
    }

    g_profile = candidate;
    g_profile_ok = true;
    g_skip_app_runtime = false;
    LOGI("Loaded per-app Native profile for %s", package_name.c_str());
    return true;
}

static void initialize_spoof_runtime(const char *process_kind) {
    if (g_runtime_initialized) return;
    if (!g_api || !g_env || !load_profile_if_needed()) return;

    install_system_property_hooks();
    install_sem_system_properties_hooks();
    apply_build_fields();
    verify_serial_and_country_apis();
    g_runtime_initialized = true;
    LOGI("Spoof runtime initialized in %s", process_kind);
}

class UniversalSamsungSpoof : public zygisk::ModuleBase {
public:
    void onLoad(zygisk::Api *api, JNIEnv *env) override {
        g_api = api;
        g_env = env;
    }

    void preAppSpecialize(zygisk::AppSpecializeArgs *args) override {
        // Explicit per-app mode overrides the legacy global profile for that app.
        // Profiles are named after the package (the process may include :remote).
        g_skip_app_runtime = false;
        g_profile_ok = false;
        std::string process_name = args && args->nice_name ? jstr(g_env, args->nice_name) : std::string();
        size_t colon = process_name.find(':');
        std::string package_name = process_name.substr(0, colon);
        if (!load_app_profile_for_package(package_name)) {
            // Backwards compatibility for apps without a mode record.
            load_profile_if_needed();
        }
    }

    void postAppSpecialize(const zygisk::AppSpecializeArgs *) override {
        if (g_skip_app_runtime) return;
        initialize_spoof_runtime("app_process");
    }

    void preServerSpecialize(zygisk::ServerSpecializeArgs *) override {
        load_profile_if_needed();
    }

    void postServerSpecialize(const zygisk::ServerSpecializeArgs *) override {
        initialize_spoof_runtime("system_server");
    }
};
REGISTER_ZYGISK_MODULE(UniversalSamsungSpoof)
