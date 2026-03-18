#include <jni.h>
#include "dobby.h"
#include <android/log.h>
#include <unistd.h>
#include <stdint.h>
#include <string>
#include <fstream>
#include <sstream>

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

// 원래 함수 저장용
bool (*old_isAnimationEnabled)(void* self);

// 우리가 바꾼 함수: 무조건 true(1) 반환
bool new_isAnimationEnabled(void* self) {
    // LOGI("애니메이션 체크 함수가 호출됨! 강제로 true 반환.");
    return true; 
}

void install_hooks() {
    uintptr_t mcpeBase = 0;
    while (!(mcpeBase = get_module_base("libminecraftpe.so"))) {
        usleep(100000); // 로드될 때까지 대기
    }

    // 지인분이 알려주신 오프셋
    uintptr_t targetAddr = mcpeBase + 0x961B6A4;

    LOGI("후킹 시작: %p", (void*)targetAddr);

    // DobbyHook을 사용하여 함수를 가로챕니다.
    DobbyHook((void *)targetAddr, (void *)new_isAnimationEnabled, (void **)&old_isAnimationEnabled);
}

// 라이브러리가 로드될 때 자동 실행
__attribute__((constructor))
void init() {
    pthread_t t;
    pthread_create(&t, NULL, (void* (*)(void*))install_hooks, NULL);
}
