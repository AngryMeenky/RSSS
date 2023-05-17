import RsssCrc8 as crc8
import RsssCrc16 as crc16


class RSSS:
  def __init__(self, serial, tail = False):
    self.__readCrc   = 0
    self.__readSync  = 0
    self.__writeCrc  = 0
    self.__writeSync = 0
    self.__serial    = serial
    self.__last      = bytearray(b'\x00\x00\x00\x00')
    self.__remain    = 0
    self.__crc       = bytearray(b'\x00\x00')
    self.__hold      = 0
    self.__addTail   = tail
    self.__valid     = not tail


  def crcValid(self):
    return self.__valid


  def read(self, size=1):
    # handle optional CRC processing
    if self.__addTail and self.__remain != 0:
      b = self.__serial.read(self.__remain);

      if len(b) > 0:
        for byte in b:
          self.__crc[2 - self.__remain] = byte
          self.__remain -= 1

        if self.__remain == 0:
          self.__valid = crc16.calculate(self.__crc, self.__readCrc) == 0
          return [ self.__hold ];

      return [] # didn't read enough to validate

    if self.__readSync <= 0:
      self.__readSync = self.__findSync()

    if self.__readSync > 0:
      count = size if self.__readSync > size else self.__readSync
      arr   = self.__serial.read(count)
      self.__readSync -= len(arr)

      if self.__readSync == 0 and self.__addTail:
        self.__remain = 2
        self.__hold = arr[-1]

        if len(self.read(1)) == 0: # dirty hack
          arr = arr[0:-1]

      return arr

    return []


  def write(self, data, size=-1):
    if size == 0 :
      return 0 # bail on lack of synchronization length

    if size < 0:
      size = len(data)
    count = size
    wrote = 0

    if self.__writeSync > 0:
      chunk = self.__writeSync if count > self.__writeSync else count
      arr = data[0:chunk]
      wrote = self.__serial.write(arr)
      if wrote > 0:
        if self.__addTail:
          self.__writeCrc = crc16.calculate(arr, self.__writeCrc)

        self.__writeSync -= wrote
        count -= wrote
        data = data[wrote:]

        if self.writeSync != 0:
          return wrote
        elif self.__addTail:
          self.__serial.write([ self.__writeCrc & 0xFF, (self.__writeCrc >> 8) & 0xFF ])

      else:
        return wrote

    if count > 0:
      self.__emitSync(size)
      self.__writeSync = size

      sent = self.__serial.write(data)
      if sent > 0:
        self.__writeSync -= sent
        wrote += sent
        if self.__addTail:
          self.__writeCrc = crc16.calculate(data[0:sent], self.__writeCrc)
          self.__serial.write([ self.__writeCrc & 0xFF, (self.__writeCrc >> 8) & 0xFF ])

    return wrote


  def flush(self):
    self.__serial.flush()


  def __findSync(self):
    while True:
      byte = self.__serial.read()

      if len(byte) == 0:
        return 0 # ran out of bytes before a synchronization point was found

      self.__last = self.__last[1:]
      self.__last.append(byte[0])

      if self.__last[0] == 0xAA and crc8.validate(self.__last, crc8.SEED):
        self.__readCrc = crc16.SEED
        return self.__last[1] | (self.__last[2] << 8)


  def __emitSync(self, length):
    self.__writeCrc = crc16.SEED
    self.__serial.write(crc8.append([ 0xAA, length & 0xFF, (length >> 8) & 0xFF ], crc8.SEED))

