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
gfx_sprite_t* cursorBackground = (gfx_sprite_t*) 0xD2E1FD;

int playerCursorX = 0;
int playerCursorY = 0;

void drawCursor();
void getBuffer();
void drawBuffer();
void moveCursor(uint8_t direction);
void selectBlock();
void drawSelection(int offset);
void redrawScreen();

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
            fillDirt();
            gfx_SetTextXY(0, 105);
            gfx_SetTextFGColor(254);
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
        gfx_SetTextFGColor(0);
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
        printStringAndMoveDownCentered("Alpha: Move the camera to show the cursor");
        printStringAndMoveDownCentered("Mode: Perform a full re-render of the screen");
        printStringAndMoveDownCentered("Made by Logan C.");
        // implement more controls in the near future
        if (!toSaveOrNotToSave) {
            playerCursor.moveTo(20, 20, 20);
            resetCamera();
            for (int i = 0; i < 400; i++) {
                if (numberOfObjects < maxNumberOfObjects) {
                    // workaround for compiler bug
                    div_t xy = div(i, 20);
                    objects[numberOfObjects] = new object((xy.rem)*20, 0, (xy.quot)*20, 20, 10, false);
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
        drawScreen(true);
        getBuffer();
        drawCursor();
        quit = false;
        while (!quit) {
            uint8_t key = os_GetCSC();
            if (key) {
                switch (key) {
                    // forward
                    case sk_9:
                        if (angleY <= -67.5f) {
                            moveCursor(6);
                        } else if (angleY <= -22.5f && angleY >= -67.5f) {
                            moveCursor(2);
                        } else if (angleY <= 22.5f && angleY >= -22.5f) {
                            moveCursor(8);
                        } else if (angleY >= 67.5f) {
                            moveCursor(7);
                        } else {
                            moveCursor(4);
                        }
                        break;
                    // backward
                    case sk_1:
                        if (angleY <= -67.5f) {
                            moveCursor(7);
                        } else if (angleY <= -22.5f && angleY >= -67.5f) {
                            moveCursor(3);
                        } else if (angleY <= 22.5f && angleY >= -22.5f) {
                            moveCursor(9);
                        } else if (angleY >= 67.5f) {
                            moveCursor(6);
                        } else {
                            moveCursor(5);
                        }
                        break;
                    // left
                    case sk_7:
                        if (angleY <= -67.5f) {
                            moveCursor(9);
                        } else if (angleY <= -22.5f && angleY >= -67.5f) {
                            moveCursor(5);
                        } else if (angleY <= 22.5f && angleY >= -67.5f) {
                            moveCursor(6);
                        } else if (angleY >= 67.5f) {
                            moveCursor(8);
                        } else {
                            moveCursor(2);
                        }
                        break;
                    // right
                    case sk_3:
                        if (angleY <= -67.5f) {
                            moveCursor(8);
                        } else if (angleY <= -22.5f && angleY >= -67.5f) {
                            moveCursor(4);
                        } else if (angleY <= 22.5f && angleY >= -22.5f) {
                            moveCursor(7);
                        } else if (angleY >= 67.5f) {
                            moveCursor(9);
                        } else {
                            moveCursor(3);
                        }
                        break;
                    // up
                    case sk_Mul:
                        moveCursor(0);
                        break;
                    case sk_Sub:
                        moveCursor(1);
                        break;
                    case sk_8:
                        if (angleY <= -67.5f) {
                            moveCursor(5);
                        } else if (angleY <= -22.5f && angleY >= -67.5f) {
                            moveCursor(6);
                        } else if (angleY <= 22.5f && angleY >= -22.5f) {
                            moveCursor(2);
                        } else if (angleY >= 67.5f) {
                            moveCursor(4);
                        } else {
                            moveCursor(8);
                        }
                        break;
                    case sk_2:
                        if (angleY <= -67.5f) {
                            moveCursor(4);
                        } else if (angleY <= -22.5f && angleY >= -67.5f) {
                            moveCursor(7);
                        } else if (angleY <= 22.5f && angleY >= -22.5f) {
                            moveCursor(3);
                        } else if (angleY >= 67.5f) {
                            moveCursor(5);
                        } else {
                            moveCursor(9);
                        }
                        break;
                    case sk_4:
                        if (angleY <= -67.5f) {
                            moveCursor(3);
                        } else if (angleY <= -22.5f && angleY >= -67.5f) {
                            moveCursor(9);
                        } else if (angleY <= 22.5f && angleY >= -22.5f) {
                            moveCursor(5);
                        } else if (angleY >= 67.5f) {
                            moveCursor(2);
                        } else {
                            moveCursor(6);
                        }
                        break;
                    case sk_6:
                        if (angleY <= -67.5f) {
                            moveCursor(2);
                        } else if (angleY <= -22.5f && angleY >= -67.5f) {
                            moveCursor(8);
                        } else if (angleY <= 22.5f && angleY >= -22.5f) {
                            moveCursor(4);
                        } else if (angleY >= 67.5f) {
                            moveCursor(3);
                        } else {
                            moveCursor(7);
                        }
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
                                if ((*matchingObject)->y == playerCursor.y && (*matchingObject)->z == playerCursor.z) {
                                    object* matchingObjectReference = *matchingObject;
                                    memmove(matchingObject, matchingObject + 1, sizeof(object*) * (&objects[numberOfObjects - 1] - matchingObject));
                                    numberOfObjects--;
                                    matchingObjectReference->deleteObject();
                                    delete matchingObjectReference;
                                    xSort();
                                    drawScreen(false);
                                    getBuffer();
                                    deletedObject = true;
                                    break;
                                }
                                matchingObject++;
                            }
                        }
                        if (!deletedObject) {
                            if (numberOfObjects < maxNumberOfObjects) {
                                object* newObject = new object(playerCursor.x, playerCursor.y, playerCursor.z, 20, selectedObject, false);
                                newObject->generatePoints();
                                objects[numberOfObjects] = newObject;
                                numberOfObjects++;
                                xSort();
                                gfx_SetDrawBuffer();
                                newObject->generatePolygons(true);
                                gfx_SetDrawScreen();
                                getBuffer();
                            }
                        }
                        drawCursor();
                        break;
                    }
                    case sk_Mode:
                        drawScreen(true);
                        getBuffer();
                        drawCursor();
                        break;
                    // exit
                    case sk_Graph:
                        quit = true;
                        gfx_SetDrawScreen();
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
                        cameraXYZ[0] += (Fixed24)56.56854251f*sy;
                        cameraXYZ[2] += (Fixed24)56.56854251f*cy;
                        redrawScreen();
                        break;
                    case sk_Down:
                        cameraXYZ[0] -= (Fixed24)56.56854251f*sy;
                        cameraXYZ[2] -= (Fixed24)56.56854251f*cy;
                        redrawScreen();
                        break;
                    case sk_Left:
                        cameraXYZ[0] -= (Fixed24)56.56854251f*cy;
                        cameraXYZ[2] += (Fixed24)56.56854251f*sy;
                        redrawScreen();
                        break;
                    case sk_Right:
                        cameraXYZ[0] += (Fixed24)56.56854251f*cy;
                        cameraXYZ[2] -= (Fixed24)56.56854251f*sy;
                        redrawScreen();
                        break;
                    case sk_Del:
                        cameraXYZ[1] += 20;
                        redrawScreen();
                        break;
                    case sk_Stat:
                        cameraXYZ[1] -= 20;
                        redrawScreen();
                        break;
                    case sk_Prgm:
                        rotateCamera(-5, 0);
                        break;
                    case sk_Cos:
                        rotateCamera(5, 0);
                        break;
                    case sk_Sin:
                        rotateCamera(0, -5);
                        break;
                    case sk_Tan:
                        rotateCamera(0, 5);
                        break;
                    case sk_Alpha:
                        cameraXYZ[0] = playerCursor.x - (((Fixed24)84.85281375f)*sy);
                        cameraXYZ[1] = playerCursor.y + (Fixed24)75;
                        cameraXYZ[2] = playerCursor.z - (((Fixed24)84.85281375f)*cy);
                        redrawScreen();
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
void drawCursor() {
    drawBuffer();
    playerCursor.generatePoints();
    if (playerCursor.visible) {
        int minX = playerCursor.renderedPoints[0].x;
        int minY = playerCursor.renderedPoints[0].y;
        int maxX = playerCursor.renderedPoints[0].x;
        int maxY = playerCursor.renderedPoints[0].y;
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
        if (width*height <= 65025) {
            cursorBackground->width = width;
            cursorBackground->height = height;
            playerCursorX = minX;
            playerCursorY = minY;
            getBuffer();
        }
        playerCursor.generatePolygons(false);
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
    drawCursor();
}

void getBuffer() {
    gfx_GetSprite(cursorBackground, playerCursorX, playerCursorY);
}

void drawBuffer() {
    gfx_Sprite(cursorBackground, playerCursorX, playerCursorY);
}

void selectBlock() {
    drawBuffer();
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
    bool quit = false;
    while (!quit) {
        uint8_t key = os_GetCSC();
        if (key) {
            switch (key) {
                case sk_Up:
                case sk_8:
                    if (selectedObject/8 > 0) {
                        drawSelection(-8);
                    } else {
                        drawSelection(40);
                    }
                    break;
                case sk_Down:
                case sk_2:
                    if (selectedObject/8 < 5) {
                        drawSelection(8);
                    } else {
                        drawSelection(-40);
                    }
                    break;
                case sk_Left:
                case sk_4:
                    if (selectedObject > 0) {
                        drawSelection(-1);
                    } else {
                        drawSelection(((int)-selectedObject) + 47);
                    }
                    break;
                case sk_Right:
                case sk_6:
                    if (selectedObject < 47) {
                        drawSelection(1);
                    } else {
                        drawSelection(((int)-selectedObject));
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
    gfx_FillRectangle(72, 42, 176, 156);
    drawScreen(false);
    getBuffer();
    drawCursor();
}

void drawSelection(int offset) {
    gfx_SetColor(253);
    gfx_FillRectangle(80 + (20*(selectedObject%8)), 50 + (24*(selectedObject/8)), 20, 20);
    memcpy(cursorBackground->data, textures[selectedObject][1], 256);
    gfx_Sprite_NoClip(cursorBackground, 82 + (20*(selectedObject%8)), 52 + (24*(selectedObject/8)));
    selectedObject += offset;
    gfx_SetColor(254);
    gfx_FillRectangle(80 + (20*(selectedObject%8)), 50 + (24*(selectedObject/8)), 20, 20);
    memcpy(cursorBackground->data, textures[selectedObject][1], 256);
    gfx_Sprite_NoClip(cursorBackground, 82 + (20*(selectedObject%8)), 52 + (24*(selectedObject/8)));
}

void redrawScreen() {
    for (unsigned int i = 0; i < numberOfObjects; i++) {
        objects[i]->generatePoints();
    }
    xSort();
    drawScreen(true);
    getBuffer();
    drawCursor();
}