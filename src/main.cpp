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

// libminecraftpe.so 베이스 주소 찾기
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

// 직접 메모리 패치를 적용하는 함수 (Dobby 대체)
void patch_memory(uintptr_t address, const std::vector<uint8_t>& patch) {
    size_t page_size = sysconf(_SC_PAGESIZE);
    uintptr_t page_start = address & ~(page_size - 1);
    
    // 1. 메모리에 쓰기 권한 부여
    mprotect((void*)page_start, page_size, PROT_READ | PROT_WRITE | PROT_EXEC);
    
    // 2. 패치 데이터(명령어) 덮어쓰기
    memcpy((void*)address, patch.data(), patch.size());
    
    // 3. ARM64 캐시 비우기 (새 명령어가 즉시 반영되도록 함 - 필수!)
    __builtin___clear_cache((char*)address, (char*)address + patch.size());
    
    // 4. 권한 원상복구
    mprotect((void*)page_start, page_size, PROT_READ | PROT_EXEC);
}

void apply_fixes() {
    uintptr_t mcpeBase = 0;
    while (!(mcpeBase = get_module_base("libminecraftpe.so"))) {
        usleep(100000); // 0.1초씩 기다리며 게임 로딩 대기
    }
    
    // 지인분이 알려주신 타겟 주소
    uintptr_t targetAddr = mcpeBase + 0x961B6A4;
    LOGI("메모리 패치 시작: %p", (void*)targetAddr);

    // 핵심: 원래 있던 함수 호출이나 조건문을 "MOV W0, #1" 로 덮어씌움
    // W0 레지스터(반환값)를 1(true)로 만들어서 애니메이션을 강제 활성화
    std::vector<uint8_t> forceOnPatch = { 0x20, 0x00, 0x80, 0x52 };
    
    patch_memory(targetAddr, forceOnPatch);
    LOGI("패치 성공! 애니메이션이 무조건 켜집니다.");
}

// 라이브러리 로드 시 자동 실행
__attribute__((constructor))
void init() {
    pthread_t t;
    pthread_create(&t, NULL, (void* (*)(void*))apply_fixes, NULL);
}
