#include <cstring>
#include <graphx.h>
#include <ti/screen.h>
#include <sys/power.h>

uint8_t interruptOriginalValue;

void exitOverlay() {
    gfx_End();
    // Restore RTC interrupts
    *((uint8_t*)0xF00005) = interruptOriginalValue;
    // APD = Automatic Power Down (AFAIK)
    os_EnableAPD();
    // clear out pixel shadow
    memset((void*) 0xD031F6, 0, 69090);
    os_ClrHomeFull();
    exit(0);
};