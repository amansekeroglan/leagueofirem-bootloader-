DEFAULT REL
SECTION .text

global ASM_PFX(BootMgrCpuPause)
ASM_PFX(BootMgrCpuPause):
    pause
    ret

global ASM_PFX(BootMgrMemoryBarrier)
ASM_PFX(BootMgrMemoryBarrier):
    mfence
    ret

global ASM_PFX(BootMgrReadTsc)
ASM_PFX(BootMgrReadTsc):
    rdtsc
    shl rdx, 32
    or rax, rdx
    ret
