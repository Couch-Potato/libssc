#pragma once
#include "stdlib.h"
#include <stdio.h>
#include <string.h>

#ifdef IMPL_XC8
#include "impl/impl_xc8.h"
#endif

#define LIBRARY_VERSION 0x1
#define S_BUFFER_SIZE 0x06

#define _NULLPTR 0x0

#define LOG_ERROR 0x01
#define LOG_WARN 0x02
#define LOG_INFO 0x03

typedef unsigned long int size_t;

struct SVectorArray{
    void* elements;
    size_t size;
    size_t elementSize;  
};
/// @brief A vector array object.
typedef struct SVectorArray SVectorArray;
/// @brief Creates a vector data type.
/// @param elementSize The size of the individual element that is within this array
/// @remarks This type requires its dispose method to be called to destruct and release all used resources.
/// @return The pointer to the type.
SVectorArray* SCreateVector(size_t elementSize);
/// @brief Appends an element to the array
/// @param arr The array to modify
/// @param element The element to be added.
/// @remarks Technically, you *can* mix and match datatypes in this array, however they must be of the same size.
void SVectorAppend(SVectorArray* arr, void* element);
/// @brief Indexes the array.
/// @param arr The array.
/// @param index The index.
/// @return The element at the index or '0' if none found. 
void *SIndexVector(SVectorArray *arr, size_t index);
/// @brief Removes an element from the array. 
/// @param arr The array.
/// @param index The index to remove.
void SVectorRemove(SVectorArray* arr, size_t index);
/// @brief Disposes and releases all memory objects used by the array.
/// @param arr The array.
/// @remarks If you are having an array of pointers, make sure to 
void SVectorDispose(SVectorArray* arr);

typedef void (*_S_M_BEGIN)(uint32_t);
typedef int (*_S_M_READY)();
typedef uint8_t (*_S_M_READBYTE)();
typedef void (*_S_M_PUTBYTE)(char byte);

struct SCommunicationAdapter
{
    _S_M_BEGIN BEGIN;
    _S_M_READY READY;
    _S_M_READBYTE READ;
    _S_M_PUTBYTE WRITE;
    char* byteBuffer;
    uint8_t byteBufferSize;
    uint8_t byteBufferIndex;
    uint8_t byteBufferReadIndex;
    uint8_t byteBufferAvailable;
};
/// @brief A communications adapter. Used to implement the proper methods for a serial bus and read/write buffers coded in.
typedef struct SCommunicationAdapter SCommunicationAdapter;

/// @brief Creates a custom communications adapter with implementation functions.
/// @param begin Begin function
/// @param ready Ready function
/// @param read Read function
/// @param write Write function
/// @return A communication adapter object.
SCommunicationAdapter *SCreateAdapter(_S_M_BEGIN begin, _S_M_READY ready, _S_M_READBYTE read, _S_M_PUTBYTE write);
void SABegin(SCommunicationAdapter*, uint32_t baudRate);
int SAReady(SCommunicationAdapter*);
void SAWrite(SCommunicationAdapter*, char);
uint8_t SARead(SCommunicationAdapter*);
uint8_t* SAReadBytes(SCommunicationAdapter*, uint8_t size);
uint8_t *SAReadBytesUntilFull(SCommunicationAdapter *, uint8_t size);
void SAWriteBytes(SCommunicationAdapter*, size_t size, char* buffer);
void SALoop(SCommunicationAdapter*);
uint8_t SABytesAvailable(SCommunicationAdapter* adapter);
void SADispose(SCommunicationAdapter* adapter);

struct SMap{
    SVectorArray* values;
    SVectorArray* keys;
};
typedef struct SMap SMap;

SMap* SCreateMap(size_t key, size_t values);
void SMapAppend(SMap*, void* key, void* value);
void* SMapIndex(SMap*, void* key);
void SMapRemove(SMap*, void* key);
void* SMapRawIndex(SMap*, size_t index);
void SMapDispose(SMap*);



struct SManagedBinary{
    void* ptr;
    size_t size;
};

typedef struct SManagedBinary SManagedBinary;

struct SDevice
{
    char* deviceName;
    size_t deviceNameSize;
    uint8_t deviceId;
    uint8_t deviceVersion;
    SCommunicationAdapter* adapter;
    SVectorArray* handlers;
    SMap* managedBuffers;
    int deviceState;
    uint16_t bufferCtr;
};
typedef struct SDevice SDevice;
typedef void (*_S_H_HANDLE)(SDevice* device, uint8_t, uint8_t, uint8_t);

SDevice* SCreateDevice(char* deviceName, uint8_t deviceId, uint8_t deviceVersion);
void SDeviceAttachAdapter(SDevice* device, SCommunicationAdapter* adapter);
void SDeviceBegin(SDevice* device, uint32_t Rate);
int SDeviceState(SDevice* device);
void SDeviceLoop(SDevice* device);
void SDeviceRegisterHandler(SDevice* device, _S_H_HANDLE handle);
void SWriteCommand(SDevice* device, uint8_t inst, uint8_t a, uint8_t b, uint8_t c);
SManagedBinary* SDeviceReadBuffer(SDevice* device, uint16_t addr);
void SDeviceClearBuffer(SDevice* device, uint16_t addr);
void SDeviceClearAllBuffers(SDevice* device);
uint16_t SDeviceWriteBuffer(SDevice* device, char* buffer, size_t length);
void SDeviceDispose(SDevice* device);
void SDeviceLog(SDevice* device, uint8_t logLevel, char* msg);
SManagedBinary* SDeviceBufferParam(SDevice* device, uint8_t a, uint8_t b);
void SDevicePanicException(SDevice* device, char* msg, unsigned char stopCode);
