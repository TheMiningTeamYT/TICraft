#include <graphx.h>
#include <fileioc.h>
#include <ti/getcsc.h>
#include <sys/power.h>
#include <cstdint>
#include <cstdio>
#include "renderer.hpp"
#include "textures.hpp"
#include "crc32.h"

#define saveFileVersion 1

uint8_t* cursorBackgroundBuffer;
gfx_sprite_t* cursorBackground = (gfx_sprite_t*) cursorBackgroundBuffer;
uint8_t selectedObject = 10;

object playerCursor(20, 20, 20, 20, selectedObject, true);
int playerCursorX;
int playerCursorY;
char buffer2[200];

void drawCursor(bool generatePoints);
void getBuffer();
void drawBuffer();
void printStringCentered(const char* string, int row);
void printStringAndMoveDownCentered(const char* string);
void moveCursor(uint8_t direction);
void selectBlock();
void redrawSelectedObject();
void selectNewObject();
void gfxStart();
void failedToSave();
void failedToLoadSave();

int main() {
    boot_Set48MHzMode();
    point cubes[] = {{0, 0, 0}, {1, 0, 0}, {2, 0, 0}, {0, 0, 1}, {1, 0, 1}, {2, 0, 1}, {0, 0, 2}, {1, 0, 2}, {2, 0, 2}, {1, 1, 1}};
    bool userSelected = false;
    bool toSaveOrNotToSave = true;
    gfxStart();
    gfx_FillScreen(255);
    gfx_SetColor(0);
    uint8_t save = ti_Open("WORLD", "r");
    if (save) {
        gfx_SetTextXY(0, 110);
        printStringAndMoveDownCentered("Would you like to load your save?");
        printStringAndMoveDownCentered("Press 1 for yes, 2 for no.");
        while (!userSelected) {
            uint8_t key = os_GetCSC();
            if (key) {
                switch (key) {
                    case sk_1:
                        userSelected = true;
                        toSaveOrNotToSave = true;
                        break;
                    case sk_2:
                        userSelected = true;
                        toSaveOrNotToSave = false;
                        break;
                    default:
                        break;
                }
            }
        }
    } else {
        toSaveOrNotToSave = false;
    }
    if (toSaveOrNotToSave) {
        char signature[7];
        if (ti_Read(signature, 7, 1, save) == 1) {
            if (strcmp(signature, "BLOCKS") == 0) {
                unsigned int version;
                if (ti_Read(&version, sizeof(unsigned int), 1, save) == 1) {
                    if (version == saveFileVersion) {
                        if (ti_Seek(0, SEEK_SET, save) != EOF) {
                            uint32_t checksum = crc32((char*) ti_GetDataPtr(save), ti_GetSize(save) - sizeof(uint32_t));
                            if (ti_Seek(-4, SEEK_END, save) != EOF) {
                                uint32_t properChecksum;
                                if (ti_Read(&properChecksum, sizeof(uint32_t), 1, save) == 1) {
                                    if (checksum != properChecksum) {
                                        printStringAndMoveDownCentered("Bad checksum.");
                                        failedToLoadSave();
                                        toSaveOrNotToSave = false;
                                        ti_Close(save);
                                        ti_Delete("WORLD");
                                    }
                                } else {
                                    printStringAndMoveDownCentered("Failed to read checksum.");
                                    failedToLoadSave();
                                    toSaveOrNotToSave = false;
                                }
                            } else {
                                printStringAndMoveDownCentered("Failed to seek to end.");
                                failedToLoadSave();
                                toSaveOrNotToSave = false;
                            }
                        } else {
                            printStringAndMoveDownCentered("Failed to seek to beginning.");
                            failedToLoadSave();
                            toSaveOrNotToSave = false;
                        }
                    } else {
                        printStringAndMoveDownCentered("Wrong save file version");
                        failedToLoadSave();
                        toSaveOrNotToSave = false;
                        ti_Close(save);
                        ti_Delete("WORLD");
                    }
                } else {
                    printStringAndMoveDownCentered("Failed to read save file version.");
                    failedToLoadSave();
                    toSaveOrNotToSave = false;
                    ti_Close(save);
                    ti_Delete("WORLD");
                }
            } else {
                printStringAndMoveDownCentered("Bad signature.");
                failedToLoadSave();
                toSaveOrNotToSave = false;
                ti_Close(save);
                ti_Delete("WORLD");
            }
        }
    }
    if (save) {
        ti_Close(save);
    }
    gfx_FillScreen(255);
    gfx_SetTextScale(4, 4);
    gfx_SetTextXY(0, 0);
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
    printStringAndMoveDownCentered("2nd: Re-render the screen");
    printStringAndMoveDownCentered("Mode: Perform a full re-render of the screen");
    printStringAndMoveDownCentered("Made by Logan C.");
    // implement more controls in the near future
    if (!toSaveOrNotToSave) {
        for (int i = 0; i < 256; i++) {
            objects[numberOfObjects] = new object((i%16)*20, (i/256)*-20, ((i%256)/16)*20, 20, 10, false);
            objects[numberOfObjects]->generatePoints();
            numberOfObjects++;
        }
    } else {
        save = ti_Open("WORLD", "r");
        if (save) {
            if (ti_Seek(7 + sizeof(unsigned int), SEEK_SET, save) != EOF) {
                if (ti_Read(&numberOfObjects, sizeof(unsigned int), 1, save) == 1) {
                    if (numberOfObjects <= maxNumberOfObjects) {
                        for (unsigned int i = 0; i < numberOfObjects; i++) {
                            cubeSave cube;
                            if (ti_Read(&cube, sizeof(cubeSave), 1, save) == 1) {
                                objects[i] = new object(cube.x, cube.y, cube.z, cube.size, cube.texture, false);
                                objects[i]->generatePoints();
                            } else {
                                failedToLoadSave();
                                break;
                            }
                        }
                    }
                } else {
                    failedToLoadSave();
                }
            } else {
                failedToLoadSave();
            }
            ti_Close(save);
        } else {
            failedToLoadSave();
        }
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
                case sk_Clear:
                    quit = true;
                    gfx_SetDrawScreen();
                    gfx_FillScreen(255);
                    break;
                case sk_Enter:
                    selectBlock();
                    break;
                case sk_Up:
                    cameraXYZ[0] += 50;
                    cameraXYZ[2] += 50;
                    for (unsigned int i = 0; i < numberOfObjects; i++) {
                        objects[i]->generatePoints();
                    }
                    drawScreen(0);
                    getBuffer();
                    drawCursor(true);
                    break;
                case sk_Down:
                    cameraXYZ[0] -= 50;
                    cameraXYZ[2] -= 50;
                    for (unsigned int i = 0; i < numberOfObjects; i++) {
                        objects[i]->generatePoints();
                    }
                    drawScreen(0);
                    getBuffer();
                    drawCursor(true);
                    break;
                case sk_Left:
                    cameraXYZ[0] -= 50;
                    cameraXYZ[2] += 50;
                    for (unsigned int i = 0; i < numberOfObjects; i++) {
                        objects[i]->generatePoints();
                    }
                    drawScreen(0);
                    getBuffer();
                    drawCursor(true);
                    break;
                case sk_Right:
                    cameraXYZ[0] += 50;
                    cameraXYZ[2] -= 50;
                    for (unsigned int i = 0; i < numberOfObjects; i++) {
                        objects[i]->generatePoints();
                    }
                    drawScreen(0);
                    getBuffer();
                    drawCursor(true);
                    break;
                case sk_Del:
                    cameraXYZ[1] += 50;
                    for (unsigned int i = 0; i < numberOfObjects; i++) {
                        objects[i]->generatePoints();
                    }
                    drawScreen(0);
                    getBuffer();
                    drawCursor(true);
                    break;
                case sk_Stat:
                    cameraXYZ[1] -= 50;
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
    gfx_SetTextXY(0, 110);
    userSelected = false;
    toSaveOrNotToSave = true;
    printStringAndMoveDownCentered("Would you like to save?");
    printStringAndMoveDownCentered("Press 1 for yes, 2 for no.");
    while (!userSelected) {
        uint8_t key = os_GetCSC();
        if (key) {
            switch (key) {
                case sk_1:
                    userSelected = true;
                    toSaveOrNotToSave = true;
                    break;
                case sk_2:
                    userSelected = true;
                    toSaveOrNotToSave = false;
                    break;
                default:
                    break;
            }
        }
    }
    if (toSaveOrNotToSave) {
        ti_SetGCBehavior(gfx_End, gfxStart);
        save = ti_Open("WORLD", "w+");
        if (save) {
            if (ti_Write("BLOCKS", 7, 1, save) == 1) {
                unsigned int version = saveFileVersion;
                if (ti_Write(&version, sizeof(unsigned int), 1 ,save) == 1) {
                    if (ti_Write(&numberOfObjects, sizeof(unsigned int), 1, save) == 1) {
                        for (unsigned int i = 0; i < numberOfObjects; i++) {
                            cubeSave cube = {objects[i]->x, objects[i]->y, objects[i]->z, objects[i]->size, objects[i]->texture};
                            if (ti_Write(&cube, sizeof(cubeSave), 1, save) != 1) {
                                ti_Close(save);
                                ti_Delete("WORLD");
                                printStringAndMoveDownCentered("Failed to write blocks.");
                                failedToSave();
                                break;
                            } else {
                                delete objects[i];
                            }
                        }
                        if (save) {
                            if (ti_Seek(0, SEEK_SET, save) != EOF) {
                                uint32_t checksum = crc32((char*) ti_GetDataPtr(save), ti_GetSize(save));
                                if (ti_Seek(0, SEEK_END, save) != EOF) {
                                    if (ti_Write(&checksum, sizeof(uint32_t), 1, save) == 1) {
                                        ti_SetArchiveStatus(true, save);
                                        ti_Close(save);
                                    } else {
                                        ti_Close(save);
                                        ti_Delete("WORLD");
                                        printStringAndMoveDownCentered("Failed to seek to write checksum.");
                                        failedToSave();
                                    }
                                } else {
                                    ti_Close(save);
                                    ti_Delete("WORLD");
                                    printStringAndMoveDownCentered("Failed to seek to end.");
                                    failedToSave();
                                }
                            } else {
                                ti_Close(save);
                                ti_Delete("WORLD");
                                printStringAndMoveDownCentered("Failed to seek to beginning.");
                                failedToSave();
                            }
                        }
                    } else {
                        printStringAndMoveDownCentered("Failed to write number of blocks.");
                        failedToSave();
                        ti_Close(save);
                        ti_Delete("WORLD");
                    }
                } else {
                    printStringAndMoveDownCentered("Failed to write version.");
                    failedToSave();
                    ti_Close(save);
                    ti_Delete("WORLD");
                }
            } else {
                printStringAndMoveDownCentered("Failed to write signature.");
                failedToSave();
                ti_Close(save);
                ti_Delete("WORLD");
            }
        }
    }
    deleteEverything();
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
    playerCursor.generatePoints();
    if (playerCursor.visible) {
        drawCursor(false);
    } else if (visibleBefore) {
        switch (direction) {
            case 0:
                playerCursor.moveBy(0, -20, 0);
                break;
            case 1:
                playerCursor.moveBy(0, 20, 0);
                break;
            case 2:
                playerCursor.moveBy(0, 0, -20);
                break;
            case 3:
                playerCursor.moveBy(0, 0, 20);
                break;
            case 4:
                playerCursor.moveBy(-20, 0, 0);
                break;
            case 5:
                playerCursor.moveBy(20, 0, 0);
                break;
            case 6:
                playerCursor.moveBy(20, 0, -20);
                break;
            case 7:
                playerCursor.moveBy(-20, 0, 20);
                break;
            case 8:
                playerCursor.moveBy(-20, 0, -20);
                break;
            case 9:
                playerCursor.moveBy(20, 0, 20);
                break;
            default:
                return;
        }
    }
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

void gfxStart() {
    gfx_Begin();
    gfx_SetTextFGColor(0);
    gfx_SetTextBGColor(255);
    gfx_SetTextScale(1, 1);
    initPalette();
}

void failedToSave() {
    printStringAndMoveDownCentered("Failed to save.");
    printStringAndMoveDownCentered("Press any key to continue.");
    while (!os_GetCSC());
}

void failedToLoadSave() {
    printStringAndMoveDownCentered("Failed to load save.");
    printStringAndMoveDownCentered("Press any key to continue.");
    while (!os_GetCSC());
}