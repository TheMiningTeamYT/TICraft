#include <graphx.h>
#include <ti/getcsc.h>
#include <sys/power.h>
#include <cstdint>
#include <cstdio>
#include "renderer.hpp"
#include "textures.hpp"

uint8_t* cursorBackgroundBuffer;
gfx_sprite_t* cursorBackground = (gfx_sprite_t*) cursorBackgroundBuffer;
char buffer2[200];
uint8_t selectedObject = 14;

object playerCursor(20, 20, 40, 20, textures[selectedObject], true);
int playerCursorX;
int playerCursorY;

void drawCursor();
void getBuffer();
void drawBuffer();
void printStringCentered(const char* string, int row);
void printStringAndMoveDownCentered(const char* string);
void moveCursor(uint8_t direction);
void selectBlock();
void redrawSelectedObject();
void selectNewObject();

int main() {
    boot_Set48MHzMode();
    point cubes[] = {{0, 0, 0}, {1, 0, 0}, {2, 0, 0}, {0, 0, 1}, {1, 0, 1}, {2, 0, 1}, {0, 0, 2}, {1, 0, 2}, {2, 0, 2}, {1, 1, 1}};
    gfx_Begin();
    gfx_SetTextXY(0, 0);
    gfx_SetTextFGColor(0);
    gfx_SetTextBGColor(255);
    gfx_SetTextScale(4, 4);
    initPalette();
    gfx_SetColor(0);
    printStringCentered("Loading...", 5);
    gfx_SetTextXY(0, 40);
    gfx_SetTextScale(1,1);
    printStringAndMoveDownCentered("While you wait:");
    printStringAndMoveDownCentered("Controls:");
    printStringAndMoveDownCentered("Clear: Exit");
    printStringAndMoveDownCentered("5: Place/Delete a block");
    printStringAndMoveDownCentered("8: Forwards");
    printStringAndMoveDownCentered("2: Backwards");
    printStringAndMoveDownCentered("4: Left");
    printStringAndMoveDownCentered("6: Right");
    printStringAndMoveDownCentered("Multiply/Subtract: Up/Down");
    printStringAndMoveDownCentered("9: Diagonally Forward");
    printStringAndMoveDownCentered("1: Diagonally Backward");
    printStringAndMoveDownCentered("7: Diagonally Left");
    printStringAndMoveDownCentered("3: Diagonally Right");
    printStringAndMoveDownCentered("Enter: Block selection menu");
    printStringAndMoveDownCentered("Made by Logan C.");
    // implement more controls in the near future
    for (int i = 0; i < 225; i++) {
        if (numberOfObjects < maxNumberOfObjects) {
            const uint8_t** texture = textures[i%18];
            objects[numberOfObjects] = new object((i%15)*20, 0, (i/15)*20, 20, texture, false);
            numberOfObjects++;
        }
    }
    for (unsigned int i = 0; i < numberOfObjects; i++) {
        objects[i]->generatePoints();
    }
    xSort();
    /*for (uint8_t i = 0; i < 200; i++) {
        if (numberOfObjects < maxNumberOfObjects) {
            const uint8_t** texture = crafting_table_texture;
            const point cubePoint = cubes[i];
            objects[numberOfObjects] = new object(cubePoint.x * (Fixed24)50, cubePoint.y*(Fixed24)50, cubePoint.z*(Fixed24)50, 50, texture);
            numberOfObjects++;
        }
    }*/
    printStringAndMoveDownCentered("Press any key to begin.");
    gfx_SetColor(255);
    gfx_FillRectangle_NoClip(0, 0, 320, 40);
    gfx_SetTextScale(4,4);
    printStringCentered("Loaded!", 5);
    gfx_SetTextScale(1,1);
    while (!os_GetCSC());
    drawScreen(0);
    drawCursor();
    bool quit = false;
    while (!quit) {
        uint8_t key = os_GetCSC();
        if (key) {
            switch (key) {
                // forward
                case sk_9:
                    moveCursor(4);
                    break;
                // backward
                case sk_1:
                    moveCursor(5);
                    break;
                // left
                case sk_7:
                    moveCursor(2);
                    break;
                // right
                case sk_3:
                    moveCursor(3);
                    break;
                // up
                case sk_Mul:
                    moveCursor(0);
                    break;
                case sk_Sub:
                    moveCursor(1);
                    break;
                case sk_8:
                    moveCursor(8);
                    break;
                case sk_2:
                    moveCursor(9);
                    break;
                case sk_4:
                    moveCursor(6);
                    break;
                case sk_6:
                    moveCursor(7);
                    break;
                case sk_5: {
                    object* annoyingPointer = &playerCursor;
                    object** matchingObject = (object**) bsearch((void*) &annoyingPointer, objects, numberOfObjects, sizeof(object *), xCompare);
                    bool deletedObject = false;
                    drawBuffer();
                    if (matchingObject != nullptr) {
                        while (matchingObject >= &objects[0]) {
                            if ((*matchingObject)->x != playerCursor.x) {
                                matchingObject++;
                                break;
                            }
                            matchingObject--;
                        }
                        while (matchingObject <= &objects[numberOfObjects - 1]) {
                            if ((*matchingObject)->x != playerCursor.x) {
                                break;
                            }
                            if ((*matchingObject)->y == playerCursor.y && (*matchingObject)->z == playerCursor.z) {
                                object* matchingObjectReference = *matchingObject;
                                memmove(matchingObject, matchingObject + 1, sizeof(object *) * (&objects[numberOfObjects - 1] - matchingObject));
                                numberOfObjects--;
                                matchingObjectReference->deleteObject();
                                delete matchingObjectReference;
                                xSort();
                                drawScreen(2);
                                getBuffer();
                                drawCursor();
                                deletedObject = true;
                                break;
                            }
                            matchingObject++;
                        }
                    }
                    if (!deletedObject) {
                        if (numberOfObjects < maxNumberOfObjects) {
                            deletePolygons();
                            objects[numberOfObjects] = new object(playerCursor.x, playerCursor.y, playerCursor.z, 20, textures[selectedObject], false);
                            gfx_SetDrawBuffer();
                            objects[numberOfObjects]->generatePoints();
                            objects[numberOfObjects]->generatePolygons(true);
                            gfx_SetDrawScreen();
                            numberOfObjects++;
                            xSort();
                            drawScreen(1);
                            getBuffer();
                            drawCursor();
                        }
                    }
                    break;
                }
                // redraw the screen
                case sk_2nd:
                    drawScreen(0);
                    getBuffer();
                    drawCursor();
                    break;
                // exit
                case sk_Clear:
                    quit = true;
                    gfx_SetDrawScreen();
                    gfx_FillScreen(254);
                    break;
                case sk_Enter:
                    selectBlock();
            }
        }
    }
    deleteEverything();
    gfx_End();
    return 0;
}

