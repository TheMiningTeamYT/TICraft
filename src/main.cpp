#include <graphx.h>
#include <fileioc.h>
#include <ti/getcsc.h>
#include <sys/power.h>
#include <cstdint>
#include <cstdio>
#include "renderer.hpp"
#include "textures.hpp"
#include "printString.hpp"
#include "saves.hpp"
#include "crc32.h"

uint8_t* cursorBackgroundBuffer;
gfx_sprite_t* cursorBackground = (gfx_sprite_t*) cursorBackgroundBuffer;

int playerCursorX;
int playerCursorY;
char buffer2[200];

void drawCursor(bool generatePoints);
void getBuffer();
void drawBuffer();
void moveCursor(uint8_t direction);
void selectBlock();
void redrawSelectedObject();
void selectNewObject();

int main() {
    boot_Set48MHzMode();
    bool userSelected = false;
    bool toSaveOrNotToSave = true;
    gfxStart();
    gfx_FillScreen(255);
    gfx_SetColor(0);
    gfx_FillScreen(255);
    gfx_SetTextScale(1, 1);
    char nameBuffer[10];
    bool emergencyExit = false;
    while (!emergencyExit && mainMenu(nameBuffer, 10)) {
        toSaveOrNotToSave = checkSave(nameBuffer);
        gfx_FillScreen(255);
        gfx_SetTextScale(4, 4);
        gfx_SetTextXY(0, 0);
        printStringCentered("Loading...", 5);
        gfx_SetTextXY(0, 40);
        gfx_SetTextScale(1,1);
        printStringAndMoveDownCentered("While you wait:");
        printStringAndMoveDownCentered("Controls:");
        printStringAndMoveDownCentered("Graph: Exit, Clear: Emergency Exit");
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
        printStringAndMoveDownCentered("2nd: Re-render the screen");
        printStringAndMoveDownCentered("Mode: Perform a full re-render of the screen");
        printStringAndMoveDownCentered("Made by Logan C.");
        // implement more controls in the near future
        if (!toSaveOrNotToSave) {
            for (int i = 0; i < 256; i++) {
                if (numberOfObjects < maxNumberOfObjects) {
                    objects[numberOfObjects] = new object((i%16)*20, (i/256)*-20, ((i%256)/16)*20, 20, 10, false);
                    numberOfObjects++;
                }
            }
        } else {
            load(nameBuffer);
        }
        for (unsigned int i = 0; i < numberOfObjects; i++) {
            objects[i]->generatePoints();
        }
        xSort();
        printStringAndMoveDownCentered("Press any key to begin.");
        gfx_SetColor(255);
        gfx_FillRectangle_NoClip(0, 0, 320, 40);
        gfx_SetTextScale(4,4);
        printStringCentered("Loaded!", 5);
        gfx_SetTextScale(1,1);
        while (!os_GetCSC());
        drawScreen(0);
        getBuffer();
        drawCursor(true);
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
                            while (matchingObject > &objects[0]) {
                                if ((*matchingObject)->x < playerCursor.x) {
                                    break;
                                }
                                matchingObject--;
                            }
                            while (matchingObject <= &objects[numberOfObjects - 1]) {
                                if ((*matchingObject)->x > playerCursor.x) {
                                    break;
                                }
                                if ((*matchingObject)->x == playerCursor.x && (*matchingObject)->y == playerCursor.y && (*matchingObject)->z == playerCursor.z) {
                                    object* matchingObjectReference = *matchingObject;
                                    memmove(matchingObject, matchingObject + 1, sizeof(object *) * (&objects[numberOfObjects - 1] - matchingObject));
                                    numberOfObjects--;
                                    matchingObjectReference->deleteObject();
                                    delete matchingObjectReference;
                                    xSort();
                                    drawScreen(2);
                                    getBuffer();
                                    drawCursor(true);
                                    deletedObject = true;
                                    break;
                                }
                                matchingObject++;
                            }
                        }
                        if (!deletedObject) {
                            if (numberOfObjects < maxNumberOfObjects) {
                                deletePolygons();
                                objects[numberOfObjects] = new object(playerCursor.x, playerCursor.y, playerCursor.z, 20, selectedObject, false);
                                gfx_SetDrawBuffer();
                                objects[numberOfObjects]->generatePoints();
                                objects[numberOfObjects]->generatePolygons(true);
                                gfx_SetDrawScreen();
                                numberOfObjects++;
                                xSort();
                                drawScreen(1);
                                getBuffer();
                                drawCursor(true);
                            }
                        }
                        break;
                    }
                    case sk_Mode:
                        drawScreen(0);
                        getBuffer();
                        drawCursor(true);
                        break;
                    // redraw the screen
                    case sk_2nd:
                        drawBuffer();
                        drawScreen(2);
                        getBuffer();
                        drawCursor(true);
                        break;
                    // exit
                    case sk_Graph:
                        quit = true;
                        gfx_SetDrawScreen();
                        gfx_FillScreen(255);
                        deletePolygons();
                        save(nameBuffer);
                        break;
                    case sk_Clear:
                        quit = true;
                        emergencyExit = true;
                        break;
                    case sk_Enter:
                        selectBlock();
                        break;
                    case sk_Up:
                        cameraXYZ[0] += 40;
                        cameraXYZ[2] += 40;
                        for (unsigned int i = 0; i < numberOfObjects; i++) {
                            objects[i]->generatePoints();
                        }
                        drawScreen(0);
                        getBuffer();
                        drawCursor(true);
                        break;
                    case sk_Down:
                        cameraXYZ[0] -= 40;
                        cameraXYZ[2] -= 40;
                        for (unsigned int i = 0; i < numberOfObjects; i++) {
                            objects[i]->generatePoints();
                        }
                        drawScreen(0);
                        getBuffer();
                        drawCursor(true);
                        break;
                    case sk_Left:
                        cameraXYZ[0] -= 40;
                        cameraXYZ[2] += 40;
                        for (unsigned int i = 0; i < numberOfObjects; i++) {
                            objects[i]->generatePoints();
                        }
                        drawScreen(0);
                        getBuffer();
                        drawCursor(true);
                        break;
                    case sk_Right:
                        cameraXYZ[0] += 40;
                        cameraXYZ[2] -= 40;
                        for (unsigned int i = 0; i < numberOfObjects; i++) {
                            objects[i]->generatePoints();
                        }
                        drawScreen(0);
                        getBuffer();
                        drawCursor(true);
                        break;
                    case sk_Del:
                        cameraXYZ[1] += 40;
                        for (unsigned int i = 0; i < numberOfObjects; i++) {
                            objects[i]->generatePoints();
                        }
                        drawScreen(0);
                        getBuffer();
                        drawCursor(true);
                        break;
                    case sk_Stat:
                        cameraXYZ[1] -= 40;
                        for (unsigned int i = 0; i < numberOfObjects; i++) {
                            objects[i]->generatePoints();
                        }
                        drawScreen(0);
                        getBuffer();
                        drawCursor(true);
                        break;
                    default:
                        break;
                }
            }
        }
        deleteEverything();
    }
    gfx_SetDrawScreen();
    gfx_FillScreen(254);
    gfx_End();
    return 0;
}

