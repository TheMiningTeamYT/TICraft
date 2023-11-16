#include <ti/getcsc.h>
#include <graphx.h>
#include <fileioc.h>
#include "printString.hpp"
#include "textures.hpp"
#include "renderer.hpp"
#include "saves.hpp"
#include "crc32.h"
extern "C" {
    #include "usb.h"
}
// I want to include save file compression... just thinking about how, especially without breaking backwards compatibility.

uint8_t selectedObject = 10;
object playerCursor(20, 20, 20, 20, selectedObject, true);

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

void save(const char* name) {
    gfx_SetTextXY(0, 110);
    bool userSelected = false;
    bool toSaveOrNotToSave = true;
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
        uint8_t* saveData = (uint8_t*) saveDataBuffer;
        bool saveGood = true;
        bool error = false;
        unsigned int version = saveFileVersion;
        Fixed24 cursorPos[3] = {playerCursor.x, playerCursor.y, playerCursor.z};
        uint32_t checksum;
        // is using memcpy a bunch the best way to write this data out to memory? I DON'T KNOW!
        memcpy(saveData, "BLOCKS", 7);
        saveData += 7;
        memcpy(saveData, &version, sizeof(unsigned int));
        saveData += sizeof(unsigned int);
        memcpy(saveData, &numberOfObjects, sizeof(unsigned int));
        saveData += sizeof(unsigned int);
        for (unsigned int i = 0; i < numberOfObjects; i++) {
            cubeSave cube = {objects[i]->x, objects[i]->y, objects[i]->z, objects[i]->size, objects[i]->texture};
            memcpy(saveData, &cube, sizeof(cubeSave));
            saveData += sizeof(cubeSave);
        }
        memcpy(saveData, &selectedObject, 1);
        saveData += 1;
        memcpy(saveData, cameraXYZ, sizeof(Fixed24)*3);
        saveData += sizeof(Fixed24)*3;
        memcpy(saveData, cursorPos, sizeof(Fixed24)*3);
        saveData += sizeof(Fixed24)*3;
        checksum = crc32((char*) saveDataBuffer, (int)(saveData - (uint8_t*)saveDataBuffer));
        memcpy(saveData, &checksum, sizeof(uint32_t));
        saveData += sizeof(uint32_t);
        deleteEverything();
        bool quit = false;
        while (quit == false) {
            gfx_FillScreen(255);
            gfx_SetTextXY(0, 105);
            printStringAndMoveDownCentered("Would you like to archive or USB?");
            printStringAndMoveDownCentered("Press 1 for archive, press 2 for USB.");
            printStringAndMoveDownCentered("Or press clear to cancel.");
            uint8_t key = os_GetCSC();
            uint8_t handle;
            while (!key) {
                key = os_GetCSC();
            }
            char nameBuffer[128];
            switch (key) {
                case sk_1:
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
                case sk_2:
                    gfx_FillScreen(255);
                    gfx_SetTextXY(0, 110);
                    printStringAndMoveDownCentered("Please plug in a USB drive now.");
                    printStringAndMoveDownCentered("Press any key to cancel");
                    saveGood = init_USB();
                    if (saveGood) {
                        printStringAndMoveDownCentered("Please do not disconnect the USB drive.");
                        createDirectory("/", "saves");
                        strncpy(nameBuffer, name, 128);
                        nameBuffer[127] = 0;
                        strncat(nameBuffer, ".bin", 128-strlen(nameBuffer));
                        saveGood = writeFile("/saves", nameBuffer, (size_t)(saveData - (uint8_t*)saveDataBuffer), (void*) saveDataBuffer);
                    } else {
                        printStringAndMoveDownCentered("Failed to init USB.");
                        failedToSave();
                    }
                    close_USB();
                    if (saveGood) {
                        printStringAndMoveDownCentered("You may now remove the drive.");
                        printStringAndMoveDownCentered("Press any key to continue.");
                        while (!os_GetCSC());
                        quit = true;
                    }
                    break;
                case sk_Clear:
                    gfx_FillScreen(255);
                    gfx_SetTextXY(0, 110);
                    printStringAndMoveDownCentered("Are you sure you don't want to save?");
                    printStringAndMoveDownCentered("Press 1 for yes, 2 for no.");
                    key = os_GetCSC();
                    while (!key) {
                        key = os_GetCSC();
                    }
                    switch (key) {
                        case sk_1:
                            quit = true;
                            break;
                        default:
                            break;
                    }
                    break;
                default:
                    break;
            }
        }
    }
}

