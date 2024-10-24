/*
Based on code from the TI CE C/C++ toolchain
Licensed under the LGPLv3 https://github.com/CE-Programming/toolchain/blob/master/license
*/
#include <math.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include <fatdrvce.h>
#include <msddrvce.h>
#include <usbdrvce.h>
#include <graphx.h>
#include <ti/getcsc.h>
#include <ti/screen.h>
#include "usb.h"
#include "printString.h"

static msd_partition_t partitions[MAX_PARTITIONS];
extern gfx_sprite_t* cursorBackground;
static global_t* global;

usb_error_t handleUsbEvent(usb_event_t event, void *event_data, usb_callback_data_t *global) {
    switch (event) {
        case USB_DEVICE_DISCONNECTED_EVENT:
            if (global->usb) {
                // msd_Close(&global->msd);
                global->storageInit = false;
            }
            global->usb = NULL;
            break;
        case USB_DEVICE_CONNECTED_EVENT:
            return usb_ResetDevice(event_data);
        case USB_DEVICE_ENABLED_EVENT:
            global->usb = event_data;
            break;
        case USB_DEVICE_DISABLED_EVENT:
            return USB_USER_ERROR;
        default:
            break;
    }
    return USB_SUCCESS;
}

bool init_USB() {
    global = (global_t*)(cursorBackground->data);
    usb_error_t usberr;
    msd_error_t msderr;
    fat_error_t faterr;
    msd_info_t msdinfo;
    uint8_t num_partitions;
    memset(global, 0, sizeof(global_t));
    uint8_t key;
    global->usb = NULL;
    usberr = usb_Init((usb_event_callback_t) handleUsbEvent, global, NULL, USB_DEFAULT_INIT_FLAGS);
    while (!global->usb) {
        if (usberr != USB_SUCCESS) {
            return false;
        }
        if (os_GetCSC()) {
            return false;
        }
        usberr = usb_WaitForInterrupt();
    }
    if (msd_Open(&global->msd, global->usb)) {
        return false;
    }
    global->storageInit = true;
    if (msd_Info(&global->msd, &msdinfo)) {
        return false;
    }
    num_partitions = msd_FindPartitions(&global->msd, partitions, MAX_PARTITIONS);
    if (num_partitions < 1) {
        return false;
    }
    for (uint8_t i = 0; i < num_partitions; i++) {
        if (!fat_Open(&global->fat, (fat_read_callback_t) &msd_Read, (fat_write_callback_t) &msd_Write, &global->msd, partitions[i].first_lba)) {
            global->fatInit = true;
            return true;
        }
    }
    return false;
}

fat_file_t* openFile(const char* path, const char* name, bool create) {
    fat_error_t faterr;
    
    fat_file_t* file = calloc(1, sizeof(fat_file_t));
    char str[256];
    strncpy(str, path, 256);
    str[255] = 0;
    if (str[strlen(str) - 1] != '/') {
        strcat(str, "/");
    }
    strncat(str, name, 256-strlen(str));
    str[255] = 0;
    if (create) {
        fat_Create(&global->fat, path, name, 0);
    }
    if (fat_OpenFile(&global->fat, str, 0, file)) {
        /*printStringAndMoveDownCentered("Failed to open file");
        printStringAndMoveDownCentered(str);*/
        free(file);
        return NULL;
    }
    // cursed hack to add support for created/modified dates
    time_t currentTime;
    time(&currentTime);
    struct tm* currentLocalTime = localtime(&currentTime);
    uint16_t* entryPointer = *((uint16_t**)(&file->priv[40]));
    ((uint8_t*)entryPointer)[11] = FAT_ARCHIVE;
    ((uint8_t*)entryPointer)[13] = 0;
    entryPointer[11] = ((currentLocalTime->tm_sec)>>1) + (currentLocalTime->tm_min<<5) + (currentLocalTime->tm_hour<<11);
    entryPointer[9] = entryPointer[12] = (currentLocalTime->tm_mday) + ((currentLocalTime->tm_mon + 1) << 5) + ((currentLocalTime->tm_year - 80)<<9);
    if (create) {
        entryPointer[7] = entryPointer[11];
        entryPointer[8] = entryPointer[12];
    }
    return file;
}

void closeFile(fat_file_t* file) {
    if (file != NULL) {
        fat_CloseFile(file);
        free(file);
    }
}

bool readFile(fat_file_t* file, uint24_t bufferSize, void* buffer) {
    if (file == NULL) {
        return false;
    }
    uint32_t readSize = fat_GetFileSize(file);
    if (readSize > bufferSize - (bufferSize%MSD_BLOCK_SIZE) - MSD_BLOCK_SIZE) {
        readSize = bufferSize - (bufferSize%MSD_BLOCK_SIZE) - MSD_BLOCK_SIZE;
    }
    readSize = (readSize/MSD_BLOCK_SIZE)+1;
    fat_SetFileBlockOffset(file, 0);
    bool good = fat_ReadFile(file, readSize, buffer) == readSize;
    return good;
}

bool writeFile(fat_file_t* file, uint24_t size, void* buffer) {
    if (file == NULL) {
        return false;
    }
    int extraBlock = 0;
    div_t blockSize = div(size, MSD_BLOCK_SIZE);
    if (blockSize.rem != 0) {
        extraBlock = 1;
    }
    if (fat_SetFileSize(file, size)) {
        // printStringAndMoveDownCentered("Failed to set size");
        return false;
    }
    fat_SetFileBlockOffset(file, 0);
    bool good = fat_WriteFile(file, blockSize.quot + extraBlock, buffer) == blockSize.quot + extraBlock;
    // cursed hack to add support for created/modified dates
    time_t currentTime;
    time(&currentTime);
    struct tm* currentLocalTime = localtime(&currentTime);
    uint16_t* entryPointer = *((uint16_t**)(&file->priv[40]));
    ((uint8_t*)entryPointer)[11] = FAT_ARCHIVE;
    entryPointer[11] = ((currentLocalTime->tm_sec)>>1) + (currentLocalTime->tm_min<<5) + (currentLocalTime->tm_hour<<11);
    entryPointer[9] = entryPointer[12] = (currentLocalTime->tm_mday) + ((currentLocalTime->tm_mon + 1) << 5) + ((currentLocalTime->tm_year - 80)<<9);
    return good;
}

bool createDirectory(const char* path, const char* name) {
    fat_error_t faterr;
    faterr = fat_Create(&global->fat, path, name, FAT_DIR);
    if (faterr != FAT_SUCCESS && faterr != FAT_ERROR_EXISTS) {
        return false;
    }
    return true;
} 

int24_t getSizeOf(fat_file_t* file) {
    if (file == NULL) {
        return 0;
    }
    int24_t size = fat_GetFileSize(file);
    return size;
}

void deleteFile(const char* path, const char* name) {    
    char str[256];
    strncpy(str, path, 256);
    str[255] = 0; 
    if (str[strlen(str) - 1] != '/') {
        strncat(str, "/", 256-strlen(str));
    }
    strncat(str, name, 256-strlen(str));
    fat_Delete(&global->fat, str);
}

void close_USB() {
    if (global->fatInit == true) {
        fat_Close(&global->fat);
    }
    if (global->storageInit == true) {
        usb_WaitForEvents();
        msd_Close(&global->msd);
    }
    usb_Cleanup();
}