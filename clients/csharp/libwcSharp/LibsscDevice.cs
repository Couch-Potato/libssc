using System.IO.Ports;
using System.Text;

namespace libsscSharp
{
    public class InvalidPacketException : Exception
    {
        public InvalidPacketException(string message) : base("Invalid packet returned: " + message) { }
    }
    public class DeviceException : Exception
    {
        public DeviceException(string message) : base("Device exception: " + message) { }
    }
    public class InvalidInstructionException : Exception
    {
        public InvalidInstructionException(byte instr) : base("The following instruction does not exist: " + instr) { }
    }
    public static class Helper
    {
        public static ushort GetUShort(byte a, byte b)
        {
            return (ushort)(b << 8 | a);
        }
        public static string GetLogType(byte a)
        {
            switch (a)
            {
                case 1:
                    return "ERROR";
                case 2:
                    return "WARNING";
                case 3:
                    return "INFO";
                default:
                    return "INVALID";
            }
        }
    }
    public class LibsscDevice
    {
        private int _baudRate;
        private string _port;
        private SerialPort serialPort;
        private Dictionary<ushort, byte[]> Vectors = new Dictionary<ushort, byte[]>();
        private Dictionary<byte, Action<byte, byte, byte>> Handlers = new Dictionary<byte, Action<byte, byte, byte>>();
        public event Action OnOpen;
        public event Action<byte, string> OnLog;
        public int DeviceId;
        public int DeviceLibraryVersion;
        public int DeviceVersion;
        public string DeviceName;
        public LibsscDevice(string port, int baudRate)
        {
            _port = port;
            _baudRate = baudRate;
            serialPort = new SerialPort(port, baudRate);
            serialPort.Open();
            serialPort.ReadTimeout = 1000;
        }
        public byte[] ReadVector(ushort vectorId)
        {
            return Vectors[vectorId];
        }
        public void MarkVectorUnused(ushort vectorId)
        {
            Vectors.Remove(vectorId);
        }
        public void RegisterCommand(byte type, Action<byte, byte, byte> handler)
        {
            Handlers.Add(type, handler);
        }
        private void writeCommand(byte instruction, byte a0, byte a1, byte a2)
        {
            serialPort.Write(new byte[6] { instruction, a0, a1, a2,0x00, 0xFF }, 0, 6);
            serialPort.BaseStream.Flush();
        }

        private void _builtin_onLog(byte a, byte b, byte c)
        {
            OnLog?.Invoke(a, Encoding.ASCII.GetString(ReadVector(Helper.GetUShort(b, c))));
            MarkVectorUnused(Helper.GetUShort(b, c));
        }
        private void _builtin_onInfo(byte a, byte b, byte c)
        {
            DeviceId = a;
            DeviceLibraryVersion = b;
            DeviceVersion = c;
        }
        private void _builtin_onName(byte a, byte b, byte c)
        {
            string name = Encoding.ASCII.GetString(ReadVector(Helper.GetUShort(a, b)));
            DeviceName = name;
            MarkVectorUnused(Helper.GetUShort(a, b));
            OnOpen?.Invoke();
            WriteCommand(0x04, 0, 0, 0);
        }

        public void DeviceBegin()
        {
            RegisterCommand(0x04, _builtin_onLog);
            RegisterCommand(0x01, _builtin_onInfo);
            RegisterCommand(0x02, _builtin_onName);
            writeCommand(0x01, 0x00, 0x00, 0x00);
            writeCommand(0x02, 0x00, 0x00, 0x00);
        }
        public void WriteCommand(byte instruction, byte a0, byte a1, byte a2)
        {
            writeCommand(instruction, a0, a1, a2);
        }
        public void DeviceDoRead()
        {
            processRead();
        }
        private void processRead()
        {
            if (serialPort.BytesToRead >= 6)
            {
                byte[] byteBuffer = new byte[6];
                serialPort.Read(byteBuffer, 0, 6);
                if (byteBuffer[5] != 0xFF)
                    throw new InvalidPacketException("packet does not have checkbyte set.");
                if (byteBuffer[0] == 0x07)
                {
                    string moreException = serialPort.ReadLine() + Encoding.ASCII.GetString(byteBuffer);
                    throw new DeviceException(moreException);
                }
                if (byteBuffer[0] == 0x03)
                {
                    ushort dSize = (ushort)(byteBuffer[2] << 8 | byteBuffer[1]);
                    ushort dKey = (ushort)(byteBuffer[4] << 8 | byteBuffer[3]);
                    if (Vectors.ContainsKey(dKey))
                    {
                        Vectors.Add(dKey, new byte[1]);
                    }
                    Vectors[dKey] = new byte[dSize];
                    while (serialPort.BytesToRead < dSize) { }
                    int read = serialPort.Read(Vectors[dKey], 0, dSize);
                }
                else
                {
                    if (Handlers.ContainsKey(byteBuffer[0]))
                    {
                        Handlers[byteBuffer[0]](byteBuffer[1], byteBuffer[2], byteBuffer[3]);
                    }else
                    {
                        throw new InvalidInstructionException(byteBuffer[0]);
                    }
                }
            }
        }

    }
}