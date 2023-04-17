import RsssCrc8 as crc8


class RSSS:
  def __init__(self, serial):
    self.__readSync  = 0
    self.__writeSync = 0
    self.__serial    = serial
    self.__last      = [0, 0, 0, 0]


  def read(self, size=1):
    if self.__readSync <= 0:
      self.__readSync = self.__findSync()

    if self.__readSync > 0:
      count = size if self.__readSync > size else self.__readSync
      arr   = self.__serial.read(count)
      self.__readSync -= len(arr)

      return arr

    return []


  def write(self, data, size=-1):
    if size == 0 :
      return 0 # bail on lack of synchronization length

    if size <= 0:
      size = len(data)
    count = size
    wrote = 0

    if self.__writeSync > 0:
      chunk = self.__writeSync if count > self.__writeSync else count
      wrote = self.__serial.write(data[0:chunk])
      if wrote > 0:
        self.__writeSync -= wrote
        count -= wrote
        data = data[wrote:]
      else:
        return wrote

    if count > 0:
      self.__emitSync(size)
      self.__writeSync = size

      sent = self.__serial.write(data)
      if sent > 0:
        self.__writeSync -= sent
        wrote += sent

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

      if self.__last[0] == 0xAA and crc8.validate(self.__last, 0x78):
        return self.__last[1] | (self.__last[2] << 8)


  def __emitSync(self, length):
    self.__serial.write(crc8.append([ 0xAA, length & 0xFF, (length >> 8) & 0xFF ], 0x78))

