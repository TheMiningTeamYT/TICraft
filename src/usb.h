#pragma once
#include <fatdrvce.h>
#include <msddrvce.h>
#include <usbdrvce.h>

typedef struct global global_t;
#define usb_callback_data_t global_t
#define fat_callback_usr_t msd_t
#define MAX_PARTITIONS 32

struct global {
    usb_device_t usb;
    msd_t msd;
    fat_t fat;
    bool storageInit;
    bool fatInit;
};

usb_error_t handleUsbEvent(usb_event_t event, void *event_data, usb_callback_data_t *global);
bool readFile(const char* path, const char* name, uint24_t bufferSize, void* buffer);
bool writeFile(const char* path, const char* name, uint24_t size, void* buffer);
bool createDirectory(const char* path, const char* name);
int24_t getSizeOf(const char* path, const char* name);
void deleteFile(const char* path, const char* name);
bool init_USB();
void close_USB();