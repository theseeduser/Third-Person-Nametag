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

// ... (위의 헤더와 get_module_base 함수는 동일하게 유지)

void apply_fixes() {
    uintptr_t mcpeBase = 0;
    while (!(mcpeBase = get_module_base("libminecraftpe.so"))) {
        usleep(500000);
    }
    
    uintptr_t funcStart = mcpeBase + 0x961B6A4;

    // [전략] 의심되는 모든 '건너뛰기'를 한꺼번에 무력화합니다.
    
    // 후보 1: CBZ X24 (0이면 점프) 무력화
    patch_mem(funcStart + 0x60, 0xD503201F); 
    
    // 후보 2: CBZ W8 (0이면 점프) 무력화
    patch_mem(funcStart + 0x68, 0xD503201F); 
    
    // 후보 3: CBZ X21 (0이면 점프) 무력화 -> 새로 추가
    patch_mem(funcStart + 0x78, 0xD503201F); 

    // 후보 4: 만약 중간에 강제 리턴(RET)이 있다면? 
    // 이 지점 근처에 0x00이나 다른 값으로 리턴하는 구간이 있을 수 있습니다.
    
    LOGI("All potential switches (+60, +68, +78) patched at once.");
}
