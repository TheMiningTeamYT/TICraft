#pragma once
typedef struct global global_t;
#define usb_callback_data_t global_t
#define fat_callback_usr_t msd_t

#include <fatdrvce.h>
#include <msddrvce.h>
#include <usbdrvce.h>

#define MAX_PARTITIONS 32

struct global {
    usb_device_t usb;
    msd_t msd;
    fat_t fat;
    bool storageInit;
    bool fatInit;
};

usb_error_t handleUsbEvent(usb_event_t event, void *event_data, usb_callback_data_t *global);
bool readFile(fat_file_t* file, uint24_t bufferSize, void* buffer);
bool writeFile(fat_file_t* file, uint24_t size, void* buffer);
bool createDirectory(const char* path, const char* name);
fat_file_t* openFile(const char* path, const char* name, bool create);
void closeFile(fat_file_t* file);
int24_t getSizeOf(fat_file_t* file);
void deleteFile(const char* path, const char* name);
bool init_USB();
void close_USB();