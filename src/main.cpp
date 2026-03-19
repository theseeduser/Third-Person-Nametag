#include <jni.h>
#include <android/log.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>
#include <sys/mman.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

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

    // [전략 1] 모든 탈출 조건문(CBZ)을 NOP으로 제거 (길을 뚫음)
    patch_mem(funcStart + 0x60, 0xD503201F); // CBZ X24 -> NOP
    patch_mem(funcStart + 0x68, 0xD503201F); // CBZ W8  -> NOP
    patch_mem(funcStart + 0x78, 0xD503201F); // CBZ X21 -> NOP

    // [전략 2] 함수의 리턴값을 강제로 '활성화(1)' 상태로 고정
    // 함수 끝부분의 return unaff_w19 | bVar1; 로직을 MOV W0, #1; RET; 로 덮어씌울 수 있지만
    // 일단은 위 패치만으로 메인 메뉴 진입 후를 확인해야 합니다.

    LOGI("Patches applied at +60, +68, +78. Please enter the MAIN MENU.");
}

__attribute__((constructor))
void init() {
    pthread_t t;
    pthread_create(&t, NULL, (void* (*)(void*))apply_fixes, NULL);
}
