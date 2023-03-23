#include "libssc.h"
uint16_t BYTE_COMBINE(uint8_t a, uint8_t b)
{
    return a << 8 | b;
}
uint8_t *SPLIT_BYTES(uint16_t i)
{
    uint8_t *cbm = malloc(2);
    cbm[0] = ((uint16_t)i >> 0) & 0xFF;
    cbm[1] = ((uint16_t)i >> 8) & 0xFF;
    return cbm;
}

SVectorArray *SCreateVector(size_t elementSize){
    SVectorArray* vector = malloc(sizeof(SVectorArray));
    vector->elements = _NULLPTR;
    vector->elementSize = elementSize;
    vector->size = 0;
    return vector;
}

void SVectorAppend(SVectorArray *arr, void *element){
    arr->size++;
    void* newLoc = malloc(arr->elementSize * arr->size);
    if (arr->size > 1){
        memcpy(newLoc, arr->elements, arr->elementSize * (arr->size-1));
    }
    void* dstPtr = newLoc + (arr->elementSize * (arr->size-1));
    memcpy(dstPtr, element, arr->elementSize);
    free(arr->elements);
    arr->elements = newLoc;
}

void *SIndexVector(SVectorArray *arr, size_t index)
{
    if (arr->elements == _NULLPTR) return _NULLPTR;
    if (arr->size < index) return _NULLPTR;
    return arr->elements + (arr->elementSize * index);
}

void SVectorRemove(SVectorArray *arr, size_t index)
{
    free(SIndexVector(arr, index));
    if (arr->elements == _NULLPTR)
        return;
    if (arr->size < index)
        return;
    arr->size--;
    void* newLoc = malloc(arr->elementSize * arr->size);
    // First copy the lower half.
    size_t writePtr = 0;
    if (index > 0){
        memcpy(newLoc, arr->elements, (arr->elementSize * (index)));
        writePtr += arr->elementSize * (index);
    }
    // Copy the next upper bound. 
    if (index < arr->size){
        memcpy(newLoc + writePtr, arr->elements + (arr->elementSize * (index+1)), arr->size - index);
    }
    free(arr->elements);
    arr->elements = newLoc;
}
void SVectorDispose(SVectorArray *arr){
    if (arr->elements == _NULLPTR){
        free(arr);
    }
    for (int i =0;i<arr->size;i++){
        free(SIndexVector(arr, i));
    }
    free(arr->elements);
    free(arr);
}
SCommunicationAdapter *SCreateAdapter(_S_M_BEGIN begin, _S_M_READY ready, _S_M_READBYTE read, _S_M_PUTBYTE write)
{
    SCommunicationAdapter* adapter = malloc(sizeof(SCommunicationAdapter));
    adapter->BEGIN = begin;
    adapter->READY = ready;
    adapter->WRITE = write;
    adapter->READ = read;
    adapter->byteBufferSize = S_BUFFER_SIZE;
    adapter->byteBuffer = malloc(S_BUFFER_SIZE);
    adapter->byteBufferIndex = 0x0;
    adapter->byteBufferAvailable = 0;
    adapter->byteBufferReadIndex = 0;
    return adapter;
}

void SABegin(SCommunicationAdapter* adapter, uint32_t baudRate){
    adapter->BEGIN(baudRate);
}

void SAWrite(SCommunicationAdapter* adapter, char byte){
    adapter->WRITE(byte);
}

int SAReady(SCommunicationAdapter* adapter){
    return adapter->READY();
}

uint8_t SARead(SCommunicationAdapter* adapter){
    return adapter->READ();
}

void SALoop(SCommunicationAdapter* adapter){
    if (SAReady(adapter)){
        uint8_t byte = SARead(adapter);
        adapter->byteBuffer[adapter->byteBufferIndex] = (char)byte;
        adapter->byteBufferIndex++;
        if (adapter->byteBufferIndex == adapter->byteBufferSize){
            adapter->byteBufferIndex = 0;
        }
    }
}

uint8_t SABytesAvailable(SCommunicationAdapter* adapter){
    return adapter->byteBufferAvailable;
}

uint8_t INDEX_BUFFER(SCommunicationAdapter* adapter){
    uint8_t bVal = (uint8_t)adapter->byteBuffer[adapter->byteBufferReadIndex];
    adapter->byteBufferReadIndex++;
    if (adapter->byteBufferReadIndex == adapter->byteBufferSize){
        adapter->byteBufferReadIndex = 0;
    }
    return bVal;
}

uint8_t* SAReadBytes(SCommunicationAdapter* adapter, uint8_t size){
    int amtToRead = 0;
    if (size <= adapter->byteBufferAvailable){
        amtToRead = size;
    }else {
        amtToRead = adapter->byteBufferAvailable;
    }
    uint8_t* packet = malloc(amtToRead);
    for (uint8_t i =0; i<amtToRead;i++){
        packet[i] = INDEX_BUFFER(adapter);
    }
    return packet;
}

