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

// 모듈 베이스 주소를 가져오는 함수
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

// 메모리 보호 권한을 풀고 데이터를 쓰는 함수
void patch_memory(uintptr_t address, const std::vector<uint8_t>& patch) {
    size_t page_size = sysconf(_SC_PAGESIZE);
    uintptr_t page_start = address & ~(page_size - 1);
    
    // 쓰기 권한 부여 (넉넉하게 2페이지 분량)
    mprotect((void*)page_start, page_size * 2, PROT_READ | PROT_WRITE | PROT_EXEC);
    
    // 데이터 복사
    memcpy((void*)address, patch.data(), patch.size());
    
    // CPU 명령 캐시 비우기 (매우 중요)
    __builtin___clear_cache((char*)address, (char*)address + patch.size());
    
    // 권한 복구
    mprotect((void*)page_start, page_size * 2, PROT_READ | PROT_EXEC);
}

void apply_fixes() {
    uintptr_t mcpeBase = 0;
    // libminecraftpe.so가 로드될 때까지 대기
    while (!(mcpeBase = get_module_base("libminecraftpe.so"))) {
        usleep(100000); 
    }
    
    // 패치할 주소 (지인분이 주신 0x961B6A4)
    uintptr_t targetAddr = mcpeBase + 0x961B6A4;
    LOGI("Patching target address: %p", (void*)targetAddr);

    // [해결책] 검은 화면 방지: RET 대신 NOP 사용
    // 0x1F, 0x20, 0x03, 0xD5는 ARM64의 NOP(No Operation) 명령어입니다.
    // 기존의 판단 로직(판치라 애니메이션 체크 등)을 무시하고 자연스럽게 다음 코드로 흐르게 합니다.
    std::vector<uint8_t> forceOnPatch = { 
        0x1F, 0x20, 0x03, 0xD5, // NOP (1번째 명령어 무력화)
        0x1F, 0x20, 0x03, 0xD5  // NOP (2번째 명령어 무력화)
    };
    
    patch_memory(targetAddr, forceOnPatch);
    LOGI("Memory patch applied with NOP. Testing panorama flow...");
}

// 라이브러리 로딩 시 별도 스레드에서 패치 실행
__attribute__((constructor))
void init() {
    pthread_t t;
    pthread_create(&t, NULL, (void* (*)(void*))apply_fixes, NULL);
}
