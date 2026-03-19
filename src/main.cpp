#include <jni.h>
#include <android/log.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>
#include <sys/mman.h>
#include <pthread.h>

#define LOG_TAG "PanoramaMod"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

uintptr_t get_module_base(const char* module_name) {
    uintptr_t addr = 0;
    char line[1024];
    FILE* fp = fopen("/proc/self/maps", "rt");
    if (fp) {
        while (fgets(line, sizeof(line), fp)) {
            if (strstr(line, module_name)) {
                addr = (uintptr_t)strtoul(line, NULL, 16);
                break;
            }
        }
        fclose(fp);
    }
    return addr;
}

void patch_mem(uintptr_t address, uint32_t data) {
    size_t pageSize = sysconf(_SC_PAGESIZE);
    uintptr_t start = address & ~(pageSize - 1);
    mprotect((void*)start, pageSize, PROT_READ | PROT_WRITE | PROT_EXEC);
    *(uint32_t*)address = data;
    mprotect((void*)start, pageSize, PROT_READ | PROT_EXEC);
}

void apply_fixes() {
    uintptr_t mcpeBase = 0;
    while (!(mcpeBase = get_module_base("libminecraftpe.so"))) {
        usleep(500000);
    }
    
    // 입구(0x961B6A4)는 절대 건드리지 않습니다! (크래시 원인)
    uintptr_t funcStart = mcpeBase + 0x961B6A4;

    // --- [테스트 구간] 아래 3개 중 하나씩 주석을 해제하며 확인해보세요 ---
    
    // 후보 1: +60 지점 스위치 끄기
    patch_mem(funcStart + 0x60, 0xD503201F); // NOP
    LOGI("Test Patch 1 (+60) Applied");

    /* 후보 2: +68 지점 스위치 끄기 (1번이 안되면 1번을 다시 주석처리하고 이걸 해제)
    patch_mem(funcStart + 0x68, 0xD503201F); // NOP
    LOGI("Test Patch 2 (+68) Applied");
    */

    /* 후보 3: +78 지점 스위치 끄기
    patch_mem(funcStart + 0x78, 0xD503201F); // NOP
    LOGI("Test Patch 3 (+78) Applied");
    */

    LOGI("Patch session complete. Check the main screen!");
}

__attribute__((constructor))
void init() {
    pthread_t t;
    pthread_create(&t, NULL, (void* (*)(void*))apply_fixes, NULL);
}
