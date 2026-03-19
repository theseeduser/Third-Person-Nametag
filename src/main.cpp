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

// [중요] 함수 내부를 128바이트만큼 덤프하여 분석합니다.
void dump_panorama_logic(uintptr_t address) {
    uint8_t buffer[128];
    memcpy(buffer, (void*)address, 128);
    
    LOGI("--- Panorama Function Hex Dump (128 bytes) ---");
    for (int i = 0; i < 128; i += 16) {
        char hex_line[128] = {0};
        sprintf(hex_line, "+%02X: %02X %02X %02X %02X | %02X %02X %02X %02X | %02X %02X %02X %02X | %02X %02X %02X %02X",
                i,
                buffer[i], buffer[i+1], buffer[i+2], buffer[i+3],
                buffer[i+4], buffer[i+5], buffer[i+6], buffer[i+7],
                buffer[i+8], buffer[i+9], buffer[i+10], buffer[i+11],
                buffer[i+12], buffer[i+13], buffer[i+14], buffer[i+15]);
        LOGI("%s", hex_line);
    }
    LOGI("--- End of Dump ---");
}

void apply_fixes() {
    uintptr_t mcpeBase = 0;
    while (!(mcpeBase = get_module_base("libminecraftpe.so"))) {
        usleep(500000);
    }
    
    uintptr_t targetAddr = mcpeBase + 0x961B6A4;
    LOGI("Analyzing target at: %p", (void*)targetAddr);

    // 패치는 하지 않고 데이터만 읽어옵니다. (크래시 방지)
    dump_panorama_logic(targetAddr);
    
    LOGI("분석 완료! 로그캣의 Hex 데이터를 복사해서 알려주세요.");
}

__attribute__((constructor))
void init() {
    pthread_t t;
    pthread_create(&t, NULL, (void* (*)(void*))apply_fixes, NULL);
}
