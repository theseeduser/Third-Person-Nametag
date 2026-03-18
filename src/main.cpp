#include <jni.h>
#include <android/log.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>
#include <sys/mman.h>
#include <pthread.h>
#include <vector>

#define LOG_TAG "PanoramaMod"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

uintptr_t get_module_base(const char* module_name) {
    uintptr_t addr = 0;
    char line[1024];
    FILE* fp = fopen("/proc/self/maps", "rt");
    if (fp) {
        while (fgets(line, sizeof(line), fp)) {
            if (strstr(line, module_name)) {
                addr = strtoull(line, NULL, 16);
                break;
            }
        }
        fclose(fp);
    }
    return addr;
}

void patch_memory(uintptr_t address, const std::vector<uint8_t>& patch) {
    size_t page_size = sysconf(_SC_PAGESIZE);
    uintptr_t page_start = address & ~(page_size - 1);

    mprotect((void*)page_start, page_size * 2, PROT_READ | PROT_WRITE | PROT_EXEC);

    memcpy((void*)address, patch.data(), patch.size());

    __builtin___clear_cache((char*)address, (char*)address + patch.size());

    mprotect((void*)page_start, page_size * 2, PROT_READ | PROT_EXEC);
}

void apply_patch() {
    uintptr_t base = 0;

    while (!(base = get_module_base("libminecraftpe.so"))) {
        usleep(100000);
    }

    LOGI("Base: %p", (void*)base);

    // 🔥 여기 중요
    uintptr_t target = base + 0x961B6A4;

    // ❌ RET 패치 (쓰면 안됨)
    // ✔ 대신 → 분기 무력화

    // ARM64 NOP (1개 = 4바이트)
    std::vector<uint8_t> patch = {
        0x1F, 0x20, 0x03, 0xD5,  // NOP
        0x1F, 0x20, 0x03, 0xD5   // NOP
    };

    patch_memory(target, patch);

    LOGI("NOP Patch Applied");
}

__attribute__((constructor))
void init() {
    pthread_t t;
    pthread_create(&t, nullptr, (void* (*)(void*))apply_patch, nullptr);
}
