#include <ti/getcsc.h>
#include <graphx.h>
#include <fileioc.h>
#include "printString.hpp"
#include "textures.hpp"
#include "renderer.hpp"
#include "saves.hpp"
#include "crc32.h"

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
    uint8_t handle;
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
        handle = ti_Open(name, "w+");
        if (handle) {
            bool saveGood = true;
            bool error = false;
            unsigned int version = saveFileVersion;
            Fixed24 cursorPos[3] = {playerCursor.x, playerCursor.y, playerCursor.z};
            uint32_t checksum;
            saveGood = ti_Write("BLOCKS", 7, 1, handle) == 1;
            if (saveGood) {
                saveGood = ti_Write(&version, sizeof(unsigned int), 1, handle) == 1;
            } else if (error == false) {
                error = true;
                printStringAndMoveDownCentered("Failed to write signature.");
            }
            if (saveGood) {
                saveGood = ti_Write(&numberOfObjects, sizeof(unsigned int), 1, handle) == 1;
            } else if (error == false) {
                error = true;
                printStringAndMoveDownCentered("Failed to write version.");
            }
            if (saveGood) {
                for (unsigned int i = 0; i < numberOfObjects; i++) {
                    cubeSave cube = {objects[i]->x, objects[i]->y, objects[i]->z, objects[i]->size, objects[i]->texture};
                    if (ti_Write(&cube, sizeof(cubeSave), 1, handle) != 1) {
                        saveGood = false;
                        break;
                    }
                }
            } else if (error == false) {
                error = true;
                printStringAndMoveDownCentered("Failed to write number of blocks.");
            }
            if (saveGood) {
                saveGood = ti_Write(&selectedObject, sizeof(uint8_t), 1, handle) == 1;
            } else if (error == false) {
                error = true;
                printStringAndMoveDownCentered("Failed to write blocks.");
            }
            if (saveGood) {
                saveGood = ti_Write(cameraXYZ, sizeof(Fixed24), 3, handle) == 3;
            } else if (error == false) {
                error = true;
                printStringAndMoveDownCentered("Failed to write selected object.");
            }
            if (saveGood) {
                saveGood = ti_Write(cursorPos, sizeof(Fixed24), 3, handle) == 3;
            } else if (error == false) {
                error = true;
                printStringAndMoveDownCentered("Failed to write camera pos.");
            }
            if (saveGood) {
                saveGood = ti_Seek(0, SEEK_SET, handle) != EOF;
            } else if (error == false) {
                error = true;
                printStringAndMoveDownCentered("Failed to write cursor pos.");
            }
            if (saveGood) {
                checksum = crc32((char*) ti_GetDataPtr(handle), ti_GetSize(handle));
                saveGood = ti_Seek(0, SEEK_END, handle) != EOF;
            } else if (error == false) {
                error = true;
                printStringAndMoveDownCentered("Failed to seek to beginning.");
            }
            if (saveGood) {
                saveGood = ti_Write(&checksum, sizeof(uint32_t), 1, handle) == 1;
            } else if (error == false) {
                error = true;
                printStringAndMoveDownCentered("Failed to seek to end.");
            }
            if (!saveGood && error == false) {
                error = true;
                printStringAndMoveDownCentered("Failed to write checksum.");
            }
            if (!saveGood && error) {
                failedToSave();
                ti_Close(handle);
                ti_Delete("WORLD");
            } else {
                ti_SetArchiveStatus(true, handle);
                ti_Close(handle);
            }
        }
        if (handle) {
            ti_Close(handle);
        }
    }
}

bool checkSave(const char* name) {
    bool toSaveOrNotToSave = true;
    bool userSelected = false;
    uint8_t handle = ti_Open(name, "r");
    if (handle) {
        toSaveOrNotToSave = true;
    } else {
        toSaveOrNotToSave = false;
    }
    if (toSaveOrNotToSave) {
        char signature[7];
        bool error = false;
        bool saveGood = ti_Read(signature, 7, 1, handle) == 1;
        unsigned int version;
        uint32_t checksum;
        uint32_t properChecksum;

        if (saveGood) {
            saveGood = strcmp(signature, "BLOCKS") == 0;
        } else if (error == false) {
            error = true;
            printStringAndMoveDownCentered("Bad signature read.");
        }
        if (saveGood) {
            saveGood = ti_Read(&version, sizeof(unsigned int), 1, handle) == 1;
        } else if (error == false) {
            error = true;
            printStringAndMoveDownCentered("Bad signature.");
        }
        if (saveGood) {
            saveGood = version == 1 || version == saveFileVersion;
        } else if (error == false) {
            error = true;
            printStringAndMoveDownCentered("Failed to read save file version.");
        }
        if (saveGood) {
            saveGood = ti_Seek(0, SEEK_SET, handle) != EOF;
        } else if (error == false) {
            error = true;
            printStringAndMoveDownCentered("Wrong save file version");
        }
        if (saveGood) {
            checksum = crc32((char*) ti_GetDataPtr(handle), ti_GetSize(handle) - sizeof(uint32_t));
            saveGood = ti_Seek(-4, SEEK_END, handle) != EOF;
        } else if (error == false) {
            error = true;
            printStringAndMoveDownCentered("Failed to seek to beginning.");
        }
        if (saveGood) {
            saveGood = ti_Read(&properChecksum, sizeof(uint32_t), 1, handle) == 1;
        } else if (error == false) {
            error = true;
            printStringAndMoveDownCentered("Failed to seek to end.");
        }
        if (saveGood) {
            saveGood = checksum == properChecksum;
        } else if (error == false) {
            error = true;
            printStringAndMoveDownCentered("Failed to read checksum.");
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
            ti_Close(handle);
            ti_Delete("WORLD");
        }
    }
    if (handle) {
        ti_Close(handle);
    }
    return toSaveOrNotToSave;
}

