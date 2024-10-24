#include <ti/getkey.h>
#include <fatdrvce.h>
#include <graphx.h>
#include <fileioc.h>
#include "textures.hpp"
#include "renderer.hpp"
#include "saves.hpp"
#include "crc32.h"
#include "cursor.hpp"
#include "sincos.hpp"
#include <time.h>

extern "C" {
    #include "usb.h"
    #include "printString.h"
}
// I want to include save file compression... just thinking about how, especially without breaking backwards compatibility.
const char* const saveNames[] = {"WORLD1","WORLD2","WORLD3","WORLD4","WORLD5","WORLD6","WORLD7","WORLD8","WORLD9","WORLD10","WORLD11","WORLD12","WORLD13","WORLD14","WORLD15","WORLD16","WORLD17","WORLD18","WORLD19","WORLD20","WORLD21","WORLD22","WORLD23","WORLD24","WORLD25","WORLD26","WORLD27","WORLD28","WORLD29","WORLD30","WORLD31","WORLD32","WORLD33","WORLD34","WORLD35","WORLD36","WORLD37","WORLD38","WORLD39","WORLD40","WORLD41","WORLD42","WORLD43","WORLD44","WORLD45","WORLD46","WORLD47","WORLD48","WORLD49","WORLD50","WORLD51","WORLD52","WORLD53","WORLD54","WORLD55","WORLD56","WORLD57","WORLD58","WORLD59","WORLD60","WORLD61","WORLD62","WORLD63","WORLD64","WORLD65","WORLD66","WORLD67","WORLD68","WORLD69","WORLD70","WORLD71","WORLD72","WORLD73","WORLD74","WORLD75","WORLD76","WORLD77","WORLD78","WORLD79","WORLD80","WORLD81","WORLD82","WORLD83","WORLD84","WORLD85","WORLD86","WORLD87","WORLD88","WORLD89","WORLD90","WORLD91","WORLD92","WORLD93","WORLD94","WORLD95","WORLD96","WORLD97","WORLD98","WORLD99", "WORLD100"};
// hilarious how most of this header is zeros.
const unsigned char BMPheader[] = {'B', 'M', 54, 48, 1, 0, 0, 0, 0, 0, 54, 4, 0, 0, 40, 0, 0, 0, 64, 1, 0, 0, 240, 0, 0, 0, 1, 0, 8, 0, 0, 0, 0, 0, 0, 44, 1, 0, 35, 46, 0, 0, 35, 46, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0};
uint8_t selectedSave = 0;
uint8_t offset = 0;

void failedToSave() {
    printStringAndMoveDownCentered("Failed to save.");
    printStringAndMoveDownCentered("Press any key to continue.");
    os_GetKey();
}

void failedToLoadSave() {
    printStringAndMoveDownCentered("Failed to load save.");
    printStringAndMoveDownCentered("Press any key to continue.");
    os_GetKey();
}

