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

/*
Implement chunking
Idea:
Load 1 chunk into the list of cubes at a time (say, when the cursor moves into a new chunk or the camera moves)
Generate the polygons for each chunk, then delete it from the list of cubes and load the next one
At the end load the current chunk into the list of cubes
Pros:
Allows many more blocks at theoretically no additional RAM cost
Cons:
Doing a re-render will be INSANELY expensive
*/
gfx_sprite_t* cursorBackground = (gfx_sprite_t*) 0xD30000;

int playerCursorX = 0;
int playerCursorY = 0;

void drawCursor(bool generatePoints);
void getBuffer();
void drawBuffer();
void moveCursor(uint8_t direction);
void selectBlock();
void redrawSelectedObject();
void selectNewObject();

int main() {
    boot_Set48MHzMode();
    ti_SetGCBehavior(gfx_End, gfxStart);
    bool userSelected = false;
    bool toSaveOrNotToSave = true;
    gfxStart();
    gfx_SetColor(0);
    gfx_SetTextScale(1, 1);
    char nameBuffer[10];
    bool emergencyExit = false;
    bool usb = false;
    load_save:
    while (!emergencyExit && mainMenu(nameBuffer, 10)) {
        bool quit = false;
        while (quit == false) {
            gfx_FillScreen(255);
            gfx_SetTextXY(0, 105);
            printStringAndMoveDownCentered("Would you like to load from archive or USB?");
            printStringAndMoveDownCentered("Press 1 for archive, press 2 for USB.");
            printStringAndMoveDownCentered("Or press clear to cancel.");
            uint8_t key = os_GetCSC();
            while (!key) {
                key = os_GetCSC();
            }
            switch (key) {
                case sk_1:
                    usb = false;
                    quit = true;
                    break;
                case sk_2:
                    printStringAndMoveDownCentered("Please plug in a USB drive now.");
                    printStringAndMoveDownCentered("Then press a key to continue.");
                    printStringAndMoveDownCentered("Please do not unplug the drive until");
                    printStringAndMoveDownCentered("the save is loaded.");
                    while (!os_GetCSC());
                    printStringAndMoveDownCentered("Press any key to cancel loading from USB");
                    printStringAndMoveDownCentered("(Doing so will result in the genration of");
                    printStringAndMoveDownCentered("a new world)");
                    usb = true;
                    quit = true;
                    break;
                case sk_Clear:
                    goto load_save;
                    break;
                default:
                    break;
            }
        }
        toSaveOrNotToSave = checkSave(nameBuffer, usb);
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
            playerCursor.moveTo(20, 20, 20);
            cameraXYZ[0] = -100;
            cameraXYZ[1] = 150;
            cameraXYZ[2] = -100;
            for (int i = 0; i < 400; i++) {
                if (numberOfObjects < maxNumberOfObjects) {
                    // workaround for compiler bug
                    objects[numberOfObjects] = new object((i%20)*20, 0, (int)((float)i/(float)20)*20, 20, 10, false);
                    numberOfObjects++;
                }
            }
        } else {
            load(nameBuffer, usb);
        }
        for (unsigned int i = 0; i < numberOfObjects; i++) {
            objects[i]->generatePoints();
        }
        xSort();
        cursorBackground->height = 1;
        cursorBackground->width = 1;
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
        quit = false;
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
                                    drawScreen(3);
                                    getBuffer();
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
                                objects[numberOfObjects]->generatePolygons(2);
                                gfx_SetDrawScreen();
                                numberOfObjects++;
                                xSort();
                                drawScreen(1);
                                getBuffer();
                            }
                        }
                        drawCursor(true);
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
                        xSort();
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
                        xSort();
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
                        xSort();
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
                        xSort();
                        drawScreen(0);
                        getBuffer();
                        drawCursor(true);
                        break;
                    case sk_Del:
                        cameraXYZ[1] += 40;
                        for (unsigned int i = 0; i < numberOfObjects; i++) {
                            objects[i]->generatePoints();
                        }
                        xSort();
                        drawScreen(0);
                        getBuffer();
                        drawCursor(true);
                        break;
                    case sk_Stat:
                        cameraXYZ[1] -= 40;
                        for (unsigned int i = 0; i < numberOfObjects; i++) {
                            objects[i]->generatePoints();
                        }
                        xSort();
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
    // clear out pixel shadow
    memset((void*) 0xD031F6, 0, 8400);
    // clear out graph
    memset((void*) 0xD09466, 0, 43890);
    return 0;
}