bool checkSave(const char* name, bool USB) {
    bool toSaveOrNotToSave = true;
    bool userSelected = false;
    uint8_t* saveData = (uint8_t*)saveDataBuffer;
    int fileSize = 0;
    if (USB) {
        if (init_USB()) {
            char nameBuffer[128];
            strncpy(nameBuffer, name, 128);
            nameBuffer[127] = 0;
            strncat(nameBuffer, ".bin", 128-strlen(nameBuffer));
            toSaveOrNotToSave = readFile("/saves", nameBuffer, saveBufferSize, saveData);
            if (toSaveOrNotToSave) {
                fileSize = getSizeOf("/saves", nameBuffer);
            }
        } else {
            toSaveOrNotToSave = false;
        }
        close_USB();
    } else {
        uint8_t handle = ti_Open(name, "r");
        if (handle) {
            toSaveOrNotToSave = ti_Read(saveData, 1, ti_GetSize(handle), handle) == ti_GetSize(handle);
            if (toSaveOrNotToSave) {
                fileSize = ti_GetSize(handle);
            }
            ti_Close(handle);
        } else {
            toSaveOrNotToSave = false;
        }
    }
    if (toSaveOrNotToSave) {
        char signature[7];
        bool error = false;
        unsigned int version;
        bool saveGood = true;
        uint32_t checksum;
        uint32_t properChecksum;
        if (saveGood) {
            memcpy(signature, saveData, 7);
            saveData += 7;
            saveGood = strcmp(signature, "BLOCKS") == 0;
            version = *((unsigned int*)saveData);
            saveData += sizeof(unsigned int);
            saveGood = version == 1 || version == saveFileVersion;
        } else if (error == false) {
            error = true;
            printStringAndMoveDownCentered("Bad read.");
        }
        if (saveGood) {
            checksum = crc32((char*) saveDataBuffer, fileSize - sizeof(uint32_t));
            saveData = ((uint8_t*) saveDataBuffer) + (fileSize - sizeof(uint32_t));
            properChecksum = *((uint32_t*)saveData);
            saveGood = checksum == properChecksum;
        } else if (error == false) {
            error = true;
            printStringAndMoveDownCentered("Wrong save file version");
        }
        if (saveGood) {
            toSaveOrNotToSave = true;
        } else if (error == false) {
            error = true;
            printStringAndMoveDownCentered("Bad checksum.");
        }
        if (!saveGood && error) {
            failedToLoadSave();
            toSaveOrNotToSave = false;
        }
    }
    return toSaveOrNotToSave;
}

