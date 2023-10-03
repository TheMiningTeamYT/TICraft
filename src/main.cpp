#include <graphx.h>
#include <ti/getcsc.h>
#include <sys/power.h>
#include <cstdint>
#include <cstdio>
#include "renderer.hpp"
#include "textures.hpp"

int main() {
    boot_Set48MHzMode();
    gfx_Begin();
    /*point points[] = {{0, 1, 1}, {0, 1, 1}, {0, 1, 1}, {0, 1, 1}, {0, 1, 1}, {0, 1, 1}, {0, 1, 1}, {0, 1, 1}};
    unsigned int polygonPoints[] = {0, 1, 2, 3};
    polygon polygons[] = {{polygonPoints, 1}};
    object testObject(points, 8, polygons, 1);
    testObject.generatePolygons(); */
    gfx_SetTextXY(0, 0);
    gfx_SetTextFGColor(0);
    gfx_SetTextBGColor(255);
    gfx_SetTextScale(1, 1);
    initPalette();
    for (int i = 0; i < 20; i++) {
        if (numberOfObjects < maxNumberOfObjects) {
            objects[numberOfObjects] = new object((i%5)*50, 0, (i/5)*50, 50, crafting_table_texture);
            numberOfObjects++;
        }
    }
    gfx_SetDrawBuffer();
    while(!os_GetCSC()) {
        drawScreen();
    };
    deleteEverything();
    gfx_End();
}