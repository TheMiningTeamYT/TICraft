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
#include "usb.h"

static msd_partition_t partitions[MAX_PARTITIONS];
static global_t global;
uint8_t num_partitions;
msd_info_t msdinfo;
usb_error_t usberr;
msd_error_t msderr;
fat_error_t faterr;

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
    memset(&global, 0, sizeof(global_t));
    uint8_t key;
    global.usb = NULL;
    usberr = usb_Init((usb_event_callback_t) handleUsbEvent, &global, NULL, USB_DEFAULT_INIT_FLAGS);
    while (usberr == USB_SUCCESS) {
        if (global.usb != NULL) break;
        key = os_GetCSC();
        if (key != 0) {
            return false;
        }
        usberr = usb_WaitForInterrupt();
    }
    if (usberr != USB_SUCCESS) {
        return false;
    }
    msderr = msd_Open(&global.msd, global.usb);
    if (msderr != MSD_SUCCESS) {
        return false;
    }
    global.storageInit = true;
    msderr = msd_Info(&global.msd, &msdinfo);
    if (msderr != MSD_SUCCESS) {
        return false;
    }
    num_partitions = msd_FindPartitions(&global.msd, partitions, MAX_PARTITIONS);
    if (num_partitions < 1) {
        return false;
    }
    for (uint8_t i = 0; i < num_partitions; i++) {
        uint32_t base_lba = partitions[i].first_lba;
        fat_callback_usr_t* usr = &global.msd;
        fat_read_callback_t read = (fat_read_callback_t) &msd_Read;
        fat_write_callback_t write = (fat_write_callback_t) &msd_Write;
        faterr = fat_Open(&global.fat, read, write, usr, base_lba);
        if (faterr == FAT_SUCCESS) {
            global.fatInit = true;
            return true;
        }
    }
    return false;
}

bool readFile(const char* path, const char* name, uint24_t bufferSize, void* buffer) {
    usb_WaitForEvents();
    fat_file_t file;
    char str[256];
    strncpy(str, path, 256);
    str[255] = 0; 
    if (str[strlen(str) - 1] != '/') {
        strncat(str, "/", 256-strlen(str));
    }
    strncat(str, name, 256-strlen(str));
    faterr = fat_OpenFile(&global.fat, str, 0, &file);
    if (faterr != FAT_SUCCESS) {
        // os_PutStrFull("Failed to open file");
        // os_PutStrFull(str);
        return false;
    }
    uint32_t readSize = fat_GetFileSize(&file);
    if (readSize > bufferSize - (bufferSize%MSD_BLOCK_SIZE) - MSD_BLOCK_SIZE) {
        readSize = bufferSize - (bufferSize%MSD_BLOCK_SIZE) - MSD_BLOCK_SIZE;
    }
    fat_SetFileBlockOffset(&file, 0);
    bool good = fat_ReadFile(&file, (readSize/MSD_BLOCK_SIZE)+1, buffer) == (readSize/MSD_BLOCK_SIZE)+1;
    fat_CloseFile(&file);
    if (!good) {
        // os_PutStrFull("Failed to read data");
        // os_PutStrFull(str);
    }
    return good;
}

bool writeFile(const char* path, const char* name, uint24_t size, void* buffer) {
    usb_WaitForEvents();
    fat_file_t file;
    char str[256];
    strncpy(str, path, 256);
    str[255] = 0; 
    if (str[strlen(str) - 1] != '/') {
        strncat(str, "/", 256-strlen(str));
    }
    strncat(str, name, 256-strlen(str));
    fat_Delete(&global.fat, str);
    faterr = fat_Create(&global.fat, path, name, 0);
    if (faterr != FAT_SUCCESS) {
        os_PutStrFull("Failed to create file");
        os_PutStrFull(str);
        return false;
    }
    faterr = fat_OpenFile(&global.fat, str, 0, &file);
    if (faterr != FAT_SUCCESS) {
        os_PutStrFull("Failed to open file");
        os_PutStrFull(str);
        return false;
    }
    faterr = fat_SetFileSize(&file, size);
    if (faterr != FAT_SUCCESS) {
        os_PutStrFull("Failed to set size");
        os_PutStrFull(str);
        fat_CloseFile(&file);
        return false;
    }
    fat_SetFileBlockOffset(&file, 0);
    bool good = fat_WriteFile(&file, (size/MSD_BLOCK_SIZE)+1, buffer) == (size/MSD_BLOCK_SIZE)+1;
    fat_CloseFile(&file);
    if (!good) {
        // os_PutStrFull("Failed to write data");
        // os_PutStrFull(str);
        return false;
    }
    return true;
}

bool createDirectory(const char* path, const char* name) {
    usb_WaitForEvents();
    faterr = fat_Create(&global.fat, path, name, FAT_DIR);
    return true;
} 

int24_t getSizeOf(const char* path, const char* name) {
    usb_WaitForEvents();
    fat_file_t file;
    char str[256];
    strncpy(str, path, 256);
    str[255] = 0; 
    if (str[strlen(str) - 1] != '/') {
        strncat(str, "/", 256-strlen(str));
    }
    strncat(str, name, 256-strlen(str));
    faterr = fat_OpenFile(&global.fat, str, 0, &file);
    if (faterr != FAT_SUCCESS) {
        return 0;
    }
    int24_t size = fat_GetFileSize(&file);
    fat_CloseFile(&file);
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
    fat_Delete(&global.fat, str);
}

void close_USB() {
    // os_PutStrFull("Closing USB...");
    // os_PutStrFull("Closing FAT");
    if (global.fatInit == true) {
        usb_WaitForEvents();
        fat_Close(&global.fat);
    }
    // os_PutStrFull("Closing MSD");
    if (global.storageInit == true) {
        usb_WaitForEvents();
        //msd_Close(&global.msd);
    }
    // os_PutStrFull("Cleaning up USB");
    usb_Cleanup();
    // os_PutStrFull("USB Closed!");
}