#include <cstring>
#include <cctype>
#include <graphx.h>
#include <fileioc.h>
#include <ti/getkey.h>
#include <ti/screen.h>
#include "texturePackMenu.hpp"
#include "renderer.hpp"
#include "crc32.h"
#include "sys.hpp"
extern "C" {
    #include "printString.h"
}

void texturePackError(const char* message) {
    gfx_End();
    os_ClrHomeFull();
    os_PutStrFull("Texture pack is invalid (");
    os_PutStrFull(message);
    os_PutStrFull("). Please select another texture pack or load on the texture pack again. Press any key to continue.");
    os_GetKey();
}

int texturePackCompare(const void *arg1, const void *arg2) {
    const char* name1 = ((packEntry*) arg1)->pack->metadata;
    const char* name2 = ((packEntry*) arg2)->pack->metadata;
    for (unsigned int i = 0; name1[i] && name2[i]; i++) {
        if (toupper(name1[i]) != toupper(name2[i])) {
            return toupper(name1[i]) - toupper(name2[i]);
        }
    }
    return strlen(name1) - strlen(name2);
}

void drawTexturePackSelection(texturePack* pack, int row, bool selected) {
    if (selected) {
        drawRectangle(0, (40*row)+40, 320, 40, 65535);
        os_SetDrawFGColor(0);
        os_SetDrawBGColor(65535);
    } else {
        drawRectangle(0, (40*row)+40, 320, 40, 0);
        os_SetDrawFGColor(65535);
        os_SetDrawBGColor(0);
    }
    drawImage(4, (40*row)+44, 32, 32, pack->icon);
    char nameString[32];
    strncpy(nameString, pack->metadata, 32);
    nameString[31] = 0;
    os_FontDrawText(nameString, 40, (40*row)+44);
}

bool verifyTexturePack(packEntry pack) {
    os_ClrHomeFull();
    os_PutStrFull("Verifying texture pack, please wait...");
    if (strcmp(pack.pack->magic, TICRAFTTexturePackMagic) != 0) {
        texturePackError("bad magic bytes");
        return false;
    }
    if (pack.pack->version != TICRAFTTexturePackVersion) {
        texturePackError("for the wrong version of TICRAFT");
        return false;
    }
    #define texturePackCRC *((uint32_t*)(((char*)pack.pack) + pack.size - sizeof(uint32_t)))
    #define trueCRC crc32((const char*)pack.pack, pack.size - sizeof(uint32_t))
    if (texturePackCRC != trueCRC) {
        texturePackError("bad checksum");
        return false;
    }
    return true;
}

void texturePackMenu() {
    memset(gfx_vram, 0, 320*240*sizeof(uint16_t));
    char* name;
    void* vat_ptr = nullptr;
    char** texturePackNames = (char**) malloc(0);
    unsigned int numberOfTexturePacks = 0;
    while ((name = ti_Detect(&vat_ptr, TICRAFTTexturePackMagic))) {
        uint8_t texturePackHandle = ti_Open(name, "r");
        ti_SetArchiveStatus(true, texturePackHandle);
        texturePack* pack = (texturePack*) ti_GetDataPtr(texturePackHandle);
        ti_Close(texturePackHandle);
        if (pack->version == TICRAFTTexturePackVersion) {
            texturePackNames = (char**) realloc(texturePackNames, sizeof(char*) * (numberOfTexturePacks + 1));
            // just remember to delete all of these strings when we're done
            texturePackNames[numberOfTexturePacks] = new char[strlen(name) + 1];
            strcpy(texturePackNames[numberOfTexturePacks], name);
            numberOfTexturePacks++;
        }
    }
    if (numberOfTexturePacks == 0) {
        texturePackError("none were found");
        exitOverlay();
    }
    packEntry* packs = new packEntry[numberOfTexturePacks];
    for (unsigned int i = 0; i < numberOfTexturePacks; i++) {
        uint8_t texturePackHandle = ti_Open(texturePackNames[i], "r");
        packs[i].pack = (texturePack*)ti_GetDataPtr(texturePackHandle);
        packs[i].filename = texturePackNames[i];
        packs[i].size = ti_GetSize(texturePackHandle);
        ti_Close(texturePackHandle);
    }
    free(texturePackNames);
    unsigned int offset = 0;
    unsigned int selectedPack = 0;
    bool quit = false;
    if (numberOfTexturePacks == 1) {
        if (verifyTexturePack(packs[0])) {
            goto texturePackEnd;
        }
        exitOverlay();
    }
    qsort(packs, numberOfTexturePacks, sizeof(packEntry), texturePackCompare);
    // here we make the actual menu
    os_FontSelect(os_LargeFont);
    os_SetDrawFGColor(65535);
    os_SetDrawBGColor(0);
    fontPrintString("Please select a", 4);
    fontPrintString("texture pack.", 23);
    while (!quit) {
        memset(gfx_vram + (12800*sizeof(uint16_t)), 0, 64000*sizeof(uint16_t));
        for (unsigned int i = 0; i + offset < numberOfTexturePacks && i < 5; i++) {
            drawTexturePackSelection(packs[i + offset].pack, i, i == selectedPack);
        }
        uint8_t key;
        bool quit2 = false;
        while (!quit2) {
            os_ResetFlag(SHIFT, ALPHALOCK);
            switch (os_GetKey()) {
                case 0xFC08:
                case k_5:
                case k_Enter:
                    quit2 = true;
                    if (verifyTexturePack(packs[selectedPack + offset])) {
                        quit = true;
                    }
                    break;
                case 0xFCF8:
                case k_Up:
                case k_8:
                    if (selectedPack > 0) {
                        drawTexturePackSelection(packs[selectedPack + offset].pack, selectedPack, false);
                        selectedPack--;
                        drawTexturePackSelection(packs[selectedPack + offset].pack, selectedPack, true);
                    }
                    break;
                case 0xFCF4:
                case k_Down:
                case k_2:
                    if (selectedPack + offset + 1 < numberOfTexturePacks && selectedPack < 4) {
                        drawTexturePackSelection(packs[selectedPack + offset].pack, selectedPack, false);
                        selectedPack++;
                        drawTexturePackSelection(packs[selectedPack + offset].pack, selectedPack, true);
                    }
                    break;
                case 0xFCE2:
                case k_Left:
                case k_4:
                    if (offset > 4) {
                        offset -= 5;
                        quit2 = true;
                    }
                    break;
                case 0xFCE5:
                case k_Right:
                case k_6:
                    if (numberOfTexturePacks > offset + 5) {
                        offset += 5;
                        if (selectedPack + offset >= numberOfTexturePacks) {
                            selectedPack = numberOfTexturePacks - offset - 1;
                        }
                        quit2 = true;
                    }
                    break;
                case 0x1C3D:
                case k_Graph:
                case k_Clear:
                    exitOverlay();
                default:
                    break;
            }
        }
    }
    texturePackEnd:
    texturePackName = new char[strlen(packs[selectedPack + offset].filename) + 1];
    strcpy(texturePackName, packs[selectedPack + offset].filename);
    for (unsigned int i = 0; i < numberOfTexturePacks; i++) {
        delete[] packs[i].filename;
    }
    delete[] packs;
}