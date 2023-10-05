#include <graphx.h>
#include <ti/getcsc.h>
#include <sys/power.h>
#include <cstdint>
#include <cstdio>
#include "renderer.hpp"
#include "textures.hpp"

uint8_t* cursorBackgroundBuffer;
gfx_sprite_t* cursorBackground = (gfx_sprite_t*) cursorBackgroundBuffer;

object playerCursor(0, 0, 0, 20, cursor_texture, true);
int playerCursorX;
int playerCursorY;

void drawCursor();
void moveCursor(uint8_t direction);

int main() {
    boot_Set48MHzMode();
    point cubes[] = {{0, 0, 0}, {1, 0, 0}, {2, 0, 0}, {0, 0, 1}, {1, 0, 1}, {2, 0, 1}, {0, 0, 2}, {1, 0, 2}, {2, 0, 2}, {1, 1, 1}};
    gfx_Begin();
    gfx_SetTextXY(0, 0);
    gfx_SetTextFGColor(0);
    gfx_SetTextBGColor(255);
    gfx_SetTextScale(1, 1);
    initPalette();
    gfx_SetColor(0);
    for (int i = 0; i < 50; i++) {
        if (numberOfObjects < maxNumberOfObjects) {
            const uint8_t** texture = textures[i%4];
            objects[numberOfObjects] = new object((i%10)*20, 0, (i/10)*20, 20, texture, false);
            numberOfObjects++;
        }
    }
    /*for (uint8_t i = 0; i < 200; i++) {
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
    drawScreen(false);
    gfx_SetDrawScreen();
    drawCursor();
    bool quit = false;
    while (!quit) {
        uint8_t key = os_GetCSC();
        if (key) {
            if (key == sk_9) {
                moveCursor(4);
            } else if (key == sk_7) {
                moveCursor(2);
            } else if (key == sk_3) {
                moveCursor(3);
            } else if (key == sk_1) {
                moveCursor(5);
            } else if (key == sk_Mul) {
                moveCursor(0);
            } else if (key == sk_Sub) {
                moveCursor(1);
            } else if (key == sk_0) {
                quit = true;
            }
        }
    }
    deleteEverything();
    gfx_End();
}

// still has problems
void drawCursor() {
    deletePolygons();
    playerCursor.generatePolygons(false);
    if (playerCursor.visible) {
        if (cursorBackgroundBuffer) {
            delete[] cursorBackgroundBuffer;
        }
        int minX = playerCursor.renderedPoints[0].x;
        int minY = playerCursor.renderedPoints[0].x;
        int maxX = playerCursor.renderedPoints[0].x;
        int maxY = playerCursor.renderedPoints[0].x;
        for (uint8_t i = 1; i < 8; i++) {
            if (playerCursor.renderedPoints[i].x < minX) {
                minX = playerCursor.renderedPoints[i].x;
            } else if (playerCursor.renderedPoints[i].x > maxX) {
                maxX = playerCursor.renderedPoints[i].x;
            }
            if (playerCursor.renderedPoints[i].y < minY) {
                minY = playerCursor.renderedPoints[i].y;
            } else if (playerCursor.renderedPoints[i].y > maxY) {
                maxY = playerCursor.renderedPoints[i].y;
            }
        }
        minX -= 4;
        minY -= 4;
        int width = (maxX-minX) + 4;
        int height = (maxY-minY) + 4;
        /*if (cursorBackgroundBuffer) {
            delete[] cursorBackgroundBuffer;
        }*/
        cursorBackgroundBuffer = new uint8_t[width*height + 2];
        cursorBackground = (gfx_sprite_t*) cursorBackgroundBuffer;
        cursorBackground->width = width;
        cursorBackground->height = height;
        playerCursorX = minX;
        playerCursorY = minY;
        gfx_GetSprite(cursorBackground, minX, minY);
        drawScreen(true);
    }
}
/*
0: up
1: down
2: left
3: right
4: forwards
5: backwards
*/
void moveCursor(uint8_t direction) {
    if (direction == 0) {
        playerCursor.moveBy(0, 20, 0);
    } else if (direction == 1) {
        playerCursor.moveBy(0, -20, 0);
    } else if (direction == 2) {
        playerCursor.moveBy(0, 0, 20);
    } else if (direction == 3) {
        playerCursor.moveBy(0, 0, -20);
    } else if (direction == 4) {
        playerCursor.moveBy(20, 0, 0);
    } else if (direction == 5) {
        playerCursor.moveBy(-20, 0, 0);
    } else {
        return;
    }
    if (playerCursorX > 0 && cursorBackground->width + playerCursorX < 320 && playerCursorY > 0 && cursorBackground->height + playerCursorY < 240) {
        gfx_Sprite_NoClip(cursorBackground, playerCursorX, playerCursorY);
    } else {
        gfx_Sprite(cursorBackground, playerCursorX, playerCursorY);
    }
    drawCursor();
}