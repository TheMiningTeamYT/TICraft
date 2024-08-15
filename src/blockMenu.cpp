#include <cstdlib>
#include <graphx.h>
#include "cursor.hpp"
#include "textures.hpp"
#include "blockMenu.hpp"

void drawSelection(int offset) {
    int x = 80 + (20*(selectedObject%8));
    int y = 50 + (24*(selectedObject/8));
    gfx_SetColor(253);
    memcpy(cursorBackground->data, textures[selectedObject][1], 256);
    gfx_FillRectangle(x, y, 20, 20);
    gfx_Sprite_NoClip(cursorBackground, x + 2, y + 2);
    gfx_BlitRectangle(gfx_buffer, x, y, 20, 20);
    selectedObject += offset;
    x = 80 + (20*(selectedObject%8));
    y = 50 + (24*(selectedObject/8));
    gfx_SetColor(254);
    memcpy(cursorBackground->data, textures[selectedObject][1], 256);
    gfx_FillRectangle(x, y, 20, 20);
    gfx_Sprite_NoClip(cursorBackground, x + 2, y + 2);
    gfx_BlitRectangle(gfx_buffer, x, y, 20, 20);
}

void selectBlock() {
    drawBuffer();
    shadeScreen();
    gfx_SetDrawBuffer();
    cursorBackground->width = 16;
    cursorBackground->height = 16;
    gfx_SetColor(253);
    gfx_FillRectangle(72, 42, 176, 156);
    for (uint8_t i = 0; i < 48; i++) {
        if (i == selectedObject) {
            gfx_SetColor(254);
            gfx_FillRectangle(80 + (20*(i%8)), 50 + (24*(i/8)), 20, 20);
        }
        memcpy(cursorBackground->data, textures[i][1], 256);
        gfx_Sprite_NoClip(cursorBackground, 82 + (20*(i%8)), 52 + (24*(i/8)));
    }
    gfx_BlitBuffer();
    bool quit = false;
    while (!quit) {
        os_ResetFlag(SHIFT, ALPHALOCK);
        switch (os_GetKey()) {
            case k_Up:
            case k_8:
                if (selectedObject/8 > 0) {
                    drawSelection(-8);
                } else {
                    drawSelection(40);
                }
                break;
            case k_Down:
            case k_2:
                if (selectedObject/8 < 5) {
                    drawSelection(8);
                } else {
                    drawSelection(-40);
                }
                break;
            case k_Left:
            case k_4:
                if (selectedObject > 0) {
                    drawSelection(-1);
                } else {
                    drawSelection(((int)-selectedObject) + 47);
                }
                break;
            case k_Right:
            case k_6:
                if (selectedObject < 47) {
                    drawSelection(1);
                } else {
                    drawSelection(((int)-selectedObject));
                }
                break;
            case k_Enter:
                quit = true;
                break;
            case k_Clear:
                quit = true;
                break;
            default:
                break;
        }
    }
    gfx_SetDrawScreen();
    gfx_palette[255] = texPalette[255];
    drawScreen();
}