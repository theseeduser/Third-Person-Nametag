void apply_fixes() {
    uintptr_t mcpeBase = 0;
    while (!(mcpeBase = get_module_base("libminecraftpe.so"))) {
        usleep(100000); 
    }
    
    uintptr_t targetAddr = mcpeBase + 0x961B6A4;
    LOGI("메모리 패치 시작: %p", (void*)targetAddr);

    // MOV W0, #1 (true 반환) + RET (함수 종료 후 복귀)
    std::vector<uint8_t> forceOnPatch = { 
        0x20, 0x00, 0x80, 0x52, // MOV W0, #1
        0xC0, 0x03, 0x5F, 0xD6  // RET
    };
    
    patch_memory(targetAddr, forceOnPatch);
    LOGI("패치 성공! 애니메이션이 켜지고 안전하게 반환됩니다.");
}
