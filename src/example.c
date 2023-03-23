#define IMPL_XC8
#include "libssc.h"

void serial_begin(int baud){}
int serial_ready(){}
uint8_t readByte(){}
void putByte(char c){}

void helloWorld(SDevice *device, uint8_t a, uint8_t b, uint8_t c){
    SDeviceLog(device, LOG_INFO, "Hello World!");
}

int main()
{
    SDevice* device = SCreateDevice("Cool Device", 123, 2);
    // SCommunicationAdapter* adapter = SCreateAdapter(
    //     &serial_begin,
    //     &serial_ready,
    //     &readByte,
    //     &putByte
    // );
    SDeviceAttachAdapter(device, GetDeviceSerialAdapter());
    SDeviceRegisterHandler(device, &helloWorld);
    SDeviceBegin(device, 9600);
    while (1){
        SDeviceLoop(device);
    }
}