void load(const char* name, bool USB) {
    uint8_t* saveData = (uint8_t*) saveDataBuffer;
    bool good;
    if (USB) {
        if (init_USB()) {
            char nameBuffer[128];
            strncpy(nameBuffer, name, 128);
            nameBuffer[127] = 0;
            strncat(nameBuffer, ".bin", 128-strlen(nameBuffer));
            good = readFile("/saves", nameBuffer, saveBufferSize, saveData);
        }
        close_USB();
    } else {
        uint8_t handle = ti_Open(name, "r");
        if (handle) {
            good = ti_Read(saveData, sizeof(uint8_t), ti_GetSize(handle), handle);
            ti_Close(handle);
        }
    }
    if (good) {
        bool saveGood = true;
        bool error = false;
        unsigned int version;
        saveGood = true;
        if (saveGood) {
            saveData += 7;
            version = *((unsigned int*)saveData);
            saveData += sizeof(unsigned int);
            numberOfObjects = *((unsigned int*)saveData);
            saveData += sizeof(unsigned int);
            for (unsigned int i = 0; i < numberOfObjects && i < maxNumberOfObjects; i++) {
                cubeSave cube = *((cubeSave*)saveData);
                saveData += sizeof(cubeSave);
                if (i < maxNumberOfObjects) {
                    objects[i] = new object(cube.x, cube.y, cube.z, cube.size, cube.texture, false);
                }
            }
        } else if (error == false) {
            error = true;
        }
        if (!saveGood) {
            error = true;
        }
        if (version == 2 && saveGood) {
            Fixed24 cursorPos[3];
            selectedObject = *saveData;
            saveData += 1;
            memcpy(cameraXYZ, saveData, sizeof(Fixed24)*3);
            saveData += sizeof(Fixed24)*3;
            memcpy(cursorPos, saveData, sizeof(Fixed24)*3);
            saveData += sizeof(Fixed24)*3;
            if (saveGood) {
                playerCursor.x = cursorPos[0];
                playerCursor.y = cursorPos[1];
                playerCursor.z = cursorPos[2];
            } else if (error == false) {
                error = true;
            }
        }
        if (!saveGood && error) {
            failedToLoadSave();
        }
    } else {
        failedToLoadSave();
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
    gfx_SetDrawBuffer();
    const char* saveNames[] = {"WORLD1","WORLD2","WORLD3","WORLD4","WORLD5","WORLD6","WORLD7","WORLD8","WORLD9","WORLD10","WORLD11","WORLD12","WORLD13","WORLD14","WORLD15","WORLD16","WORLD17","WORLD18","WORLD19","WORLD20","WORLD21","WORLD22","WORLD23","WORLD24","WORLD25","WORLD26","WORLD27","WORLD28","WORLD29","WORLD30","WORLD31","WORLD32","WORLD33","WORLD34","WORLD35","WORLD36","WORLD37","WORLD38","WORLD39","WORLD40","WORLD41","WORLD42","WORLD43","WORLD44","WORLD45","WORLD46","WORLD47","WORLD48","WORLD49","WORLD50","WORLD51","WORLD52","WORLD53","WORLD54","WORLD55","WORLD56","WORLD57","WORLD58","WORLD59","WORLD60","WORLD61","WORLD62","WORLD63","WORLD64","WORLD65","WORLD66","WORLD67","WORLD68","WORLD69","WORLD70","WORLD71","WORLD72","WORLD73","WORLD74","WORLD75","WORLD76","WORLD77","WORLD78","WORLD79","WORLD80","WORLD81","WORLD82","WORLD83","WORLD84","WORLD85","WORLD86","WORLD87","WORLD88","WORLD89","WORLD90","WORLD91","WORLD92","WORLD93","WORLD94","WORLD95","WORLD96","WORLD97","WORLD98","WORLD99", "WORLD100"};
    gfx_FillScreen(252);
    uint8_t selectedSave = 0;
    uint8_t offset = 0;
    for (unsigned int i = 0; i < 4; i++) {
        drawSaveOption(i, (i == selectedSave), saveNames[i + offset]);
    }
    gfx_BlitBuffer();
    bool quit = false;
    while (!quit) {
        uint8_t key = os_GetCSC();
        if (key) {
            switch (key) {
                case sk_Up:
                    if (selectedSave > 0) {
                        drawSaveOption(selectedSave, false, saveNames[selectedSave + offset]);
                        selectedSave--;
                        drawSaveOption(selectedSave, true, saveNames[selectedSave + offset]);
                        gfx_BlitBuffer();
                    }
                    break;
                case sk_Down:
                    if (selectedSave < 3) {
                        drawSaveOption(selectedSave, false, saveNames[selectedSave + offset]);
                        selectedSave++;
                        drawSaveOption(selectedSave, true, saveNames[selectedSave + offset]);
                        gfx_BlitBuffer();
                    }
                    break;
                case sk_Left:
                    if (offset > 3) {
                        offset -= 4;
                        gfx_FillScreen(252);
                        for (unsigned int i = 0; i < 4; i++) {
                            drawSaveOption(i, (i == selectedSave), saveNames[i + offset]);
                        }
                        gfx_BlitBuffer();
                    }
                    break;
                case sk_Right:
                    if (offset < 96) {
                        offset += 4;
                        gfx_FillScreen(252);
                        for (unsigned int i = 0; i < 4; i++) {
                            drawSaveOption(i, (i == selectedSave), saveNames[i + offset]);
                        }
                        gfx_BlitBuffer();
                    }
                    break;
                case sk_Enter:
                    strcpy(nameBuffer, saveNames[selectedSave + offset]);
                    gfx_SetDrawScreen();
                    return true;
                    break;
                case sk_Clear:
                    gfx_SetDrawScreen();
                    return false;
                    break;
                case sk_Graph:
                    gfx_SetDrawScreen();
                    return false;
                    break;
                case sk_Del:
                    gfx_FillScreen(252);
                    char buffer[100] = "Are you sure you'd like to delete ";
                    strcat(buffer, saveNames[selectedSave + offset]);
                    strcat(buffer, "?");
                    printStringCentered(buffer, 110);
                    printStringCentered("Press 1 for yes, 2 for no.", 120);
                    gfx_BlitBuffer();
                    bool userSelected = false;
                    while (!userSelected) {
                        uint8_t key2 = os_GetCSC();
                        if (key2) {
                            switch (key2) {
                                case sk_1:
                                    userSelected = true;
                                    quit = false;
                                    while (!quit) {
                                        gfx_FillScreen(252);
                                        printStringAndMoveDownCentered("Would you like to delete the save from USB or archive?");
                                        printStringAndMoveDownCentered("Press 1 for archive, 2 for USB.");
                                        gfx_BlitBuffer();
                                        uint8_t key = os_GetCSC();
                                        while (!key) {
                                            key = os_GetCSC();
                                        }
                                        switch (key) {
                                            case sk_1:
                                                ti_Delete(saveNames[selectedSave + offset]);
                                                quit = true;
                                                break;
                                            case sk_2:
                                                printStringAndMoveDownCentered("Please do not disconnect the USB drive.");
                                                printStringAndMoveDownCentered("Press any key to cancel.");
                                                gfx_BlitBuffer();
                                                if (init_USB()) {
                                                    char nameBuffer[128];
                                                    strncpy(nameBuffer, saveNames[selectedSave + offset], 128);
                                                    nameBuffer[127] = 0;
                                                    strncat(nameBuffer, ".bin", 128-strlen(nameBuffer));
                                                    deleteFile("/saves", nameBuffer);
                                                    quit = true;
                                                } else {
                                                    printStringAndMoveDownCentered("Failed to init USB.");
                                                    printStringAndMoveDownCentered("Press any key to continue.");
                                                    gfx_BlitBuffer();
                                                    uint8_t key = os_GetCSC();
                                                    while (!key) {
                                                        key = os_GetCSC();
                                                    }
                                                }
                                                close_USB();
                                                break;
                                            default:
                                                break;
                                        }
                                    }
                                    quit = false;
                                    gfx_FillScreen(252);
                                    for (unsigned int i = 0; i < 4; i++) {
                                        drawSaveOption(i, (i == selectedSave), saveNames[i + offset]);
                                    }
                                    gfx_BlitBuffer();
                                    break;
                                case sk_2:
                                    userSelected = true;
                                    gfx_FillScreen(252);
                                    for (unsigned int i = 0; i < 4; i++) {
                                        drawSaveOption(i, (i == selectedSave), saveNames[i + offset]);
                                    }
                                    gfx_BlitBuffer();
                                    break;
                                default:
                                    break;

                            }
                        }
                    }
            }
        }
    }
    gfx_SetDrawScreen();
    return false;
}

void drawSaveOption(unsigned int number, bool selectedSave, const char* name) {
    int y = 10 + (number*57) + (number % 2);
    if (number > 1) {
        y++;
    }
    if (selectedSave) {
        gfx_SetColor(254);
    } else {
        gfx_SetColor(252);
    }
    gfx_FillRectangle(5, y-5, 310, 57 + (number%2));
    gfx_SetColor(253);
    gfx_FillRectangle(10, y, 300, 47 + (number%2));
    printStringCentered(name, y + 16);
}