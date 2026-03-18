#include <jni.h>
#include <android/log.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/mman.h>
#include <vector>
#include <string>
#include <fstream>
#include <sstream>

#define LOG_TAG "MCPE_MOD"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

// libminecraftpe.so의 베이스 주소를 찾는 함수
uintptr_t GetModuleBase(const std::string& moduleName) {
    uintptr_t baseAddress = 0;
    std::ifstream maps("/proc/self/maps");
    std::string line;
    while (std::getline(maps, line)) {
        if (line.find(moduleName) != std::string::npos) {
            std::istringstream iss(line);
            std::string addressRange;
            iss >> addressRange;
            size_t dashPos = addressRange.find('-');
            if (dashPos != std::string::npos) {
                std::string startAddr = addressRange.substr(0, dashPos);
                baseAddress = std::stoul(startAddr, nullptr, 16);
                break;
            }
        }
    }
    return baseAddress;
}

// 메모리 패치를 수행하는 함수
void PatchMemory(uintptr_t address, const std::vector<uint8_t>& patchBytes) {
    // 메모리 페이지 크기에 맞춰 정렬된 주소 계산
    size_t pageSize = sysconf(_SC_PAGESIZE);
    uintptr_t pageStart = address & ~(pageSize - 1);

    // 메모리 보호 해제 (읽기, 쓰기, 실행 권한 부여)
    if (mprotect((void*)pageStart, pageSize, PROT_READ | PROT_WRITE | PROT_EXEC) != 0) {
        LOGE("메모리 권한 변경 실패! 주소: %x", address);
        return;
    }

    // 바이트 덮어쓰기 (패치 적용)
    for (size_t i = 0; i < patchBytes.size(); i++) {
        *(uint8_t*)(address + i) = patchBytes[i];
    }

    LOGI("메모리 패치 성공! 대상 주소: %x", address);

    // (선택사항) 권한을 원래대로 복구하려면 다시 mprotect 호출
}

// 백그라운드 스레드에서 패치를 진행하는 함수
void* ModMainThread(void*) {
    LOGI("모드 스레드 시작됨...");

    uintptr_t mcpeBase = 0;
    // libminecraftpe.so가 메모리에 로드될 때까지 대기
    while ((mcpeBase = GetModuleBase("libminecraftpe.so")) == 0) {
        sleep(1);
    }

    LOGI("libminecraftpe.so 베이스 주소 발견: %x", mcpeBase);

    // 지인분이 알려주신 오프셋
    uintptr_t targetOffset = 0x961B6A4;
    
    // [핵심] 0x961B6A4는 함수의 시작점입니다. 
    // 실제 분기문(if 애니메이션 꺼짐)의 오프셋을 Ghidra로 찾아 이 주소에 더해야 합니다.
    // 임시로 함수 시작점 + 분기문까지의 거리(예: 0x2A)라고 가정하겠습니다.
    uintptr_t exactPatchAddress = mcpeBase + targetOffset + /* 분기문 거리 */ 0x00; 

    // 덮어씌울 헥스 코드 (예: ARM64 NOP 코드 -> 1F 20 03 D5)
    // 이 부분은 기계어(Assembly)에 따라 달라집니다.
    std::vector<uint8_t> patchHex = { 0x1F, 0x20, 0x03, 0xD5 }; 

    // 패치 적용
    PatchMemory(exactPatchAddress, patchHex);

    return nullptr;
}

// 안드로이드에서 라이브러리가 로드될 때 최초로 실행되는 진입점
JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM* vm, void* reserved) {
    LOGI("모드 라이브러리가 로드되었습니다!");

    // 메인 스레드를 멈추지 않기 위해 새로운 스레드에서 모드 로직 실행
    pthread_t thread;
    pthread_create(&thread, nullptr, ModMainThread, nullptr);

    return JNI_VERSION_1_6;
}
