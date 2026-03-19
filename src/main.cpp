#include <jni.h>
#include <android/log.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>
#include <sys/mman.h>
#include <pthread.h>
#include <vector>
// --- 추가된 헤더 ---
#include <stdio.h>
#include <stdlib.h>
// -----------------

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
    
    uintptr_t funcStart = mcpeBase + 0x961B6A4;

    // --- [테스트 구간] 후보 1번부터 시도합니다 ---
    
    // 후보 1: +60 지점 (98 01 00 B4) 패치
    patch_mem(funcStart + 0x60, 0xD503201F); // NOP 처리
    LOGI("Test Patch 1 (+60) Applied");

    /* 후보 2: 1번이 효과 없으면 나중에 교체해서 테스트
    patch_mem(funcStart + 0x68, 0xD503201F); 
    */

    LOGI("Patch applied. Checking for panorama...");
}

__attribute__((constructor))
void init() {
    pthread_t t;
    pthread_create(&t, NULL, (void* (*)(void*))apply_fixes, NULL);
}