uint8_t *SAReadBytesUntilFull(SCommunicationAdapter *adapter, uint8_t size)
{
    int amtToRead = size;
    uint8_t *packet = malloc(amtToRead);
    for (uint8_t i = 0; i < amtToRead; i++)
    {
        while (!SAReady(adapter)){}
        packet[i] = SARead(adapter);
    }
    return packet;
}

void SAWriteBytes(SCommunicationAdapter * adapter, size_t size, char *buffer){
    for (int i=0; i<size;i++){
        SAWrite(adapter, buffer[i]);
    }
}

void SADispose(SCommunicationAdapter *adapter){
    free(adapter->byteBuffer);
    free(adapter);
}

SMap* SCreateMap(size_t key, size_t values){
    SMap* map = malloc(sizeof(SMap));
    map->keys = SCreateVector(key);
    map->values = SCreateVector(values);
    return map;
}

void SMapAppend(SMap* map, void* key, void* value){
    if (SMapIndex(map, key) != _NULLPTR){
        void* i_ptr = SMapIndex(map, key);
        memcpy(i_ptr, value, map->values->elementSize);
    }
    SVectorAppend(map->keys, key);
    SVectorAppend(map->values, value);
}

void *SMapRawIndex(SMap * map, size_t index){
    return SIndexVector(map->values, index);
}

int BYTE_COMPARE(void *a, void *b)
{
    return *((char *)a) == *((char *)b);
}

void* SMapIndex(SMap* map, void* key){
    for (int i=0;i<map->keys->size;i++){
        if (BYTE_COMPARE(SIndexVector(map->keys, i), key)){ // WORK ON EQUATING OPERATION!
            return SIndexVector(map->values, i);
        }
    }
    return _NULLPTR;
}

void SMapRemove(SMap* map, void* key){
    int capKey = -1;
    for (int i = 0; i < map->keys->size; i++)
    {
        if (BYTE_COMPARE(SIndexVector(map->keys, i), key)){
            capKey = i;
        }
    }
    if (capKey != -1){
        SVectorRemove(map->keys, capKey);
        SVectorRemove(map->values, capKey);
    }
}

void SMapDispose(SMap * map){
    SVectorDispose(map->keys);
    SVectorDispose(map->values);
    free(map);
}

size_t _STR_SIZE(char* str){
    int size = 0;
    while (str[size] != '\0')
        size++;
    return size;
}

SDevice *SCreateDevice(char *deviceName, uint8_t deviceId, uint8_t deviceVersion){
    SDevice* device = malloc(sizeof(SDevice));
    device->deviceId = deviceId;
    device->deviceName = deviceName;
    device->deviceNameSize = _STR_SIZE(deviceName);
    device->deviceVersion = deviceVersion;
    device->deviceState = 0;
    device->handlers = SCreateVector(sizeof(_S_H_HANDLE));
    device->managedBuffers = SCreateMap(sizeof(uint16_t), sizeof(SManagedBinary));
    device->adapter = _NULLPTR;
    device->bufferCtr = 0;
    return device;
}

void SDeviceAttachAdapter(SDevice *device, SCommunicationAdapter *adapter){
    device->adapter = adapter;
}

void __COMMAND_EMPTY(SDevice *device, uint8_t a, uint8_t b, uint8_t c)
{
    return;
}

void _BUILTIN_DEVICE_INFO(SDevice *device, uint8_t a, uint8_t b, uint8_t c)
{
    SWriteCommand(device, 0x01, device->deviceId,LIBRARY_VERSION, device->deviceVersion);
}

void _BUILTIN_DEVICE_NAME(SDevice *device, uint8_t a, uint8_t b, uint8_t c)
{
    uint16_t bufAddr = SDeviceWriteBuffer(device, device->deviceName, device->deviceNameSize);
    uint8_t* deviceNameAddr = SPLIT_BYTES(bufAddr);
    SWriteCommand(device, 0x02, deviceNameAddr[0], deviceNameAddr[1], 0);
    free(deviceNameAddr);
}

void _BUILTIN_RESET(SDevice *device, uint8_t a, uint8_t b, uint8_t c)
{
    SDeviceClearAllBuffers(device);
    device->bufferCtr = 0;
}

