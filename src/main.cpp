#include <jni.h>
#include <android/log.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>
#include <sys/mman.h>
#include <pthread.h>
#include <vector>
#include <stdio.h>

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

// 분석용 메모리 덤프 함수
void dump_instructions(uintptr_t address, size_t size) {
    uint8_t buffer[32];
    memcpy(buffer, (void*)address, size);
    
    char hex_out[128] = {0};
    for(size_t i=0; i<size; i++) {
        sprintf(hex_out + strlen(hex_out), "%02X ", buffer[i]);
    }
    LOGI("Address %p Hex: %s", (void*)address, hex_out);
}

void apply_fixes() {
    uintptr_t mcpeBase = 0;
    while (!(mcpeBase = get_module_base("libminecraftpe.so"))) {
        usleep(500000);
    }
    
    uintptr_t targetAddr = mcpeBase + 0x961B6A4;
    LOGI("Analyzing Panorama Function at: %p", (void*)targetAddr);

    // [중요] 일단 튕기지 않게 패치는 하지 않고 내부 코드만 로그로 찍습니다.
    dump_instructions(targetAddr, 32);
    
    LOGI("분석 로그 출력 완료. 기드라 화면이나 위 Hex 값을 알려주세요!");
}

__attribute__((constructor))
void init() {
    pthread_t t;
    pthread_create(&t, NULL, (void* (*)(void*))apply_fixes, NULL);
}
