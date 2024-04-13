#pragma once
#include <cstdint>
#include "renderer.hpp"

#define saveFileVersion 4
// hey we can just use the back buffer
#define saveDataBuffer 0xD52C00
#define saveBufferSize 76800
#define k_F5 0x3D

struct cubeSave {
    // X position of the cube
    Fixed24 x;

    // Y position of the cube
    Fixed24 y;

    // Z position of the cube
    Fixed24 z;

    // Size of the cube
    Fixed24 legacy_size;

    // An index into an array of pointers representing the texture of the cube
    uint8_t texture;
};

struct cubeSave_v2 {
    // X position of the cube
    int16_t x;

    // Y position of the cube
    int16_t y;

    // Z position of the cube
    int16_t z;

    // An index into an array of pointers representing the texture of the cube
    uint8_t texture;
};

/*
Save file format:
struct saveFile {
    char magic[7]; // BLOCKS
    unsigned int version = 4; // Version
    unsigned int numberOfObjects;
    cubeSave_v2 objects[numberOfObjects];
    uint8_t selectedObject;
    Fixed24[3] cameraPos;
    Fixed24[3] cursorPos;
    uint32_t checksum; // CRC32
};
*/

void gfxStart();
void failedToSave();
void failedToLoadSave();
void save(const char* name);
bool checkSave(const char* name, bool USB);
/*
LOADS WHATEVER IS IN THE SAVE BUFFER
Which should be the save file assuming you ran checkSave first
SO RUN checkSave FIRST AND DO NOT OVERWRITE THE SAVE BUFFER!!!
*/
void load();
bool mainMenu(char* nameBuffer, unsigned int nameBufferLength);
void drawSaveOption(unsigned int number, bool selectedSave, const char* name, bool drawBackground);
void redrawSaveOptions();
void fillDirt();
void takeScreenshot();