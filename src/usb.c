/*
Based on code from the TI CE C/C++ toolchain
Licensed under the LGPLv3 https://github.com/CE-Programming/toolchain/blob/master/license
*/
#include <fatdrvce.h>
#include <msddrvce.h>
#include <usbdrvce.h>
#include <string.h>
#include <ti/getcsc.h>
#include <ti/screen.h>
#include <stdlib.h>
#include "usb.h"
#include "printString.h"
#include <math.h>
#include <graphx.h>

static msd_partition_t partitions[MAX_PARTITIONS];
extern gfx_sprite_t* cursorBackground;
static global_t* global;

usb_error_t handleUsbEvent(usb_event_t event, void *event_data, usb_callback_data_t *global) {
    switch (event) {
        case USB_DEVICE_DISCONNECTED_EVENT:
            if (global->usb) {
                msd_Close(&global->msd);
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
    global = ((global_t*)cursorBackground);
    usb_error_t usberr;
    msd_error_t msderr;
    fat_error_t faterr;
    msd_info_t msdinfo;
    uint8_t num_partitions;
    memset(global, 0, sizeof(global_t));
    uint8_t key;
    global->usb = NULL;
    usberr = usb_Init((usb_event_callback_t) handleUsbEvent, global, NULL, USB_DEFAULT_INIT_FLAGS);
    while (usberr == USB_SUCCESS) {
        if (global->usb != NULL) break;
        key = os_GetCSC();
        if (key != 0) {
            return false;
        }
        usberr = usb_WaitForInterrupt();
    }
    if (usberr != USB_SUCCESS) {
        return false;
    }
    msderr = msd_Open(&global->msd, global->usb);
    if (msderr != MSD_SUCCESS) {
        return false;
    }
    global->storageInit = true;
    msderr = msd_Info(&global->msd, &msdinfo);
    if (msderr != MSD_SUCCESS) {
        return false;
    }
    num_partitions = msd_FindPartitions(&global->msd, partitions, MAX_PARTITIONS);
    if (num_partitions < 1) {
        return false;
    }
    for (uint8_t i = 0; i < num_partitions; i++) {
        uint32_t base_lba = partitions[i].first_lba;
        fat_callback_usr_t* usr = &global->msd;
        fat_read_callback_t read = (fat_read_callback_t) &msd_Read;
        fat_write_callback_t write = (fat_write_callback_t) &msd_Write;
        faterr = fat_Open(&global->fat, read, write, usr, base_lba);
        if (faterr == FAT_SUCCESS) {
            global->fatInit = true;
            return true;
        }
    }
    return false;
}

fat_file_t* openFile(const char* path, const char* name) {
    fat_error_t faterr;
    usb_WaitForEvents();
    fat_file_t* file = malloc(sizeof(fat_file_t));
    char str[256];
    strncpy(str, path, 256);
    str[255] = 0;
    if (str[strlen(str) - 1] != '/') {
        strcat(str, "/");
    }
    strncat(str, name, 256-strlen(str));
    str[255] = 0;
    fat_Create(&global->fat, path, name, 0);
    faterr = fat_OpenFile(&global->fat, str, 0, file);
    if (faterr != FAT_SUCCESS) {
        /*printStringAndMoveDownCentered("Failed to open file");
        printStringAndMoveDownCentered(str);*/
        free(file);
        return NULL;
    }
    return file;
}

void closeFile(fat_file_t* file) {
    usb_WaitForEvents();
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
    fat_error_t faterr;
    if (file == NULL) {
        return false;
    }
    int extraBlock = 0;
    div_t blockSize = div(size, MSD_BLOCK_SIZE);
    if (blockSize.rem != 0) {
        extraBlock = 1;
    }
    faterr = fat_SetFileSize(file, size);
    if (faterr != FAT_SUCCESS) {
        // printStringAndMoveDownCentered("Failed to set size");
        return false;
    }
    fat_SetFileBlockOffset(file, 0);
    bool good = fat_WriteFile(file, blockSize.quot + extraBlock, buffer) == blockSize.quot + extraBlock;
    return good;
}

bool createDirectory(const char* path, const char* name) {
    fat_error_t faterr;
    usb_WaitForEvents();
    faterr = fat_Create(&global->fat, path, name, FAT_DIR);
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
    usb_WaitForEvents();
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
        usb_WaitForEvents();
        fat_Close(&global->fat);
    }
    if (global->storageInit == true) {
        usb_WaitForEvents();
        //msd_Close(&global->msd);
    }
    usb_Cleanup();
    cursorBackground->width = 1;
    cursorBackground->height = 1;
}