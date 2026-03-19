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
    
    uintptr_t target = mcpeBase + 0x961B6A4;

    /* [최종 병기: Force Return 1]
       함수의 입구에서 바로 1을 반환하고 나가게 만듭니다.
       이래도 안 된다면 이 함수는 파노라마와 상관없는 함수일 확률이 99%입니다.
    */
    
    // 1. MOV W0, #1  (결과값을 1로 설정)
    patch_mem(target + 0x0, 0x20008052); 
    // 2. RET         (함수 즉시 종료)
    patch_mem(target + 0x4, 0xC0035FD6); 

    LOGI("Force Return Patch Applied. 만약 메인 메뉴에서도 변화가 없다면 주소가 틀린 것입니다.");
}

__attribute__((constructor))
void init() {
    pthread_t t;
    pthread_create(&t, NULL, (void* (*)(void*))apply_fixes, NULL);
}
