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

// 모듈의 베이스 주소를 찾는 함수
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

// 메모리 패치 함수 (권한 변경 포함)
void patch_mem(uintptr_t address, uint32_t data) {
    size_t pageSize = sysconf(_SC_PAGESIZE);
    uintptr_t start = address & ~(pageSize - 1);
    
    // 쓰기 권한 부여
    mprotect((void*)start, pageSize, PROT_READ | PROT_WRITE | PROT_EXEC);
    
    // 데이터 쓰기 (4바이트 명령어)
    *(uint32_t*)address = data;
    
    // 권한 복구
    mprotect((void*)start, pageSize, PROT_READ | PROT_EXEC);
}

void apply_fixes() {
    uintptr_t mcpeBase = 0;
    
    // 라이브러리가 로드될 때까지 대기
    while (!(mcpeBase = get_module_base("libminecraftpe.so"))) {
        usleep(500000);
    }
    
    // 지인분이 주신 함수 시작 주소
    uintptr_t funcStart = mcpeBase + 0x961B6A4;

    /* [분석 결과 적용]
       디컴파일 코드에서 'goto LAB_0961b7ac'를 유발하는 
       +78 지점의 CBZ 명령어를 NOP(D503201F)으로 변경합니다.
    */
    
    // 1순위 타겟: 파노라마 로직을 통째로 점프하는 조건문 무력화
    patch_mem(funcStart + 0x78, 0xD503201F); 
    
    // 2순위 타겟 (선택 사항): 혹시 모를 다른 점프문들도 함께 무력화
    patch_mem(funcStart + 0x60, 0xD503201F);
    patch_mem(funcStart + 0x68, 0xD503201F);

    LOGI("Final Panorama Fix Applied at +60, +68, +78!");
}

// 라이브러리 로드 시 자동 실행
__attribute__((constructor))
void init() {
    pthread_t t;
    pthread_create(&t, NULL, (void* (*)(void*))apply_fixes, NULL);
}