// still has problems
void drawCursor() {
    deletePolygons();
    if (cursorBackgroundBuffer) {
        drawBuffer();
    }
    playerCursor.generatePoints();
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
        minX--;
        minY--;
        int width = (maxX-minX) + 1;
        int height = (maxY-minY) + 1;
        /*if (cursorBackgroundBuffer) {
            delete[] cursorBackgroundBuffer;
        }*/
        cursorBackgroundBuffer = new uint8_t[width*height + 2];
        cursorBackground = (gfx_sprite_t*) cursorBackgroundBuffer;
        cursorBackground->width = width;
        cursorBackground->height = height;
        playerCursorX = minX;
        playerCursorY = minY;
        getBuffer();
        drawScreen(1);
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
    if (playerCursorX > 0 && cursorBackground->width + playerCursorX < 320 && playerCursorY > 0 && cursorBackground->height + playerCursorY < 240) {
        gfx_Sprite_NoClip(cursorBackground, playerCursorX, playerCursorY);
    } else {
        gfx_Sprite(cursorBackground, playerCursorX, playerCursorY);
    }
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

void selectBlock() {
    gfx_SetColor(102);
    gfx_FillRectangle(80, 60, 160, 120);
    for (uint8_t i = 0; i < 18; i++) {
        if (i == selectedObject) {
            gfx_SetColor(254);
            gfx_FillRectangle(98 + (20*(i%6)), 78 + (24*(i/6)), 20, 20);
        }
        uint8_t* blockBuffer = new uint8_t[258];
        memcpy(&blockBuffer[2], textures[i][1], 256);
        gfx_sprite_t* block = (gfx_sprite_t*) blockBuffer;
        block->width = 16;
        block->height = 16;
        gfx_Sprite_NoClip(block, 100 + (20*(i%6)), 80 + (24*(i/6)));
        delete[] blockBuffer;
    }
    bool quit = false;
    while (!quit) {
        uint8_t key = os_GetCSC();
        if (key) {
            switch (key) {
                case sk_Up:
                    if (selectedObject > 5) {
                        redrawSelectedObject();
                        selectedObject -= 6;
                        selectNewObject();
                    } else {
                        redrawSelectedObject();
                        selectedObject += 12;
                        selectNewObject();
                    }
                    break;
                case sk_Down:
                    if (selectedObject < 12) {
                        redrawSelectedObject();
                        selectedObject += 6;
                        selectNewObject();
                    } else {
                        redrawSelectedObject();
                        selectedObject -= 12;
                        selectNewObject();
                    }
                    break;
                case sk_Left:
                    if (selectedObject > 0) {
                        redrawSelectedObject();
                        selectedObject--;
                        selectNewObject();
                    } else {
                        redrawSelectedObject();
                        selectedObject = 17;
                        selectNewObject();
                    }
                    break;
                case sk_Right:
                    if (selectedObject < 17) {
                        redrawSelectedObject();
                        selectedObject++;
                        selectNewObject();
                    } else {
                        redrawSelectedObject();
                        selectedObject = 0;
                        selectNewObject();
                    }
                    break;
                case sk_Enter:
                    quit = true;
                    break;
                case sk_Clear:
                    quit = true;
                    break;
            }
        }
    }
    drawScreen(0);
    getBuffer();
    drawCursor();
}

void redrawSelectedObject() {
    gfx_SetColor(102);
    gfx_FillRectangle(98 + (20*(selectedObject%6)), 78 + (24*(selectedObject/6)), 20, 20);
    uint8_t* blockBuffer = new uint8_t[258];
    memcpy(&blockBuffer[2], textures[selectedObject][1], 256);
    gfx_sprite_t* block = (gfx_sprite_t*) blockBuffer;
    block->width = 16;
    block->height = 16;
    gfx_Sprite_NoClip(block, 100 + (20*(selectedObject%6)), 80 + (24*(selectedObject/6)));
    delete[] blockBuffer;
}

void selectNewObject() {
    gfx_SetColor(254);
    gfx_FillRectangle(98 + (20*(selectedObject%6)), 78 + (24*(selectedObject/6)), 20, 20);
    uint8_t* blockBuffer = new uint8_t[258];
    memcpy(&blockBuffer[2], textures[selectedObject][1], 256);
    gfx_sprite_t* block = (gfx_sprite_t*) blockBuffer;
    block->width = 16;
    block->height = 16;
    gfx_Sprite_NoClip(block, 100 + (20*(selectedObject%6)), 80 + (24*(selectedObject/6)));
    delete[] blockBuffer;
}