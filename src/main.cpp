#include <time.h>
#include <graphx.h>
#include <fileioc.h>
#include <ti/flags.h>
#include <ti/getkey.h>
#include <sys/power.h>
#include "renderer.hpp"
#include "textures.hpp"
#include "saves.hpp"
#include "cursor.hpp"
#include "version.hpp"
#include "sincos.hpp"
#include "blockMenu.hpp"
#include "sys.hpp"
#include "texturePackMenu.hpp"
#define freeMem 0xD02587

extern "C" {
    void ti_CleanAll();
    int ti_MemChk();
    #include "printString.h"
}

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

bool fineMovement;

int main() {
    //srand(time(nullptr));
    interruptOriginalValue = *((uint8_t*)0xF00005);
    // Disable RTC interrupt
    *((uint8_t*)0xF00005) &= 0xEF;
    os_DisableAPD();
    os_ClrHomeFull();
    ti_SetGCBehavior(gfx_End, texturePackMenu);
    // what exactly do you do?
    // ti_CleanAll();
    // (255*255) + 2 = 65027B (63.50 KiB)
    if (ti_MemChk() < (255*255) + 2) {
        os_PutStrFull("WARNING!! Not enough free user mem to run TICRAFT! Continuing may result in corruption or your calculator crashing and resetting. Try deleting or archiving some programs or AppVars. Press \"Enter\" to continue or press any other key to exit.");
        os_ResetFlag(SHIFT, ALPHALOCK);
        if (os_GetKey() != k_Enter) {
            return 1;
        }
        os_ClrHomeFull();
    }
    cursorBackground = *((gfx_sprite_t**) freeMem);
    // texture pack selection menu goes here
    texturePackMenu();
    fastSinCos::generateTable();
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
        while (!quit) {
            fillDirt();
            gfx_SetTextXY(0, 105);
            gfx_SetTextFGColor(254);
            printStringAndMoveDownCentered("Would you like to load from archive or USB?");
            printStringAndMoveDownCentered("Press 1 for archive, press 2 for USB.");
            printStringAndMoveDownCentered("Or press clear to cancel.");
            userSelected = false;
            while (!userSelected) {
                os_ResetFlag(SHIFT, ALPHALOCK);
                switch (os_GetKey()) {
                    case k_1:
                        usb = false;
                        quit = true;
                        userSelected = true;
                        break;
                    case k_2:
                        printStringAndMoveDownCentered("Please plug in a FAT32 formatted USB drive now.");
                        printStringAndMoveDownCentered("Do not unplug the drive until");
                        printStringAndMoveDownCentered("the save is loaded.");
                        printStringAndMoveDownCentered("Press any key to cancel loading from USB");
                        printStringAndMoveDownCentered("(Doing so will result in the genration of");
                        printStringAndMoveDownCentered("a new world)");
                        usb = true;
                        quit = true;
                        userSelected = true;
                        break;
                    case k_Clear:
                        goto load_save;
                        break;
                    default:
                        break;
                }
            }
        }
        toSaveOrNotToSave = checkSave(nameBuffer, usb);
        fillDirt();
        gfx_sprite_t* background2 = (gfx_sprite_t*)(cursorBackground->data + 6400);
        cursorBackground->width = 160;
        cursorBackground->height = 40;
        background2->width = 160;
        background2->height = 40;
        gfx_GetSprite(cursorBackground, 0, 0);
        gfx_GetSprite(background2, 160, 0);
        gfx_SetTextFGColor(254);
        gfx_SetTextScale(4, 4);
        gfx_SetTextXY(0, 0);
        printStringCentered("Loading...", 5);
        gfx_SetTextXY(0, 40);
        gfx_SetTextScale(1,1);
        printStringAndMoveDownCentered("Welcome to TICraft " TICRAFTversion "! While you wait:");
        printStringAndMoveDownCentered("Controls:");
        printStringAndMoveDownCentered("Graph/Clear: Exit/Emergency Exit");
        printStringAndMoveDownCentered("Cursor controls:");
        printStringAndMoveDownCentered("5: Place/Remove a block");
        printStringAndMoveDownCentered("8/2/4/6: Move forwards/backwards/left/right");
        printStringAndMoveDownCentered("Multiply/Subtract: Move up/down");
        printStringAndMoveDownCentered("9/1: Move diagonally forward/backward");
        printStringAndMoveDownCentered("7/3: Move diagonally left/right");
        printStringAndMoveDownCentered("Camera controls:");
        printStringAndMoveDownCentered("Up/Down: Move forwards/backwards");
        printStringAndMoveDownCentered("Left/Right: Move left/right");
        printStringAndMoveDownCentered("Del/Stat: Move up/down");
        printStringAndMoveDownCentered("Prgm/Cos/Sin/Tan: Rotate up/down/left/right");
        printStringAndMoveDownCentered("Math: Move to show the cursor");
        printStringAndMoveDownCentered("Enter: Show the block selection menu");
        printStringAndMoveDownCentered("Mode: Perform a full re-render of the screen");
        printStringAndMoveDownCentered("Made by Logan C.");
        resetCamera();
        if (toSaveOrNotToSave) {
            load();
        } else {
            playerCursor.x = cubeSize;
            playerCursor.y = cubeSize;
            playerCursor.z = cubeSize;
            selectedObject = 10;
            for (int i = 0; i < 1225 && i < maxNumberOfObjects; i++) {
                // workaround for compiler bug
                div_t xy = div(i, 35);
                objects[numberOfObjects] = new object(xy.rem*cubeSize, 0, xy.quot*cubeSize, 10, false);
                numberOfObjects++;
            }
        }
        fineMovement = false;
        outlineColor = 0;
        xSort();
        gfx_Sprite_NoClip(cursorBackground, 0, 0);
        gfx_Sprite_NoClip(background2, 160, 0);
        cursorBackground->height = 1;
        cursorBackground->width = 1;
        printStringAndMoveDownCentered("Press any key to begin.");
        gfx_SetTextScale(4,4);
        printStringCentered("Loaded!", 5);
        gfx_SetTextScale(1,1);
        os_GetKey();
        drawScreen();
        quit = false;
        while (!quit) {
            os_ResetFlag(SHIFT, ALPHALOCK);
            switch (os_GetKey()) {
                // forward
                case 0xFCE6:
                case k_9:
                    if (angleY < static_cast<Fixed24>(-67.5f*degRadRatio)) {
                        moveCursor(6);
                    } else if (angleY < static_cast<Fixed24>(-22.5f*degRadRatio)) {
                        moveCursor(2);
                    } else if (angleY < static_cast<Fixed24>(22.5f*degRadRatio)) {
                        moveCursor(8);
                    } else if (angleY < static_cast<Fixed24>(67.5f*degRadRatio)) {
                        moveCursor(4);
                    } else if (angleY < static_cast<Fixed24>(112.5f*degRadRatio)) {
                        moveCursor(7);
                    } else if (angleY < static_cast<Fixed24>(157.5f*degRadRatio)) {
                        moveCursor(3);
                    } else if (angleY < static_cast<Fixed24>(202.5f*degRadRatio)) {
                        moveCursor(9);
                    } else if (angleY < static_cast<Fixed24>(247.5f*degRadRatio)) {
                        moveCursor(5);
                    } else {
                        moveCursor(6);
                    }
                    break;
                // backward
                case 0xFCFB:
                case k_1:
                    if (angleY < static_cast<Fixed24>(-67.5f*degRadRatio)) {
                        moveCursor(7);
                    } else if (angleY < static_cast<Fixed24>(-22.5f*degRadRatio)) {
                        moveCursor(3);
                    } else if (angleY < static_cast<Fixed24>(22.5f*degRadRatio)) {
                        moveCursor(9);
                    } else if (angleY < static_cast<Fixed24>(67.5f*degRadRatio)) {
                        moveCursor(5);
                    } else if (angleY < static_cast<Fixed24>(112.5f*degRadRatio)) {
                        moveCursor(6);
                    } else if (angleY < static_cast<Fixed24>(157.5f*degRadRatio)) {
                        moveCursor(2);
                    } else if (angleY < static_cast<Fixed24>(202.5f*degRadRatio)) {
                        moveCursor(8);
                    } else if (angleY < static_cast<Fixed24>(247.5f*degRadRatio)) {
                        moveCursor(4);
                    } else {
                        moveCursor(7);
                    }
                    break;
                // left
                case 0xFCF2:
                case k_7:
                    if (angleY < static_cast<Fixed24>(-67.5f*degRadRatio)) {
                        moveCursor(9);
                    } else if (angleY < static_cast<Fixed24>(-22.5f*degRadRatio)) {
                        moveCursor(5);
                    } else if (angleY < static_cast<Fixed24>(22.5f*degRadRatio)) {
                        moveCursor(6);
                    } else if (angleY < static_cast<Fixed24>(67.5f*degRadRatio)) {
                        moveCursor(2);
                    } else if (angleY < static_cast<Fixed24>(112.5f*degRadRatio)) {
                        moveCursor(8);
                    } else if (angleY < static_cast<Fixed24>(157.5f*degRadRatio)) {
                        moveCursor(4);
                    } else if (angleY < static_cast<Fixed24>(202.5f*degRadRatio)) {
                        moveCursor(7);
                    } else if (angleY < static_cast<Fixed24>(247.5f*degRadRatio)) {
                        moveCursor(3);
                    } else {
                        moveCursor(9);
                    }
                    break;
                // right
                case 0xFCE4:
                case k_3:
                    if (angleY < static_cast<Fixed24>(-67.5f*degRadRatio)) {
                        moveCursor(8);
                    } else if (angleY < static_cast<Fixed24>(-22.5f*degRadRatio)) {
                        moveCursor(4);
                    } else if (angleY < static_cast<Fixed24>(22.5f*degRadRatio)) {
                        moveCursor(7);
                    } else if (angleY < static_cast<Fixed24>(67.5f*degRadRatio)) {
                        moveCursor(3);
                    } else if (angleY < static_cast<Fixed24>(112.5f*degRadRatio)) {
                        moveCursor(9);
                    } else if (angleY < static_cast<Fixed24>(157.5f*degRadRatio)) {
                        moveCursor(5);
                    } else if (angleY < static_cast<Fixed24>(202.5f*degRadRatio)) {
                        moveCursor(6);
                    } else if (angleY < static_cast<Fixed24>(247.5f*degRadRatio)) {
                        moveCursor(2);
                    } else {
                        moveCursor(8);
                    }
                    break;
                // up
                case 0xFCF3:
                case k_Mul:
                    moveCursor(0);
                    break;
                // down
                case 0xFCE7:
                case k_Sub:
                    moveCursor(1);
                    break;
                case 0xFCF8:
                case k_8:
                    if (angleY < static_cast<Fixed24>(-67.5f*degRadRatio)) {
                        moveCursor(5);
                    } else if (angleY < static_cast<Fixed24>(-22.5f*degRadRatio)) {
                        moveCursor(6);
                    } else if (angleY < static_cast<Fixed24>(22.5f*degRadRatio)) {
                        moveCursor(2);
                    } else if (angleY < static_cast<Fixed24>(67.5f*degRadRatio)) {
                        moveCursor(8);
                    } else if (angleY < static_cast<Fixed24>(112.5f*degRadRatio)) {
                        moveCursor(4);
                    } else if (angleY < static_cast<Fixed24>(157.5f*degRadRatio)) {
                        moveCursor(7);
                    } else if (angleY < static_cast<Fixed24>(202.5f*degRadRatio)) {
                        moveCursor(3);
                    } else if (angleY < static_cast<Fixed24>(247.5f*degRadRatio)) {
                        moveCursor(9);
                    } else {
                        moveCursor(5);
                    }
                    break;
                case 0xFCF4:
                case k_2:
                    if (angleY < static_cast<Fixed24>(-67.5f*degRadRatio)) {
                        moveCursor(4);
                    } else if (angleY < static_cast<Fixed24>(-22.5f*degRadRatio)) {
                        moveCursor(7);
                    } else if (angleY < static_cast<Fixed24>(22.5f*degRadRatio)) {
                        moveCursor(3);
                    } else if (angleY < static_cast<Fixed24>(67.5f*degRadRatio)) {
                        moveCursor(9);
                    } else if (angleY < static_cast<Fixed24>(112.5f*degRadRatio)) {
                        moveCursor(5);
                    } else if (angleY < static_cast<Fixed24>(157.5f*degRadRatio)) {
                        moveCursor(6);
                    } else if (angleY < static_cast<Fixed24>(202.5f*degRadRatio)) {
                        moveCursor(2);
                    } else if (angleY < static_cast<Fixed24>(247.5f*degRadRatio)) {
                        moveCursor(8);
                    } else {
                        moveCursor(4);
                    }
                    break;
                case 0xFCE2:
                case k_4:
                    if (angleY < static_cast<Fixed24>(-67.5f*degRadRatio)) {
                        moveCursor(3);
                    } else if (angleY < static_cast<Fixed24>(-22.5f*degRadRatio)) {
                        moveCursor(9);
                    } else if (angleY < static_cast<Fixed24>(22.5f*degRadRatio)) {
                        moveCursor(5);
                    } else if (angleY < static_cast<Fixed24>(67.5f*degRadRatio)) {
                        moveCursor(6);
                    } else if (angleY < static_cast<Fixed24>(112.5f*degRadRatio)) {
                        moveCursor(2);
                    } else if (angleY < static_cast<Fixed24>(157.5f*degRadRatio)) {
                        moveCursor(8);
                    } else if (angleY < static_cast<Fixed24>(202.5f*degRadRatio)) {
                        moveCursor(4);
                    } else if (angleY < static_cast<Fixed24>(247.5f*degRadRatio)) {
                        moveCursor(7);
                    } else {
                        moveCursor(3);
                    }
                    break;
                case 0xFCE5:
                case k_6:
                    if (angleY < static_cast<Fixed24>(-67.5f*degRadRatio)) {
                        moveCursor(2);
                    } else if (angleY < static_cast<Fixed24>(-22.5f*degRadRatio)) {
                        moveCursor(8);
                    } else if (angleY < static_cast<Fixed24>(22.5f*degRadRatio)) {
                        moveCursor(4);
                    } else if (angleY < static_cast<Fixed24>(67.5f*degRadRatio)) {
                        moveCursor(7);
                    } else if (angleY < static_cast<Fixed24>(112.5f*degRadRatio)) {
                        moveCursor(3);
                    } else if (angleY < static_cast<Fixed24>(157.5f*degRadRatio)) {
                        moveCursor(9);
                    } else if (angleY < static_cast<Fixed24>(202.5f*degRadRatio)) {
                        moveCursor(5);
                    } else if (angleY < static_cast<Fixed24>(247.5f*degRadRatio)) {
                        moveCursor(6);
                    } else {
                        moveCursor(2);
                    }
                    break;
                case k_Space:
                case k_5: {
                    // A linear search feels inefficient here but I guess it's fast enough
                    object** matchingObject = objects;
                    bool deletedObject = false;
                    drawBuffer();
                    while (matchingObject < objects + numberOfObjects) {
                        if (((*matchingObject)->x == playerCursor.x) && ((*matchingObject)->y == playerCursor.y) && ((*matchingObject)->z == playerCursor.z) && (is_visible(*matchingObject))) {
                            deletedObject = true;
                            break;
                        }
                        matchingObject++;
                    }
                    if (deletedObject) {
                        object* matchingObjectReference = *matchingObject;
                        memmove(matchingObject, matchingObject + 1, sizeof(object*) * (size_t)(objects + numberOfObjects - matchingObject - 1));
                        numberOfObjects--;
                        matchingObjectReference->deleteObject();
                    } else {
                        if (numberOfObjects < maxNumberOfObjects) {
                            object* newObject = new object(playerCursor.x, playerCursor.y, playerCursor.z, selectedObject, false);
                            newObject->findDistance();
                            matchingObject = objects;
                            while (matchingObject < objects + numberOfObjects) {
                                if (distanceCompare(matchingObject, &newObject) >= 0) {
                                    memmove(matchingObject + 1, matchingObject, sizeof(object*) * (size_t)(objects + numberOfObjects - matchingObject));
                                    break;
                                }
                                matchingObject++;
                            }
                            *matchingObject = newObject;
                            numberOfObjects++;
                            gfx_SetDrawBuffer();
                            newObject->generatePolygons();
                            gfx_SetDrawScreen();
                        }
                    }
                    getBuffer();
                    drawCursor();
                    break;
                }
                case 0x21:
                case k_Mode:
                    drawScreen();
                    break;
                // exit
                case k_Quit:
                case k_Graph:
                    gfx_SetDrawBuffer();
                    gfx_SetTextFGColor(254);
                    gfx_SetTextXY(0, 105);
                    fillDirt();
                    printStringAndMoveDownCentered("Would you like to save?");
                    printStringAndMoveDownCentered("Press 1 for yes, 2 for no.");
                    printStringAndMoveDownCentered("Or press graph again to return to the game.");
                    gfx_BlitBuffer();
                    gfx_SetDrawScreen();
                    userSelected = false;
                    while (!userSelected) {
                        os_ResetFlag(SHIFT, ALPHALOCK);
                        switch (os_GetKey()) {
                            case k_1:
                                save(nameBuffer);
                            case k_2:
                                userSelected = true;
                                quit = true;
                                break;
                            case k_Quit:
                            case k_Graph:
                                quit = false;
                                userSelected = true;
                                drawScreen();
                                break;
                            default:
                                break;
                        }
                    }
                    break;
                case 0x1C3D:
                case k_Clear:
                    quit = true;
                    emergencyExit = true;
                    break;
                case k_Enter:
                    selectBlock();
                    break;
                case k_Up:
                    if (fineMovement) {
                        cameraXYZ[0] += (Fixed24)10*sy;
                        cameraXYZ[2] += (Fixed24)10*cy;
                    } else {
                        cameraXYZ[0] += (Fixed24)40*sy;
                        cameraXYZ[2] += (Fixed24)40*cy;
                    }
                    redrawScreen();
                    break;
                case k_Down:
                    if (fineMovement) {
                        cameraXYZ[0] -= (Fixed24)10*sy;
                        cameraXYZ[2] -= (Fixed24)10*cy;
                    } else {
                        cameraXYZ[0] -= (Fixed24)40*sy;
                        cameraXYZ[2] -= (Fixed24)40*cy;
                    }
                    redrawScreen();
                    break;
                case k_Left:
                    if (fineMovement) {
                        cameraXYZ[0] -= (Fixed24)10*cy;
                        cameraXYZ[2] += (Fixed24)10*sy;
                    } else {
                        cameraXYZ[0] -= (Fixed24)40*cy;
                        cameraXYZ[2] += (Fixed24)40*sy;
                    }
                    redrawScreen();
                    break;
                case k_Right:
                    if (fineMovement) {
                        cameraXYZ[0] += (Fixed24)10*cy;
                        cameraXYZ[2] -= (Fixed24)10*sy;
                    } else {
                        cameraXYZ[0] += (Fixed24)40*cy;
                        cameraXYZ[2] -= (Fixed24)40*sy;
                    }
                    redrawScreen();
                    break;
                case 0xFE27:
                case k_Del:
                    if (fineMovement) {
                        cameraXYZ[1] += 5;
                    } else {
                        cameraXYZ[1] += 20;
                    }
                    redrawScreen();
                    break;
                case 0x83:
                case k_Stat:
                    if (fineMovement) {
                        cameraXYZ[1] -= 5;
                    } else {
                        cameraXYZ[1] -= 20;
                    }
                    redrawScreen();
                    break;
                case 0xFCEC:
                case k_Prgm:
                    if (fineMovement) {
                        rotateCamera(static_cast<Fixed24>(-2.5f*degRadRatio), 0);
                    } else {
                        rotateCamera(static_cast<Fixed24>(-10.0f*degRadRatio), 0);
                    }
                    break;
                case 0x8B:
                case k_Cos:
                    if (fineMovement) {
                        rotateCamera(static_cast<Fixed24>(2.5f*degRadRatio), 0);
                    } else {
                        rotateCamera(static_cast<Fixed24>(10.0f*degRadRatio), 0);
                    }
                    break;
                case 0xFCEE:
                case k_Sin:
                    if (fineMovement) {
                        rotateCamera(0, static_cast<Fixed24>(-2.5f*degRadRatio));
                    } else {
                        rotateCamera(0, static_cast<Fixed24>(-10.0f*degRadRatio));
                    }
                    break;
                case 0x8D:
                case k_Tan:
                    if (fineMovement) {
                        rotateCamera(0, static_cast<Fixed24>(2.5f*degRadRatio));
                    } else {
                        rotateCamera(0, static_cast<Fixed24>(10.0f*degRadRatio));
                    }
                    break;
                case 0xFE0A:
                case k_Math:
                    cameraXYZ[0] = (Fixed24)playerCursor.x - (((Fixed24)84.85281375f)*sy);
                    cameraXYZ[1] = (Fixed24)playerCursor.y + (Fixed24)75;
                    cameraXYZ[2] = (Fixed24)playerCursor.z - (((Fixed24)84.85281375f)*cy);
                    redrawScreen();
                    break;
                case 0x193D:
                case k_Window:
                    takeScreenshot();
                    break;
                case 0x1B3D:
                case k_Trace:
                    fineMovement = !fineMovement;
                    if (fineMovement) {
                        outlineColor = 252;
                    } else {
                        outlineColor = 0;
                    }
                    playerCursor.generatePolygons();
                    break;
                default:
                    break;
            }
        }
        deleteEverything();
    }
    exitOverlay();
}