// still has problems
void drawCursor(bool generatePoints) {
    deletePolygons();
    drawBuffer();
    if (generatePoints) {
        playerCursor.generatePoints();
    }
    if (playerCursor.visible) {
        playerCursor.generatePolygons(0);
        int minX = playerCursor.renderedPoints[0].x;
        int minY = playerCursor.renderedPoints[0].x;
        int maxX = playerCursor.renderedPoints[0].x;
        int maxY = playerCursor.renderedPoints[0].x;
        for (uint8_t i = 1; i < 7; i++) {
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
        if (width*height <= 47360) {
            cursorBackground->width = width;
            cursorBackground->height = height;
            playerCursorX = minX;
            playerCursorY = minY;
            getBuffer();
        }
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
    gfx_SetDrawScreen();
    gfx_SetColor(253);
    gfx_FillRectangle(72, 42, 176, 156);
    for (uint8_t i = 0; i < 48; i++) {
        if (i == selectedObject) {
            gfx_SetColor(254);
            gfx_FillRectangle(80 + (20*(i%8)), 50 + (24*(i/8)), 20, 20);
        }
        memcpy(cursorBackground->data, textures[i][1], 256);
        cursorBackground->width = 16;
        cursorBackground->height = 16;
        gfx_Sprite_NoClip(cursorBackground, 82 + (20*(i%8)), 52 + (24*(i/8)));
    }
    bool quit = false;
    while (!quit) {
        uint8_t key = os_GetCSC();
        if (key) {
            switch (key) {
                case sk_Up:
                    if (selectedObject/8 > 0) {
                        redrawSelectedObject();
                        selectedObject -= 8;
                        selectNewObject();
                    } else {
                        redrawSelectedObject();
                        selectedObject += 40;
                        selectNewObject();
                    }
                    break;
                case sk_Down:
                    if (selectedObject/8 < 5) {
                        redrawSelectedObject();
                        selectedObject += 8;
                        selectNewObject();
                    } else {
                        redrawSelectedObject();
                        selectedObject -= 40;
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
                        selectedObject = 47;
                        selectNewObject();
                    }
                    break;
                case sk_Right:
                    if (selectedObject < 47) {
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
    gfx_SetDrawBuffer();
    gfx_SetColor(255);
    gfx_FillRectangle(72, 42, 176, 156);
    gfx_SetDrawScreen();
    gfx_SetColor(255);
    gfx_FillRectangle(72, 42, 176, 156);
    drawScreen(3);
    getBuffer();
    drawCursor(true);
}

void redrawSelectedObject() {
    gfx_SetColor(253);
    gfx_FillRectangle(80 + (20*(selectedObject%8)), 50 + (24*(selectedObject/8)), 20, 20);
    memcpy(cursorBackground->data, textures[selectedObject][1], 256);
    cursorBackground->width = 16;
    cursorBackground->height = 16;
    gfx_Sprite_NoClip(cursorBackground, 82 + (20*(selectedObject%8)), 52 + (24*(selectedObject/8)));
}

void selectNewObject() {
    gfx_SetColor(254);
    gfx_FillRectangle(80 + (20*(selectedObject%8)), 50 + (24*(selectedObject/8)), 20, 20);
    memcpy(cursorBackground->data, textures[selectedObject][1], 256);
    cursorBackground->width = 16;
    cursorBackground->height = 16;
    gfx_Sprite_NoClip(cursorBackground, 82 + (20*(selectedObject%8)), 52 + (24*(selectedObject/8)));
}