void save(const char* name) {
    fillDirt();
    gfx_SetTextXY(0, 115);
    printStringAndMoveDownCentered("Generating save file, please wait...");
    uint8_t* saveData = (uint8_t*) saveDataBuffer;
    bool saveGood = true;
    bool error = false;
    Fixed24 cursorPos[3] = {playerCursor.x, playerCursor.y, playerCursor.z};
    // is this the best way to write this data out to memory? I DON'T KNOW!
    memcpy(saveData, "BLOCKS", 7);
    saveData += 7;
    *((unsigned int*)saveData) = saveFileVersion;
    saveData += sizeof(unsigned int);
    *((unsigned int*)saveData) = numberOfObjects;
    saveData += sizeof(unsigned int);
    for (unsigned int i = 0; i < numberOfObjects; i++) {
        *((cubeSave_v3*)saveData) = {static_cast<int8_t>(objects[i]->x/cubeSize), static_cast<int8_t>(objects[i]->y/cubeSize), static_cast<int8_t>(objects[i]->z/cubeSize), objects[i]->texture};
        saveData += sizeof(cubeSave_v3);
    }
    *saveData = selectedObject;
    saveData += 1;
    memcpy(saveData, cameraXYZ, sizeof(Fixed24)*3);
    saveData += sizeof(Fixed24)*3;
    memcpy(saveData, cursorPos, sizeof(Fixed24)*3);
    saveData += sizeof(Fixed24)*3;
    *((Fixed24*)saveData) = angleX;
    saveData += sizeof(Fixed24);
    *((Fixed24*)saveData) = angleY;
    saveData += sizeof(Fixed24);
    *((uint32_t*)saveData) = crc32((char*) saveDataBuffer, (int)(saveData - (uint8_t*)saveDataBuffer));
    saveData += sizeof(uint32_t);
    deleteEverything();
    bool quit = false;
    while (!quit) {
        fillDirt();
        gfx_SetTextXY(0, 105);
        printStringAndMoveDownCentered("Would you like to save to archive or USB?");
        printStringAndMoveDownCentered("Press 1 for archive, press 2 for USB.");
        printStringAndMoveDownCentered("Or press clear to cancel.");
        uint8_t handle;
        char nameBuffer[32];
        bool userSelected = false;
        while (!userSelected) {
            os_ResetFlag(SHIFT, ALPHALOCK);
            switch (os_GetKey()) {
                case k_1:
                    userSelected = true;
                    handle = ti_Open(name, "w+");
                    if (handle) {
                        saveGood = ti_Write((void*)saveDataBuffer, 1, (size_t)(saveData - (uint8_t*)saveDataBuffer), handle) == (size_t)(saveData - (uint8_t*)saveDataBuffer);
                        if (saveGood) {
                            ti_SetArchiveStatus(true, handle);
                            ti_Close(handle);
                            quit = true;
                        } else {
                            ti_Close(handle);
                            ti_Delete(name);
                            printStringAndMoveDownCentered("Failed to write save");
                            failedToSave();
                        }
                        ti_Close(handle);
                    }
                    break;
                case k_2:
                    userSelected = true;
                    fillDirt();
                    gfx_SetTextXY(0, 110);
                    printStringAndMoveDownCentered("Please plug in a FAT32 formatted USB drive now.");
                    printStringAndMoveDownCentered("Press any key to cancel");
                    saveGood = init_USB();
                    if (saveGood) {
                        printStringAndMoveDownCentered("Please do not disconnect the USB drive.");
                        createDirectory("/", "SAVES");
                        strcpy(nameBuffer, name);
                        strcat(nameBuffer, ".SAV");
                        fat_file_t* file = openFile("/SAVES", nameBuffer, true);
                        if (file == nullptr) {
                            saveGood = false;
                        } else {
                            saveGood = writeFile(file, (size_t)(saveData - (uint8_t*)saveDataBuffer), (void*) saveDataBuffer);
                            closeFile(file);
                        }
                    } else {
                        printStringAndMoveDownCentered("Failed to init USB.");
                        failedToSave();
                    }
                    close_USB();
                    if (saveGood) {
                        printStringAndMoveDownCentered("You may now remove the drive.");
                        printStringAndMoveDownCentered("Press any key to continue.");
                        os_GetKey();
                        quit = true;
                    }
                    break;
                case k_Clear:
                    fillDirt();
                    gfx_SetTextXY(0, 110);
                    printStringAndMoveDownCentered("Are you sure you don't want to save?");
                    printStringAndMoveDownCentered("Press 1 for yes, 2 for no.");
                    while (!userSelected) {
                        os_ResetFlag(SHIFT, ALPHALOCK);
                        switch (os_GetKey()) {
                            case k_1:
                                quit = true;
                            case k_2:
                                userSelected = true;
                                break;
                            default:
                                break;
                        }
                    }
                    break;
                default:
                    break;
            }
        }
    }
}

bool checkSave(const char* name, bool USB) {
    gfx_SetTextFGColor(254);
    bool toSaveOrNotToSave = true;
    bool userSelected = false;
    char* saveData = reinterpret_cast<char*>(saveDataBuffer);
    int fileSize = 0;
    bool fileFound = true;
    if (USB) {
        if (init_USB()) {
            char nameBuffer[32];
            strcpy(nameBuffer, name);
            strcat(nameBuffer, ".SAV");
            fat_file_t* file = openFile("/SAVES", nameBuffer, false);
            if (!file) {
                toSaveOrNotToSave = false;
                fileFound = false;
            } else {
                fileSize = getSizeOf(file);
                if (fileSize > saveBufferSize) {
                    printStringAndMoveDownCentered("Save file too big.");
                    toSaveOrNotToSave = false;
                } else {
                    toSaveOrNotToSave = readFile(file, saveBufferSize, saveData);
                }
                closeFile(file);
            }
        } else {
            toSaveOrNotToSave = false;
        }
        close_USB();
    } else {
        uint8_t handle = ti_Open(name, "r");
        if (handle) {
            fileSize = ti_GetSize(handle);
            if (fileSize > saveBufferSize) {
                printStringAndMoveDownCentered("Save file too big.");
                toSaveOrNotToSave = false;
            } else {
                toSaveOrNotToSave = ti_Read(saveData, 1, fileSize, handle) == fileSize;
            }
            ti_Close(handle);
        } else {
            toSaveOrNotToSave = false;
            fileFound = false;
        }
    }
    if (toSaveOrNotToSave) {
        if (strcmp((char*)saveData, "BLOCKS") != 0) {
            printStringAndMoveDownCentered("Bad magic bytes.");
            failedToLoadSave();
            return false;
        }
        saveData += 7;
        if (*((unsigned int*)saveData) > saveFileVersion) {
            printStringAndMoveDownCentered("Wrong save file version.");
            failedToLoadSave();
            return false;
        }
        saveData += sizeof(unsigned int);
        if (*((unsigned int*)saveData) > maxNumberOfObjects) {
            printStringAndMoveDownCentered("Save file contains too many blocks.");
            failedToLoadSave();
            return false;
        }
        saveData = ((char*) saveDataBuffer) + (fileSize - sizeof(uint32_t));
        if (crc32((char*) saveDataBuffer, fileSize - sizeof(uint32_t)) != *((uint32_t*)saveData)) {
            printStringAndMoveDownCentered("Bad checksum.");
            failedToLoadSave();
            return false;
        }
        return true;
    }
    if (fileFound) {
        failedToLoadSave();
    }
    return false;
}

