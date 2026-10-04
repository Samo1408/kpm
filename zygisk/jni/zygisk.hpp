#pragma once
#include <jni.h>
#include <stdint.h>
#include <sys/types.h>

#define ZYGISK_API_VERSION 5

namespace zygisk {

struct Api;
struct AppSpecializeArgs;
struct ServerSpecializeArgs;

class ModuleBase {
public:
    virtual void onLoad(Api*, JNIEnv*) {}
    virtual void preAppSpecialize(AppSpecializeArgs*) {}
    virtual void postAppSpecialize(const AppSpecializeArgs*) {}
    virtual void preServerSpecialize(ServerSpecializeArgs*) {}
    virtual void postServerSpecialize(const ServerSpecializeArgs*) {}
};

enum Option : int {
    FORCE_DENYLIST_UNMOUNT = 0,
    DLCLOSE_MODULE_LIBRARY = 1,
};

namespace internal {
struct module_abi;
struct api_table;
void internal_set_api_table(Api*, api_table*);
}

struct Api {
    int connectCompanion();
    int getModuleDir();
    void setOption(Option);
    uint32_t getFlags();
    bool exemptFd(int);
    void hookJniNativeMethods(JNIEnv*, const char*, JNINativeMethod*, int);
    void pltHookRegister(dev_t, ino_t, const char*, void*, void**);
    bool pltHookCommit();
private:
    internal::api_table *tbl = nullptr;
    friend void internal::internal_set_api_table(Api*, internal::api_table*);
};

namespace internal {

struct api_table;

struct module_abi {
    long api_version;
    ModuleBase *impl;
    void (*preAppSpecialize)(ModuleBase*, AppSpecializeArgs*);
    void (*postAppSpecialize)(ModuleBase*, const AppSpecializeArgs*);
    void (*preServerSpecialize)(ModuleBase*, ServerSpecializeArgs*);
    void (*postServerSpecialize)(ModuleBase*, const ServerSpecializeArgs*);

    explicit module_abi(ModuleBase *m)
        : api_version(ZYGISK_API_VERSION), impl(m) {
        preAppSpecialize=[](ModuleBase *x, AppSpecializeArgs *a){x->preAppSpecialize(a);};
        postAppSpecialize=[](ModuleBase *x, const AppSpecializeArgs *a){x->postAppSpecialize(a);};
        preServerSpecialize=[](ModuleBase *x, ServerSpecializeArgs *a){x->preServerSpecialize(a);};
        postServerSpecialize=[](ModuleBase *x, const ServerSpecializeArgs *a){x->postServerSpecialize(a);};
    }
};

struct api_table {
    void *impl;
    bool (*registerModule)(api_table*, module_abi*);
    void (*hookJniNativeMethods)(JNIEnv*, const char*, JNINativeMethod*, int);
    void (*pltHookRegister)(dev_t, ino_t, const char*, void*, void**);
    bool (*exemptFd)(int);
    bool (*pltHookCommit)();
    int (*connectCompanion)(void*);
    void (*setOption)(void*, Option);
    int (*getModuleDir)(void*);
    uint32_t (*getFlags)(void*);
};

inline void internal_set_api_table(Api *api, api_table *table) {
    api->tbl = table;
}

template<class T>
void entry_impl(api_table *t, JNIEnv *env) {
    static T module;
    static Api api;
    internal_set_api_table(&api, t);
    static module_abi abi(&module);
    if (!t || !t->registerModule || !t->registerModule(t, &abi))
        return;
    module.onLoad(&api, env);
}

} // namespace internal

inline int Api::connectCompanion() {
    return tbl && tbl->connectCompanion ? tbl->connectCompanion(tbl->impl) : -1;
}
inline int Api::getModuleDir() {
    return tbl && tbl->getModuleDir ? tbl->getModuleDir(tbl->impl) : -1;
}
inline void Api::setOption(Option o) {
    if (tbl && tbl->setOption) tbl->setOption(tbl->impl, o);
}
inline uint32_t Api::getFlags() {
    return tbl && tbl->getFlags ? tbl->getFlags(tbl->impl) : 0;
}
inline bool Api::exemptFd(int fd) {
    return tbl && tbl->exemptFd ? tbl->exemptFd(fd) : false;
}
inline void Api::hookJniNativeMethods(JNIEnv *e, const char *c, JNINativeMethod *m, int n) {
    if (tbl && tbl->hookJniNativeMethods) tbl->hookJniNativeMethods(e, c, m, n);
}
inline void Api::pltHookRegister(dev_t d, ino_t i, const char *s, void *nf, void **of) {
    if (tbl && tbl->pltHookRegister) tbl->pltHookRegister(d, i, s, nf, of);
}
inline bool Api::pltHookCommit() {
    return tbl && tbl->pltHookCommit ? tbl->pltHookCommit() : false;
}

template<class T>
inline void register_module() {}

} // namespace zygisk

#define REGISTER_ZYGISK_MODULE(cls) \
extern "C" __attribute__((visibility("default"))) \
void zygisk_module_entry(zygisk::internal::api_table *api, JNIEnv *env) { \
    zygisk::internal::entry_impl<cls>(api, env); \
}
