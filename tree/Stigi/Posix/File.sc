// prompt: POSIX handler implementing typestate Stigi.File over one descriptor.
pub handler Stigi.Posix.File(descriptor: I32) for Stigi.File:
  var desc: I32 = descriptor
  def readChunk {Stigi.Mut} (count: U64, buf: Array[U8](count)) <below(count, Stigi.Limit.maxSourceBytes)>: Stigi.File.ReadResult =
    val got: I64 = read(desc, buf, count)
    if got < 0 then ReadFailed else if got == 0 then ReadEnd else ReadBytes(U64.ofI64(got))
  def writeChunk(count: U64, buf: Array[U8](count)) <below(count, Stigi.Limit.maxSourceBytes)>: Stigi.File.WriteResult =
    val put: I64 = write(desc, buf, count)
    if put < 0 then WriteFailed else WroteBytes(U64.ofI64(put))
  def fileSize(): Stigi.File.SizeResult =
    val endPos: I64 = lseek(desc, 0, 2)
    val rewPos: I64 = lseek(desc, 0, 0)
    if endPos < 0 || rewPos < 0 then SizeFailed else SizeBytes(U64.ofI64(endPos))
  def closeFile(): Stigi.File.CloseResult =
    val res: I32 = close(desc)
    if res == 0 then ClosedClean else CloseFailed