// still has problems
void drawCursor(bool generatePoints) {
    deletePolygons();
    if (cursorBackgroundBuffer) {
        drawBuffer();
    }
    if (generatePoints) {
        playerCursor.generatePoints();
    }
    if (playerCursor.visible) {
        playerCursor.generatePolygons(false);
        delete[] cursorBackgroundBuffer;
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
    bool visibleBefore = playerCursor.visible;
    switch (direction) {
        case 0:
            if (playerCursor.y < (Fixed24)2027)
                playerCursor.moveBy(0, 20, 0);
            break;
        case 1:
            if (playerCursor.y > (Fixed24)-2027)
                playerCursor.moveBy(0, -20, 0);
            break;
        case 2:
            if (playerCursor.z < (Fixed24)2027)
                playerCursor.moveBy(0, 0, 20);
            break;
        case 3:
            if (playerCursor.z > (Fixed24)-2027)
                playerCursor.moveBy(0, 0, -20);
            break;
        case 4:
            if (playerCursor.x < (Fixed24)2027)
                playerCursor.moveBy(20, 0, 0);
            break;
        case 5:
            if (playerCursor.x > (Fixed24)-2027)
                playerCursor.moveBy(-20, 0, 0);
            break;
        case 6:
            if (playerCursor.x > (Fixed24)-2027 && playerCursor.z < (Fixed24)2027)
                playerCursor.moveBy(-20, 0, 20);
            break;
        case 7:
            if (playerCursor.x < (Fixed24)2027 && playerCursor.z > (Fixed24)-2027)
                playerCursor.moveBy(20, 0, -20);
            break;
        case 8:
            if (playerCursor.x < (Fixed24)2027 && playerCursor.z < (Fixed24)2027)
                playerCursor.moveBy(20, 0, 20);
            break;
        case 9:
            if (playerCursor.x > (Fixed24)-2027 && playerCursor.z > (Fixed24)-2027)
                playerCursor.moveBy(-20, 0, -20);
            break;
        default:
            return;
    }
    playerCursor.generatePoints();
    if (playerCursor.visible) {
        drawCursor(false);
    } else if (visibleBefore) {
        switch (direction) {
            case 0:
                if (playerCursor.y > (Fixed24)-2027)
                    playerCursor.moveBy(0, -20, 0);
                break;
            case 1:
                if (playerCursor.y < (Fixed24)2027)
                    playerCursor.moveBy(0, 20, 0);
                break;
            case 2:
                if (playerCursor.z > (Fixed24)-2027)
                    playerCursor.moveBy(0, 0, -20);
                break;
            case 3:
                if (playerCursor.z < (Fixed24)2027)
                    playerCursor.moveBy(0, 0, 20);
                break;
            case 4:
                if (playerCursor.x > (Fixed24)-2027)
                    playerCursor.moveBy(-20, 0, 0);
                break;
            case 5:
                if (playerCursor.x < (Fixed24)2027)
                    playerCursor.moveBy(20, 0, 0);
                break;
            case 6:
                if (playerCursor.x < (Fixed24)2027 && playerCursor.z > (Fixed24)-2027)
                    playerCursor.moveBy(20, 0, -20);
                break;
            case 7:
                if (playerCursor.x > (Fixed24)-2027 && playerCursor.z < (Fixed24)2027)
                    playerCursor.moveBy(-20, 0, 20);
                break;
            case 8:
                if (playerCursor.x > (Fixed24)-2027 && playerCursor.z > (Fixed24)-2027)
                    playerCursor.moveBy(-20, 0, -20);
                break;
            case 9:
                if (playerCursor.x < (Fixed24)2027 && playerCursor.z < (Fixed24)2027)
                    playerCursor.moveBy(20, 0, 20);
                break;
            default:
                return;
        }
        playerCursor.generatePoints();
    }
}

void getBuffer() {
    if (cursorBackground) {
        gfx_GetSprite(cursorBackground, playerCursorX, playerCursorY);
    }
}

void drawBuffer() {
    if (playerCursorX > 0 && cursorBackground->width + playerCursorX < 320 && playerCursorY > 0 && cursorBackground->height + playerCursorY < 240) {
        gfx_Sprite_NoClip(cursorBackground, playerCursorX, playerCursorY);
    } else {
        gfx_Sprite(cursorBackground, playerCursorX, playerCursorY);
    }
}

void selectBlock() {
    drawBuffer();
    gfx_SetDrawBuffer();
    gfx_SetColor(255);
    gfx_FillRectangle(80, 60, 160, 120);
    gfx_SetDrawScreen();
    gfx_SetColor(102);
    gfx_FillRectangle(80, 60, 160, 120);
    for (uint8_t i = 0; i < 23; i++) {
        if (i == selectedObject) {
            gfx_SetColor(254);
            gfx_FillRectangle(98 + (20*(i%6)), 68 + (24*(i/6)), 20, 20);
        }
        uint8_t* blockBuffer = new uint8_t[258];
        memcpy(&blockBuffer[2], textures[i][1], 256);
        gfx_sprite_t* block = (gfx_sprite_t*) blockBuffer;
        block->width = 16;
        block->height = 16;
        gfx_Sprite_NoClip(block, 100 + (20*(i%6)), 70 + (24*(i/6)));
        delete[] blockBuffer;
    }
    bool quit = false;
    while (!quit) {
        uint8_t key = os_GetCSC();
        if (key) {
            switch (key) {
                case sk_Up:
                    if (selectedObject/6 > 0) {
                        redrawSelectedObject();
                        selectedObject -= 6;
                        selectNewObject();
                    } else {
                        redrawSelectedObject();
                        if (selectedObject != 5) {
                            selectedObject += 18;
                        } else {
                            selectedObject += 12;
                        }
                        selectNewObject();
                    }
                    break;
                case sk_Down:
                    if (selectedObject/6 < 3 && selectedObject != 17) {
                        redrawSelectedObject();
                        selectedObject += 6;
                        selectNewObject();
                    } else {
                        redrawSelectedObject();
                        if (selectedObject != 17) {
                            selectedObject -= 18;
                        } else {
                            selectedObject -= 12;
                        }
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
                        selectedObject = 22;
                        selectNewObject();
                    }
                    break;
                case sk_Right:
                    if (selectedObject < 22) {
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
    gfx_SetColor(255);
    gfx_FillRectangle(80, 60, 160, 120);
    drawScreen(2);
    getBuffer();
    drawCursor(true);
}

void redrawSelectedObject() {
    gfx_SetColor(102);
    gfx_FillRectangle(98 + (20*(selectedObject%6)), 68 + (24*(selectedObject/6)), 20, 20);
    uint8_t* blockBuffer = new uint8_t[258];
    memcpy(&blockBuffer[2], textures[selectedObject][1], 256);
    gfx_sprite_t* block = (gfx_sprite_t*) blockBuffer;
    block->width = 16;
    block->height = 16;
    gfx_Sprite_NoClip(block, 100 + (20*(selectedObject%6)), 70 + (24*(selectedObject/6)));
    delete[] blockBuffer;
}

void selectNewObject() {
    gfx_SetColor(254);
    gfx_FillRectangle(98 + (20*(selectedObject%6)), 68 + (24*(selectedObject/6)), 20, 20);
    uint8_t* blockBuffer = new uint8_t[258];
    memcpy(&blockBuffer[2], textures[selectedObject][1], 256);
    gfx_sprite_t* block = (gfx_sprite_t*) blockBuffer;
    block->width = 16;
    block->height = 16;
    gfx_Sprite_NoClip(block, 100 + (20*(selectedObject%6)), 70 + (24*(selectedObject/6)));
    delete[] blockBuffer;
}