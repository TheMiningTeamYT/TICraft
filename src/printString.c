#include <graphx.h>
#include <ti/screen.h>
#include <sys/lcd.h>

void fontPrintString(const char* string, uint16_t row) {
    os_FontDrawText(string, (LCD_WIDTH - os_FontGetWidth(string))>>1, row);
}

void printStringCentered(const char* string, int row) {
    unsigned int width = gfx_GetStringWidth(string);
    gfx_PrintStringXY(string, 160-(width/2), row);
}
void printStringAndMoveDownCentered(const char* string) {
    unsigned int width = gfx_GetStringWidth(string);
    gfx_SetTextXY(160-(width/2), gfx_GetTextY());
    gfx_PrintString(string);
    gfx_SetTextXY(0, gfx_GetTextY() + 10);
}