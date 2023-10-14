#pragma once
#include <cstdint>
#include "renderer.hpp"

#define saveFileVersion 2

void gfxStart();
void failedToSave();
void failedToLoadSave();
void save(const char* name);
bool checkSave(const char* name);
void load(const char* name);
bool mainMenu(char* nameBuffer, unsigned int nameBufferLength);
void drawSaveOption(unsigned int number, bool selectedSave, const char* name);
extern uint8_t selectedObject;
extern object playerCursor;