void load() {
    uint8_t* saveData = ((uint8_t*)saveDataBuffer) + 7;
    unsigned int version = *((unsigned int*)saveData);
    saveData += sizeof(unsigned int);
    numberOfObjects = *((unsigned int*)saveData);
    saveData += sizeof(unsigned int);
    size_t cubeSaveSize = sizeof(cubeSave_v3);
    if (version < 4) {
        cubeSaveSize = sizeof(cubeSave);
    } else if (version < 5) {
        cubeSaveSize = sizeof(cubeSave_v2);
    }
    // Version 4 save files were first introduced in TICraft v1.1.0
    // Version 3 and below save files are deprecated and support for them will be removed in the future
    for (unsigned int i = 0; i < numberOfObjects; i++) {
        if (version <= 3) {
            objects[i] = new object(((cubeSave*)saveData)->x, ((cubeSave*)saveData)->y, ((cubeSave*)saveData)->z, ((cubeSave*)saveData)->texture, false);
        } else if (version == 4) {
            objects[i] = new object(((cubeSave_v2*)saveData)->x, ((cubeSave_v2*)saveData)->y, ((cubeSave_v2*)saveData)->z, ((cubeSave_v2*)saveData)->texture, false);
        } else {
            objects[i] = new object(static_cast<int>(((cubeSave_v3*)saveData)->x)*cubeSize, static_cast<int>(((cubeSave_v3*)saveData)->y)*cubeSize, static_cast<int>(((cubeSave_v3*)saveData)->z)*cubeSize, ((cubeSave_v3*)saveData)->texture, false);
        }
        saveData += cubeSaveSize;
    }
    if (version >= 2) {
        selectedObject = *saveData;
        saveData += sizeof(uint8_t);
        memcpy(cameraXYZ, saveData, sizeof(Fixed24)*3);
        saveData += sizeof(Fixed24)*3;
        playerCursor.x = *((Fixed24*)saveData);
        saveData += sizeof(Fixed24);
        playerCursor.y = *((Fixed24*)saveData);
        saveData += sizeof(Fixed24);
        playerCursor.z = *((Fixed24*)saveData);
        saveData += sizeof(Fixed24);
        if (version >= 3) {
            if (version >= 5) {
                angleX = *((Fixed24*)saveData);
                angleY = *(((Fixed24*)saveData) + 1);
            } else {
                angleX = *((float*)saveData)*degRadRatio;
                angleY = *(((float*)saveData) + 1)*degRadRatio;
            }
            cx = fastSinCos::cos(angleX);
            cxd = cx*(Fixed24)cubeSize;
            sx = fastSinCos::sin(angleX);
            nsxd = sx*(Fixed24)cubeSize;
            cy = fastSinCos::cos(angleY);
            cyd = cy*(Fixed24)cubeSize;
            sy = fastSinCos::sin(angleY);
            nsyd = sy*(Fixed24)-cubeSize;
            cxsy = cx*sy;
            cxsyd = cxsy*(Fixed24)cubeSize;
            cxcy = cx*cy;
            cxcyd = cxcy*(Fixed24)cubeSize;
            nsxsy = -sx*sy;
            nsxsyd = nsxsy*(Fixed24)cubeSize;
            nsxcy = -sx*cy;
            nsxcyd = nsxcy*(Fixed24)cubeSize;
        }
    }
}

