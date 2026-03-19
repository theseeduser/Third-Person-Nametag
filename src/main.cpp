#include <jni.h>
#include <android/log.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>
#include <sys/mman.h>
#include <pthread.h>
#include <vector>
#include <iomanip>
#include <sstream>

#define LOG_TAG "PanoramaMod"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

// 모듈 베이스 주소 찾기
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

// 메모리 패치 함수
void patch_memory(uintptr_t address, const std::vector<uint8_t>& patch) {
    size_t page_size = sysconf(_SC_PAGESIZE);
    uintptr_t page_start = address & ~(page_size - 1);
    
    mprotect((void*)page_start, page_size, PROT_READ | PROT_WRITE | PROT_EXEC);
    
    // [로그 기록] 패치 전 원래 데이터를 로그로 출력 (매우 중요!)
    uint8_t old_data[4];
    memcpy(old_data, (void*)address, 4);
    LOGI("Original bytes at %p: %02X %02X %02X %02X", 
         (void*)address, old_data[0], old_data[1], old_data[2], old_data[3]);
    
    // 데이터 덮어쓰기
    memcpy((void*)address, patch.data(), patch.size());
    
    __builtin___clear_cache((char*)address, (char*)address + patch.size());
    mprotect((void*)page_start, page_size, PROT_READ | PROT_EXEC);
}

void apply_fixes() {
    uintptr_t mcpeBase = 0;
    // 게임이 완전히 로드될 때까지 약간 더 여유있게 대기
    while (!(mcpeBase = get_module_base("libminecraftpe.so"))) {
        usleep(500000); // 0.5초 대기
    }
    
    // 지인분이 주신 주소
    uintptr_t targetAddr = mcpeBase + 0x961B6A4;
    LOGI("Patching started at: %p", (void*)targetAddr);

    // [시도] MOV W0, #1 (결과값을 true로 강제 설정하되 함수는 유지)
    // 이 방법이 실패하면(크래시) 해당 주소가 명령어가 아닌 데이터 영역일 수 있습니다.
    std::vector<uint8_t> forceOnPatch = { 0x20, 0x00, 0x80, 0x52 }; 
    
    patch_memory(targetAddr, forceOnPatch);
    LOGI("Patch applied. Please check if it still crashes at 98%%.");
}

__attribute__((constructor))
void init() {
    pthread_t t;
    pthread_create(&t, NULL, (void* (*)(void*))apply_fixes, NULL);
}
