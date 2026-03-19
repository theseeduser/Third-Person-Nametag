void apply_fixes() {
    uintptr_t mcpeBase = 0;
    while (!(mcpeBase = get_module_base("libminecraftpe.so"))) {
        usleep(100000); 
    }
    
    uintptr_t targetAddr = mcpeBase + 0x961B6A4;
    LOGI("Patching target address: %p", (void*)targetAddr);

    // [수정] 딱 4바이트(명령어 1개)만 NOP으로 만듭니다.
    // 만약 이 자리가 "애니메이션 꺼졌으면 점프해라(CBZ 등)" 였다면 
    // 점프를 안 하고 그대로 파노라마를 실행하게 됩니다.
    std::vector<uint8_t> forceOnPatch = { 
        0x1F, 0x20, 0x03, 0xD5  // NOP (딱 1줄만 무력화)
    };
    
    patch_memory(targetAddr, forceOnPatch);
    LOGI("4-byte NOP Patch Applied! Testing...");
}
