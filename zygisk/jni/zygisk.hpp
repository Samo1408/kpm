#pragma once
#include <jni.h>
#include <sys/types.h>
#define ZYGISK_API_VERSION 5
namespace zygisk {
struct Api;
struct AppSpecializeArgs;
struct ServerSpecializeArgs;
class ModuleBase {
public:
    virtual void onLoad([[maybe_unused]] Api *api, [[maybe_unused]] JNIEnv *env) {}
    virtual void preAppSpecialize([[maybe_unused]] AppSpecializeArgs *args) {}
    virtual void postAppSpecialize([[maybe_unused]] const AppSpecializeArgs *args) {}
    virtual void preServerSpecialize([[maybe_unused]] ServerSpecializeArgs *args) {}
    virtual void postServerSpecialize([[maybe_unused]] const ServerSpecializeArgs *args) {}
};
struct AppSpecializeArgs {
    jint &uid; jint &gid; jintArray &gids; jint &runtime_flags; jobjectArray &rlimits;
    jint &mount_external; jstring &se_info; jstring &nice_name; jstring &instruction_set; jstring &app_data_dir;
    jintArray *const fds_to_ignore; jboolean *const is_child_zygote; jboolean *const is_top_app;
    jobjectArray *const pkg_data_info_list; jobjectArray *const whitelisted_data_info_list;
    jboolean *const mount_data_dirs; jboolean *const mount_storage_dirs; jboolean *const mount_sysprop_overrides;
    AppSpecializeArgs() = delete;
};
struct ServerSpecializeArgs {
    jint &uid; jint &gid; jintArray &gids; jint &runtime_flags;
    jlong &permitted_capabilities; jlong &effective_capabilities;
    ServerSpecializeArgs() = delete;
};
namespace internal { struct api_table; template <class T> void entry_impl(api_table *, JNIEnv *); }
enum Option : int { FORCE_DENYLIST_UNMOUNT = 0, DLCLOSE_MODULE_LIBRARY = 1 };
enum StateFlag : uint32_t { PROCESS_GRANTED_ROOT = (1u << 0), PROCESS_ON_DENYLIST = (1u << 1) };
struct Api {
    int connectCompanion(); int getModuleDir(); void setOption(Option); uint32_t getFlags(); bool exemptFd(int);
    void hookJniNativeMethods(JNIEnv *, const char *, JNINativeMethod *, int);
    void pltHookRegister(dev_t, ino_t, const char *, void *, void **); bool pltHookCommit();
private:
    internal::api_table *tbl;
    template <class T> friend void internal::entry_impl(internal::api_table *, JNIEnv *);
};
#define REGISTER_ZYGISK_MODULE(clazz) \
void zygisk_module_entry(zygisk::internal::api_table *table, JNIEnv *env) { \
    zygisk::internal::entry_impl<clazz>(table, env); }
#define REGISTER_ZYGISK_COMPANION(func) void zygisk_companion_entry(int client) { func(client); }
namespace internal {
struct module_abi {
    long api_version; ModuleBase *impl;
    void (*preAppSpecialize)(ModuleBase *, AppSpecializeArgs *);
    void (*postAppSpecialize)(ModuleBase *, const AppSpecializeArgs *);
    void (*preServerSpecialize)(ModuleBase *, ServerSpecializeArgs *);
    void (*postServerSpecialize)(ModuleBase *, const ServerSpecializeArgs *);
    module_abi(ModuleBase *m) : api_version(ZYGISK_API_VERSION), impl(m) {
        preAppSpecialize=[](auto x, auto a){x->preAppSpecialize(a);};
        postAppSpecialize=[](auto x, auto a){x->postAppSpecialize(a);};
        preServerSpecialize=[](auto x, auto a){x->preServerSpecialize(a);};
        postServerSpecialize=[](auto x, auto a){x->postServerSpecialize(a);};
    }
};
struct api_table {
    void *impl; bool (*registerModule)(api_table *, module_abi *);
    void (*hookJniNativeMethods)(JNIEnv *, const char *, JNINativeMethod *, int);
    void (*pltHookRegister)(dev_t, ino_t, const char *, void *, void **);
    bool (*exemptFd)(int); bool (*pltHookCommit)(); int (*connectCompanion)(void *);
    void (*setOption)(void *, Option); int (*getModuleDir)(void *); uint32_t (*getFlags)(void *);
};
template <class T> void entry_impl(api_table *table, JNIEnv *env) {
    static Api api; api.tbl = table; static T module; ModuleBase *m = &module; static module_abi abi(m);
    if (!table || !table->registerModule || !table->registerModule(table, &abi)) return; m->onLoad(&api, env);
}
}
inline int Api::connectCompanion(){return tbl->connectCompanion?tbl->connectCompanion(tbl->impl):-1;}
inline int Api::getModuleDir(){return tbl->getModuleDir?tbl->getModuleDir(tbl->impl):-1;}
inline void Api::setOption(Option o){if(tbl->setOption)tbl->setOption(tbl->impl,o);}
inline uint32_t Api::getFlags(){return tbl->getFlags?tbl->getFlags(tbl->impl):0;}
inline bool Api::exemptFd(int fd){return tbl->exemptFd!=nullptr&&tbl->exemptFd(fd);}
inline void Api::hookJniNativeMethods(JNIEnv*e,const char*c,JNINativeMethod*m,int n){if(tbl->hookJniNativeMethods)tbl->hookJniNativeMethods(e,c,m,n);}
inline void Api::pltHookRegister(dev_t d,ino_t i,const char*s,void*n,void**o){if(tbl->pltHookRegister)tbl->pltHookRegister(d,i,s,n,o);}
inline bool Api::pltHookCommit(){return tbl->pltHookCommit!=nullptr&&tbl->pltHookCommit();}
}
extern "C" { void zygisk_module_entry(zygisk::internal::api_table *, JNIEnv *); void zygisk_companion_entry(int); }
