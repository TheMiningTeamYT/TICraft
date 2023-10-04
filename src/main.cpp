#include <graphx.h>
#include <ti/getcsc.h>
#include <sys/power.h>
#include <cstdint>
#include <cstdio>
#include "renderer.hpp"
#include "textures.hpp"

int main() {
    boot_Set48MHzMode();
    point cubes[] = {{0, 0, 0}, {1, 0, 0}, {2, 0, 0}, {0, 0, 1}, {1, 0, 1}, {2, 0, 1}, {0, 0, 2}, {1, 0, 2}, {2, 0, 2}, {1, 1, 1}};
    gfx_Begin();
    gfx_SetTextXY(0, 0);
    gfx_SetTextFGColor(0);
    gfx_SetTextBGColor(255);
    gfx_SetTextScale(1, 1);
    initPalette();
    for (int i = 0; i < 20; i++) {
        if (numberOfObjects < maxNumberOfObjects) {
            const uint8_t** texture = textures[i%4];
            objects[numberOfObjects] = new object((i%5)*50, 0, (i/5)*50, 50, texture);
            numberOfObjects++;
        }
    }
    /*for (uint8_t i = 0; i < 10; i++) {
        if (numberOfObjects < maxNumberOfObjects) {
            const uint8_t** texture = crafting_table_texture;
            const point cubePoint = cubes[i];
            objects[numberOfObjects] = new object(cubePoint.x * (Fixed24)50, cubePoint.y*(Fixed24)50, cubePoint.z*(Fixed24)50, 50, texture);
            numberOfObjects++;
        }
    }*/
    #if showDraw == false
    gfx_SetDrawBuffer();
    #endif
    while(!os_GetCSC()) {
        drawScreen();
    };
    deleteEverything();
    gfx_End();
}