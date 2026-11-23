// prompt: decode Linux x86-64 struct rusage fields at offsets 32, 64, and 72.
pub def Stigi.Posix.decodeUsageBuffer(usageBytes: Array[U8](144)) <lessOrEqual(0, 1)>: Stigi.Posix.UsageSnapshot =
  val maxRss: U64 = Stigi.Posix.readLittleEndianWord(144, usageBytes, 32)
  val minFaults: U64 = Stigi.Posix.readLittleEndianWord(144, usageBytes, 64)
  val majFaults: U64 = Stigi.Posix.readLittleEndianWord(144, usageBytes, 72)
  UsageData(maxRss, minFaults, majFaults)
