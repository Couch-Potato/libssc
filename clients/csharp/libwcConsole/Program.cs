using libsscSharp;

LibsscDevice device = new LibsscDevice("COM3", 9600);
device.OnOpen += Device_OnOpen;
device.OnLog += Device_OnLog;


 

void Device_OnLog(byte arg1, string arg2)
{
    Console.WriteLine($"[DEVICE::{device.DeviceId}] [{Helper.GetLogType(arg1)}] {arg2}");
}

void Device_OnOpen()
{
    ConsoleColor defaultColor = Console.ForegroundColor;
    Console.WriteLine("============ Libssc DEVICE ============");
    Console.Write("Device Name: ");
    Console.ForegroundColor = ConsoleColor.Cyan;
    Console.WriteLine(device.DeviceName);
    Console.ForegroundColor = defaultColor;
    Console.Write($"Device Id: {device.DeviceId}\nDevice Version: {device.DeviceVersion}\nDevice Libssc Version: {device.DeviceLibraryVersion}\n");
    Console.Write("Status: ");
    Console.ForegroundColor = ConsoleColor.Green;
    Console.WriteLine("CONNECTED");
    Console.ForegroundColor = defaultColor;
    Console.WriteLine("======================================");
    device.WriteCommand(0x10, 0, 0, 0);
}

device.DeviceBegin();

while (true)
{
    device.DeviceDoRead();
}