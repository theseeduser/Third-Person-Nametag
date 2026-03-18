#include <jni.h>
#include <android/log.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>
#include <pthread.h>
#include <dlfcn.h>
#include <dobby.h>

#define LOG_TAG "PanoramaMod"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

typedef void (*CubemapFunc)(void*);
CubemapFunc orig_Cubemap = nullptr;

void hook_Cubemap(void* thiz) {
    // 원래 함수 실행
    orig_Cubemap(thiz);

    // 🔥 핵심: 항상 업데이트 상태 유지
    *(char*)((uintptr_t)thiz + 0x97c) = 1;
}

uintptr_t get_module_base(const char* name) {
    uintptr_t addr = 0;
    char line[1024];
    FILE* fp = fopen("/proc/self/maps", "r");
    if (fp) {
        while (fgets(line, sizeof(line), fp)) {
            if (strstr(line, name)) {
                addr = strtoull(line, NULL, 16);
                break;
            }
        }
        fclose(fp);
    }
    return addr;
}

void* main_thread(void*) {
    uintptr_t base = 0;

    while (!(base = get_module_base("libminecraftpe.so"))) {
        usleep(100000);
    }

    LOGI("Base: %p", (void*)base);

    void* target = (void*)(base + 0x961B6A4);

    if (DobbyHook(target, (void*)hook_Cubemap, (void**)&orig_Cubemap) == RT_SUCCESS) {
        LOGI("Hook success");
    } else {
        LOGI("Hook failed");
    }

    return nullptr;
}

__attribute__((constructor))
void init() {
    pthread_t t;
    pthread_create(&t, nullptr, main_thread, nullptr);
}