void load(const char* name) {
    uint8_t handle = ti_Open(name, "r");

    if (handle) {
        bool saveGood = true;
        bool error = false;
        unsigned int version;
        saveGood = ti_Seek(7, SEEK_SET, handle) != EOF;
        if (saveGood) {
            saveGood = ti_Read(&version, sizeof(unsigned int), 1, handle) == 1;
        } else if (error == false) {
            error = true;
        }
        if (saveGood) {
            saveGood = ti_Read(&numberOfObjects, sizeof(unsigned int), 1, handle) == 1;
        } else if (error == false) {
            error = true;
        }
        if (saveGood) {
            for (unsigned int i = 0; i < numberOfObjects && i < maxNumberOfObjects; i++) {
                cubeSave cube;
                if (ti_Read(&cube, sizeof(cubeSave), 1, handle) == 1) {
                    if (i < maxNumberOfObjects) {
                        objects[i] = new object(cube.x, cube.y, cube.z, cube.size, cube.texture, false);
                    }
                } else {
                    saveGood = false;
                    break;
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
            saveGood = ti_Read(&selectedObject, sizeof(uint8_t), 1, handle) == 1;
            if (saveGood) {
                saveGood = ti_Read(cameraXYZ, sizeof(Fixed24), 3, handle) == 3;
            } else if (error == false) {
                error = true;
            }
            if (saveGood) {
                saveGood = ti_Read(cursorPos, sizeof(Fixed24), 3, handle) == 3;
            } else if (error == false) {
                error = true;
            }
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
        ti_Close(handle);
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
    const char* saveNames[] = {"WORLD1","WORLD2","WORLD3","WORLD4","WORLD5","WORLD6","WORLD7","WORLD8","WORLD9","WORLD10","WORLD11","WORLD12","WORLD13","WORLD14","WORLD15","WORLD16","WORLD17","WORLD18","WORLD19","WORLD20","WORLD21","WORLD22","WORLD23","WORLD24","WORLD25","WORLD26","WORLD27","WORLD28","WORLD29","WORLD30","WORLD31","WORLD32","WORLD33","WORLD34","WORLD35","WORLD36","WORLD37","WORLD38","WORLD39","WORLD40","WORLD41","WORLD42","WORLD43","WORLD44","WORLD45","WORLD46","WORLD47","WORLD48","WORLD49","WORLD50","WORLD51","WORLD52","WORLD53","WORLD54","WORLD55","WORLD56","WORLD57","WORLD58","WORLD59","WORLD60","WORLD61","WORLD62","WORLD63","WORLD64","WORLD65","WORLD66","WORLD67","WORLD68","WORLD69","WORLD70","WORLD71","WORLD72","WORLD73","WORLD74","WORLD75","WORLD76","WORLD77","WORLD78","WORLD79","WORLD80","WORLD81","WORLD82","WORLD83","WORLD84","WORLD85","WORLD86","WORLD87","WORLD88","WORLD89","WORLD90","WORLD91","WORLD92","WORLD93","WORLD94","WORLD95","WORLD96","WORLD97","WORLD98","WORLD99", "WORLD100"};
    gfx_FillScreen(252);
    uint8_t selectedSave = 0;
    uint8_t offset = 0;
    for (unsigned int i = 0; i < 4; i++) {
        drawSaveOption(i, (i == selectedSave), saveNames[i + offset]);
    }
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
                    }
                    break;
                case sk_Down:
                    if (selectedSave < 3) {
                        drawSaveOption(selectedSave, false, saveNames[selectedSave + offset]);
                        selectedSave++;
                        drawSaveOption(selectedSave, true, saveNames[selectedSave + offset]);
                    }
                    break;
                case sk_Left:
                    if (offset > 3) {
                        offset -= 4;
                        gfx_FillScreen(252);
                        for (unsigned int i = 0; i < 4; i++) {
                            drawSaveOption(i, (i == selectedSave), saveNames[i + offset]);
                        }
                    }
                    break;
                case sk_Right:
                    if (offset < 96) {
                        offset += 4;
                        gfx_FillScreen(252);
                        for (unsigned int i = 0; i < 4; i++) {
                            drawSaveOption(i, (i == selectedSave), saveNames[i + offset]);
                        }
                    }
                    break;
                case sk_Enter:
                    strcpy(nameBuffer, saveNames[selectedSave + offset]);
                    return true;
                    quit = true;
                    break;
                case sk_Clear:
                    return false;
                    quit = true;
                    break;
                case sk_Graph:
                    return false;
                    quit = true;
                    break;
                case sk_Del:
                    gfx_FillScreen(252);
                    char buffer[100] = "Are you sure you'd like to delete ";
                    strcat(buffer, saveNames[selectedSave + offset]);
                    strcat(buffer, "?");
                    printStringCentered(buffer, 110);
                    printStringCentered("Press 1 for yes, 2 for no.", 120);
                    bool userSelected = false;
                    while (!userSelected) {
                        uint8_t key2 = os_GetCSC();
                        if (key2) {
                            switch (key2) {
                                case sk_1:
                                    userSelected = true;
                                    ti_Delete(saveNames[selectedSave + offset]);
                                    gfx_FillScreen(252);
                                    for (unsigned int i = 0; i < 4; i++) {
                                        drawSaveOption(i, (i == selectedSave), saveNames[i + offset]);
                                    }
                                    break;
                                case sk_2:
                                    userSelected = true;
                                    gfx_FillScreen(252);
                                    for (unsigned int i = 0; i < 4; i++) {
                                        drawSaveOption(i, (i == selectedSave), saveNames[i + offset]);
                                    }
                                    break;
                                default:
                                    break;

                            }
                        }
                    }
            }
        }
    }
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