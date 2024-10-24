#pragma once
#include <cstdint>
#include <graphx.h>
#include "renderer.hpp"

extern uint8_t selectedObject;
extern object playerCursor;
extern gfx_sprite_t* cursorBackground;
extern int playerCursorX;
extern int playerCursorY;

void drawCursor(bool drawBuff);
void moveCursor(uint8_t direction);
void getBuffer();
void drawBuffer();
object** cursorOnObject();