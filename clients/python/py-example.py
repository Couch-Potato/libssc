import libssc

device = libssc.LibsscDevice("COM3", 9600)
device.OnOpen.Connect(
    lambda data:
        print("Device connected --\nDevice Name: " + str(device.DeviceName) + "\nDevice Id: " + str(device.DeviceId) + "\nDevice Version: " + str(device.DeviceVersion) + "\nLibrary Version: " + str(device.DeviceLibraryVersion) + "\nPython Library Version: " + str(libssc.LIB_ssc_VERSION))
)
device.OnOpen.Connect(
    lambda data:
        device.WriteCommand(16, 0, 0,0)
)
device.OnDeviceLog.Connect(
    lambda data:
        print("[DEVICE::" + str(device.DeviceId) + "] [" + libssc.LOG_LEVELS[data["Type"]] + "] " + data["Message"])
)
device.DeviceBegin()
while True:
    device.DeviceDoRead()