void gfxStart() {
    gfx_Begin();
    gfx_SetTextFGColor(0);
    gfx_SetTextBGColor(255);
    gfx_SetTextScale(1, 1);
    initPalette();
}

bool mainMenu(char* nameBuffer, unsigned int nameBufferLength) {
    if (nameBufferLength < 9) {
        return false;
    }
    offset = 0;
    selectedSave = 0;
    gfx_SetDrawBuffer();
    redrawSaveOptions();
    bool quit = false;
    while (!quit) {
        os_ResetFlag(SHIFT, ALPHALOCK);
        switch (os_GetKey()) {
            case 0xFCF8:
            case k_8:
            case k_Up:
                drawSaveOption(selectedSave, false, saveNames[selectedSave + offset], true);
                if (selectedSave > 0) {
                    selectedSave--;
                } else {
                    selectedSave += 3;
                }
                drawSaveOption(selectedSave, true, saveNames[selectedSave + offset], false);
                gfx_BlitBuffer();
                break;
            case 0xFCF4:
            case k_2:
            case k_Down:
                drawSaveOption(selectedSave, false, saveNames[selectedSave + offset], true);
                if (selectedSave < 3) {
                    selectedSave++;
                } else {
                    selectedSave -= 3;
                }
                drawSaveOption(selectedSave, true, saveNames[selectedSave + offset], false);
                gfx_BlitBuffer();
                break;
            case 0xFCE2:
            case k_4:
            case k_Left:
                if (offset > 3) {
                    offset -= 4;
                } else {
                    offset += 96;
                }
                redrawSaveOptions();
                break;
            case 0xFCE5:
            case k_6:
            case k_Right:
                if (offset < 96) {
                    offset += 4;
                } else {
                    offset -= 96;
                }
                redrawSaveOptions();
                break;
            case 0xFB08:
            case k_5:
            case k_Enter:
                strcpy(nameBuffer, saveNames[selectedSave + offset]);
                gfx_SetDrawScreen();
                return true;
                break;
            case k_Quit:
            case 0x1C3D:
            case k_Clear:
            case k_Graph:
                gfx_SetDrawScreen();
                return false;
                break;
            case k_Del:
                gfx_SetTextFGColor(254);
                fillDirt();
                char buffer[100] = "Are you sure you'd like to delete ";
                strcat(buffer, saveNames[selectedSave + offset]);
                strcat(buffer, "?");
                printStringCentered(buffer, 110);
                printStringCentered("Press 1 for yes, 2 for no.", 120);
                gfx_BlitBuffer();
                bool userSelected = false;
                while (!userSelected) {
                    os_ResetFlag(SHIFT, ALPHALOCK);
                    switch (os_GetKey()) {
                        case k_1:
                            userSelected = true;
                            quit = false;
                            while (!quit) {
                                fillDirt();
                                gfx_SetTextXY(0, 100);
                                printStringAndMoveDownCentered("Would you like to delete the save from");
                                printStringAndMoveDownCentered("archive or USB?");
                                printStringAndMoveDownCentered("Press 1 for archive, 2 for USB.");
                                printStringAndMoveDownCentered("Or press clear to cancel.");
                                gfx_BlitBuffer();
                                os_ResetFlag(SHIFT, ALPHALOCK);
                                switch (os_GetKey()) {
                                    case k_1:
                                        ti_Delete(saveNames[selectedSave + offset]);
                                        quit = true;
                                        break;
                                    case k_2:
                                        gfx_SetTextXY(0, 110);
                                        printStringAndMoveDownCentered("Please plug in a FAT32 formatted USB drive now.");
                                        printStringAndMoveDownCentered("Press any key to cancel");
                                        gfx_BlitBuffer();
                                        if (init_USB()) {
                                            printStringAndMoveDownCentered("Please do not disconnect the USB drive.");
                                            char nameBuffer[32];
                                            strcpy(nameBuffer, saveNames[selectedSave + offset]);
                                            strcat(nameBuffer, ".SAV");
                                            deleteFile("/SAVES", nameBuffer);
                                            quit = true;
                                        } else {
                                            printStringAndMoveDownCentered("Failed to init USB.");
                                            printStringAndMoveDownCentered("Press any key to continue.");
                                            gfx_BlitBuffer();
                                            os_GetKey();
                                        }
                                        close_USB();
                                        break;
                                    case k_Clear:
                                        quit = true;
                                        break;
                                    default:
                                        break;
                                }
                            }
                            quit = false;
                            redrawSaveOptions();
                            break;
                        case k_2:
                            userSelected = true;
                            redrawSaveOptions();
                            break;
                        default:
                            break;

                    }
                }
        }
    }
    gfx_SetDrawScreen();
    return false;
}