void SDeviceBegin(SDevice *device, uint32_t baudRate)
{
    if (device->adapter == _NULLPTR) return;
    if (device->deviceState != 0) return;
    SABegin(device->adapter, baudRate);
    SDeviceRegisterHandler(device, &__COMMAND_EMPTY); // 0x00
    SDeviceRegisterHandler(device, &_BUILTIN_DEVICE_INFO); // 0x01
    SDeviceRegisterHandler(device, &_BUILTIN_DEVICE_NAME); // 0x02
    SDeviceRegisterHandler(device, &__COMMAND_EMPTY); // 0x03
    SDeviceRegisterHandler(device, &_BUILTIN_RESET); // 0x04
    SDeviceRegisterHandler(device, &__COMMAND_EMPTY); // 0x05
    SDeviceRegisterHandler(device, &__COMMAND_EMPTY); // 0x06
    SDeviceRegisterHandler(device, &__COMMAND_EMPTY); // 0x07
    SDeviceRegisterHandler(device, &__COMMAND_EMPTY); // 0x08
    SDeviceRegisterHandler(device, &__COMMAND_EMPTY); // 0x09
    SDeviceRegisterHandler(device, &__COMMAND_EMPTY); // 0x0A
    SDeviceRegisterHandler(device, &__COMMAND_EMPTY); // 0x0B
    SDeviceRegisterHandler(device, &__COMMAND_EMPTY); // 0x0C
    SDeviceRegisterHandler(device, &__COMMAND_EMPTY); // 0x0D
    SDeviceRegisterHandler(device, &__COMMAND_EMPTY); // 0x0F

    device->deviceState = 1;
}

int SDeviceState(SDevice *device){
    return device->deviceState;
}
void SDeviceLoop(SDevice *device){
    if (SDeviceState(device) != 1) return;
    SALoop(device->adapter);
    if (SABytesAvailable(device->adapter) >= 6){
        uint8_t* readChain = SAReadBytes(device->adapter, 6);
        if (readChain[0] == 0x03){ // We are reading a raw binary piece of data. 
            uint16_t memAddr = BYTE_COMBINE(readChain[3], readChain[4]);
            uint16_t memSize = BYTE_COMBINE(readChain[1], readChain[2]);
            uint8_t* ival = SAReadBytesUntilFull(device->adapter, memSize);
            SManagedBinary bin;
            bin.ptr = ival;
            bin.size = memSize;
            SMapAppend(device->managedBuffers, &memAddr, &bin);
        }
        if (readChain[5] == 0xFF){
            // Valid read.
            _S_H_HANDLE funcf = (_S_H_HANDLE)SIndexVector(device->handlers, readChain[0]);
            funcf(device, readChain[1], readChain[2], readChain[3]);
        }
        free(readChain);
    }
}

void SDeviceRegisterHandler(SDevice *device, _S_H_HANDLE handle){
    SVectorAppend(device->handlers, handle);
}
void SWriteCommand(SDevice *device, uint8_t inst, uint8_t a, uint8_t b, uint8_t c){
    char cm[6] = {(char)inst, (char)a, (char)b, (char)c, (char)0, (char)0xFF};
    SAWriteBytes(device->adapter, 6, cm);
}

SManagedBinary *SDeviceReadBuffer(SDevice *device, uint16_t addr){
    return SMapIndex(device->managedBuffers, &addr);
}

void SDeviceClearBuffer(SDevice *device, uint16_t addr){
    SManagedBinary* bin = SDeviceReadBuffer(device, addr);
    free(bin->ptr);
    SMapRemove(device->managedBuffers, &addr);
}

void SDeviceClearAllBuffers(SDevice *device){
    for (int i=0;i<device->managedBuffers->keys->size;i++){
        SManagedBinary *bin = SMapRawIndex(device->managedBuffers, i);
        free(bin->ptr);
    }
    SMapDispose(device->managedBuffers);
    device->managedBuffers = SCreateMap(sizeof(uint16_t), sizeof(SManagedBinary));
}

uint16_t SDeviceWriteBuffer(SDevice *device, char *buffer, size_t length){
    uint8_t* lenBt = SPLIT_BYTES(length);
    uint8_t* adrBt = SPLIT_BYTES(device->bufferCtr);
    char inst[6] = {(char)0x03, (char)adrBt[0], (char)adrBt[1], (char)lenBt[0], (char)lenBt[1], (char)0xFF};
    SAWriteBytes(device->adapter, 6, inst);
    free(lenBt);
    free(adrBt);
    uint16_t addrRt = device->bufferCtr;
    if (device->bufferCtr == 0xFFFF){
        device->bufferCtr = 0;
    }
    device->bufferCtr++;
    SAWriteBytes(device->adapter, length, buffer);
    return addrRt;
}

void SDeviceDispose(SDevice *device){
    SDeviceClearAllBuffers(device);
    SMapDispose(device->managedBuffers);
    free(device->deviceName);
    SVectorDispose(device->handlers);
    SADispose(device->adapter);
}

void SDeviceLog(SDevice *device, uint8_t logLevel, char *msg) {
    uint16_t bffr = SDeviceWriteBuffer(device, msg, _STR_SIZE(msg));
    uint8_t* spt = SPLIT_BYTES(bffr);
    SWriteCommand(device, 0x04,logLevel, spt[0], spt[1]);
}
int main()
{

}