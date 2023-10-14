#include <graphx.h>

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