void drawSaveOption(unsigned int number, bool selectedSave, const char* name, bool drawBackground) {
    int y = 10 + (number*57) + (number % 2);
    if (number > 1) {
        y++;
    }
    if (drawBackground) {
        gfx_Sprite_NoClip(cursorBackground, 110, y);
    }
    if (selectedSave) {
        gfx_SetTextFGColor(254);
        gfx_GetSprite(cursorBackground, 110, y);
        gfx_SetColor(253);
    } else {
        gfx_SetTextFGColor(0);
    }
    printStringCentered(name, y + 16);
}

void fillDirt() {
    cursorBackground->width = 16;
    cursorBackground->height = 16;
    memcpy(cursorBackground->data, dirt_texture[0], 256);
    for (unsigned int i = 0; i < 256; i++) {
        cursorBackground->data[i] += 126;
    }
    for (unsigned int x = 0; x < 320; x += 16) {
        for (uint8_t y = 0; y < 240; y += 16) {
            gfx_Sprite_NoClip(cursorBackground, x, y);
        }
    }
}

void redrawSaveOptions() {
    fillDirt();
    for (unsigned int i = 0; i < 4; i++) {
        drawSaveOption(i, (i == selectedSave), saveNames[i + offset], false);
    }
    gfx_BlitBuffer();
}

void takeScreenshot() {
    drawBuffer();
    for (uint8_t i = 0; i < 240; i++) {
        memcpy(gfx_vram + (LCD_WIDTH*LCD_HEIGHT) + (LCD_WIDTH*((LCD_HEIGHT-1)-i)), gfx_vram + (LCD_WIDTH*i), LCD_WIDTH);
    }
    fillDirt();
    gfx_SetTextFGColor(254);
    gfx_SetTextXY(0, 105);
    printStringAndMoveDownCentered("Please plug in a FAT32 formatted USB drive");
    printStringAndMoveDownCentered("to save the screenshot to now.");
    printStringAndMoveDownCentered("Press any key to cancel");
    char name[42] = "Screenshot saved as \"SHOTS\\";
    bool good = false;
    if (init_USB()) {
        printStringAndMoveDownCentered("Please do not disconnect the USB drive.");
        time_t currentTime;
        time(&currentTime);
        tm* currentLocalTime = localtime(&currentTime);
        // Untested
        strftime(name + 27, 15, "%H%M-%j.BMP", currentLocalTime);
        // actual header data is 1078B
        unsigned char* headerBuffer = cursorBackground->data + sizeof(global_t);
        memcpy(headerBuffer, BMPheader, 54);
        for (unsigned int i = 0; i < 256; i++) {
            headerBuffer[(i*4) + 54] = (gfx_palette[i] & 0x001F)<<3;
            headerBuffer[(i*4) + 55] = (gfx_palette[i] & 0x03E0)>>2;
            headerBuffer[(i*4) + 56] = (gfx_palette[i] & 0x7C00)>>7;
            headerBuffer[(i*4) + 57] = 0;
        }
        memcpy(headerBuffer + 1078, gfx_vram + (LCD_WIDTH*LCD_HEIGHT), 458);
        createDirectory("/", "SHOTS");
        fat_file_t* screenshot = openFile("/SHOTS", name + 27, true);
        if (screenshot) {
            fat_SetFileSize(screenshot, 77878);
            good = fat_WriteFile(screenshot, 3, headerBuffer) == 3;
            if (good) {
                printStringAndMoveDownCentered("Writing image data");
                good = fat_WriteFile(screenshot, 150, gfx_vram + (LCD_WIDTH*LCD_HEIGHT) + 458) == 150;
            } else {
                printStringAndMoveDownCentered("Failed to write header.");
            }
            closeFile(screenshot);
        } else {
            printStringAndMoveDownCentered("Failed to open file.");
        }
    } else {
        printStringAndMoveDownCentered("Failed to init USB.");
    }
    if (good) {
        // Untested
        name[39] = '\"';
        name[40] = '.';
        name[41] = 0;
        printStringAndMoveDownCentered(name);
        printStringAndMoveDownCentered("You may now remove the drive.");
    } else {
        printStringAndMoveDownCentered("Failed to write screenshot.");
        printStringAndMoveDownCentered(name + 27);
    }
    printStringAndMoveDownCentered("Press any key to continue.");
    close_USB();
    os_GetKey();
    drawScreen();
}