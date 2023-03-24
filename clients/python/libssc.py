import serial

LIB_ssc_VERSION = 0x01

LOG_ERROR = 0x01
LOG_WARN = 0x02
LOG_INFO = 0x03

LOG_LEVELS = {
    LOG_ERROR : "ERROR",
    LOG_WARN : "WARNING",
    LOG_INFO : "INFO"
}

class sscEvent:
    def __init__(self):
        self.Handlers = []
    def Connect(self, handler):
        self.Handlers.append(handler)
    def Invoke(self, params):
        for x in self.Handlers:
            x(params)

class LibsscDevice:
    def __init__(self, pt, rate) -> None:
        self.port = pt
        self.baudRate = rate
        self.SerialPort = serial.Serial(pt, rate, dsrdtr=True,  timeout=1)
        self.DeviceConnected = False
        self.DeviceName = ""
        self.DeviceLibraryVersion = -1
        self.DeviceVersion = -1
        self.DeviceId = -1
        self.CommandHandlers = {}
        self.Vectors = {}
        self.OnOpen = sscEvent()
        self.OnDeviceLog = sscEvent()
    def _writeCommand(self, instr, a0, a1, a2):
        if not self.SerialPort.is_open:
            raise Exception("Cannot write instruction: serial port is not open.")
        self.SerialPort.write(b''.join([instr, a0, a1, a2, b'\x00', b'\xFF']))
        self.SerialPort.flush()
    def _readProcess(self):
        if self.SerialPort.in_waiting >= 6:
            byteData = self.SerialPort.read(6)
            if byteData[0] == 0x07: #Exception packet
                moreData = self.SerialPort.read_until("\n")
                fullFrame = b''.join([byteData, moreData])
                raise Exception("Device Exception Thrown: " + fullFrame.decode("ascii"))
            if byteData[5] != 0xFF:
                raise Exception("Invalid packet: " + byteData.decode("ascii"))
            if byteData[0] == 0x03:
                bSize = byteData[2] << 8 | byteData[1]
                vKey = byteData[4] << 8 | byteData[3]
                vectorData = self.SerialPort.read(bSize)
                self.Vectors[vKey] = vectorData
            else:
                self.CommandHandlers[byteData[0] << 0](byteData[1] << 0, byteData[2] << 0, byteData[3] << 0)
    def ReadVector(self, key) -> bytes:
        return self.Vectors[key]
    def MarkVectorUnused(self, key):
        del self.Vectors[key]
    def _deviceHandleInfo(self, a0, a1, a2):
        self.DeviceId = a0
        self.DeviceLibraryVersion = a1
        self.DeviceVersion = a2
    def _onLog(self, a0, a1, a2):
        vKey = a2 << 8 | a1
        self.OnDeviceLog.Invoke({
            "Type":a0,
            "Message":self.ReadVector(vKey).decode("ascii")
        })
        self.MarkVectorUnused(vKey)
    def _deviceHandleName(self, a0, a1, a2):
        vKey = a1 << 8 | a0
        self.DeviceName = self.ReadVector(vKey).decode('ascii')
        self.MarkVectorUnused(vKey)
        self.OnOpen.Invoke(None)
        self.DeviceConnected = True
    def DeviceBegin(self) ->None:
        self.RegisterCommand(0x04, self._onLog)
        self.RegisterCommand(0x01, self._deviceHandleInfo)
        self.RegisterCommand(0x02, self._deviceHandleName)
        self._writeCommand(b'\x01', b'\x00', b'\x00', b'\x00')
        self._writeCommand(b'\x02', b'\x00', b'\x00', b'\x00')
        pass
    def DeviceDoRead(self):
        self._readProcess()
        # print(self.Vectors)
    def RegisterCommand(self, cid, val):
        self.CommandHandlers[cid] = val
    def WriteCommand(self, instr, a0, a1, a2):
        self._writeCommand(instr.to_bytes(1, 'big'), a0.to_bytes(1, 'big'), a1.to_bytes(1, 'big'), a2.to_bytes(1, 'big'))
    
