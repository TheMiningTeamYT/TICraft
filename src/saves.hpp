#pragma once
#include <cstdint>
#include "renderer.hpp"

#define saveFileVersion 3
// hey we can just use the back buffer
#define saveDataBuffer 0xD52C00
#define saveBufferSize 76800

void gfxStart();
void failedToSave();
void failedToLoadSave();
void save(const char* name);
bool checkSave(const char* name, bool USB);
void load(const char* name, bool USB);
bool mainMenu(char* nameBuffer, unsigned int nameBufferLength);
void drawSaveOption(unsigned int number, bool selectedSave, const char* name, bool drawBackground);
void redrawSaveOptions();
extern uint8_t selectedObject;
extern object playerCursor;
void fillDirt();