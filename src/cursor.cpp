#include <cstdint>
#include <graphx.h>
#include "renderer.hpp"
#include "cursor.hpp"

uint8_t selectedObject = 10;
object playerCursor(20, 20, 20, selectedObject, true);
gfx_sprite_t* cursorBackground;
int playerCursorX = 0;
int playerCursorY = 0;

extern "C" {
    int ti_MemChk();
}

void drawCursor() {
    __asm__ ("di");
    playerCursor.generatePoints();
    drawBuffer();
    if (playerCursor.visible) {
        int minX = renderedPoints[0].x;
        int minY = renderedPoints[0].y;
        int maxX = renderedPoints[0].x;
        int maxY = renderedPoints[0].y;
        for (uint8_t i = 1; i < 8; i++) {
            if (renderedPoints[i].x < minX) {
                minX = renderedPoints[i].x;
            } else if (renderedPoints[i].x > maxX) {
                maxX = renderedPoints[i].x;
            }
            if (renderedPoints[i].y < minY) {
                minY = renderedPoints[i].y;
            } else if (renderedPoints[i].y > maxY) {
                maxY = renderedPoints[i].y;
            }
        }
        minX--;
        minY--;
        int width = (maxX-minX) + 1;
        int height = (maxY-minY) + 1;
        if (width < 256 && height < 256 && width*height + 2 <= ti_MemChk()) {
            cursorBackground->width = width;
            cursorBackground->height = height;
            playerCursorX = minX;
            playerCursorY = minY;
            getBuffer();
        }
        playerCursor.generatePolygons();
    }
}

/*
0: up
1: down
2: left
3: right
4: forwards
5: backwards
6: diagonally left
7: diagonally right
8: diagonally forward
9: diagonally backward
*/
void moveCursor(uint8_t direction) {
    bool visibleBefore = playerCursor.visible;
    switch (direction) {
        case 0:
            playerCursor.moveBy(0, 20, 0);
            break;
        case 1:
            playerCursor.moveBy(0, -20, 0);
            break;
        case 2:
            playerCursor.moveBy(0, 0, 20);
            break;
        case 3:
            playerCursor.moveBy(0, 0, -20);
            break;
        case 4:
            playerCursor.moveBy(20, 0, 0);
            break;
        case 5:
            playerCursor.moveBy(-20, 0, 0);
            break;
        case 6:
            playerCursor.moveBy(-20, 0, 20);
            break;
        case 7:
            playerCursor.moveBy(20, 0, -20);
            break;
        case 8:
            playerCursor.moveBy(20, 0, 20);
            break;
        case 9:
            playerCursor.moveBy(-20, 0, -20);
            break;
        default:
            return;
    }
    drawCursor();
}

void getBuffer() {
    gfx_GetSprite(cursorBackground, playerCursorX, playerCursorY);
}

void drawBuffer() {
    gfx_Sprite(cursorBackground, playerCursorX, playerCursorY);
}